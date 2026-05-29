#include "TypeChecker.h"
#include "Lexer.h"
#include <string>

// -------------------------------------------------------------------------
// Program — two-pass: pre-register structs/funcs, then full check
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(ProgramAst* node) {
    // Pass 1: register all struct and function declarations so they can be
    //         used before their lexical position (forward-reference support).
    //         Also detect duplicate type and function names here.
    for (auto& stmt : node->statements) {
        if (auto* sd = dynamic_cast<StructDeclStmtAst*>(stmt.get())) {
            if (struct_table.count(sd->name)) {
                push_error(sd->line, sd->col,
                    "Duplicate type declaration: '" + sd->name + "' is already defined.");
                // Keep the first definition — do not overwrite it.
            } else {
                struct_table[sd->name] = sd->fields;
                // Pre-register method signatures; catch duplicate methods.
                for (auto& m : sd->methods) {
                    if (struct_methods[sd->name].count(m->name)) {
                        push_error(m->line, m->col,
                            "Duplicate method '" + m->name + "' in type '"
                            + sd->name + "': already defined.");
                    } else {
                        FuncSignature sig;
                        for (auto& p : m->params) sig.param_types.push_back(p.type);
                        sig.return_type = m->return_type;
                        struct_methods[sd->name][m->name] = sig;
                    }
                }
            }
        }
        if (auto* fd = dynamic_cast<FuncDeclStmtAst*>(stmt.get())) {
            if (func_table.count(fd->name)) {
                push_error(fd->line, fd->col,
                    "Duplicate function declaration: '" + fd->name + "' is already defined.");
                // Keep the first definition.
            } else {
                FuncSignature sig;
                for (auto& p : fd->params) sig.param_types.push_back(p.type);
                sig.return_type = fd->return_type;
                func_table[fd->name] = sig;
            }
        }
    }

    // Pass 1.5: register top-level variable declarations so functions can reference them.
    for (auto& stmt : node->statements) {
        auto* vd = dynamic_cast<VarDeclStmtAst*>(stmt.get());
        if (!vd) continue;

        if (global_vars.count(vd->name)) {
            push_error(vd->line, vd->col,
                "Variable '" + vd->name + "' is already declared in this scope.");
            continue;
        }
        global_vars[vd->name] = vd->type;

        if (vd->type->to_string() == "void") {
            push_error(vd->line, vd->col, "Cannot declare a variable of type 'void'.");
            continue;
        }

        auto expr_type = get_type(vd->init_expr.get());
        bool types_match = is_same_type(vd->type.get(), expr_type.get());
        if (!types_match) {
            auto arr = std::dynamic_pointer_cast<ArrayTypeAst>(vd->type);
            if (arr && is_same_type(get_deep_base_type(vd->type.get()), expr_type.get()))
                types_match = true;
        }
        if (!types_match) {
            push_error(vd->line, vd->col,
                "Initialisation type '" + expr_type->to_string() +
                "' does not match declared type '" + vd->type->to_string() + "'.");
        }
    }

    // Pass 2: full type-checking
    for (auto& stmt : node->statements) {
        stmt->accept(*this);
    }
}

// -------------------------------------------------------------------------
// Literals
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(IntExprAst*    node) { last_type = tc::TypeAst::get_int(); }
void tc::TypeChecker::visit(FloatExprAst*  node) { last_type = tc::TypeAst::get_float(); }
void tc::TypeChecker::visit(StringExprAst* node) { last_type = tc::TypeAst::get_string(); }
void tc::TypeChecker::visit(BoolExprAst*   node) { last_type = tc::TypeAst::get_bool(); }

// -------------------------------------------------------------------------
// Variable / array / field access
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(VariableExprAst* node) {
    for (int i = (int)scopes.size() - 1; i >= 0; i--) {
        if (scopes[i].count(node->name)) {
            last_type = scopes[i][node->name];
            return;
        }
    }
    auto git = global_vars.find(node->name);
    if (git != global_vars.end()) {
        last_type = git->second;
        return;
    }
    push_error(node->line, node->col, "Undeclared variable: '" + node->name + "'");
    last_type = TypeAst::get_int(); // fallback to avoid cascade errors
}

void tc::TypeChecker::visit(ArrayAccessExprAst* node) {
    auto target_type = get_type(node->target.get());
    auto array_type  = std::dynamic_pointer_cast<ArrayTypeAst>(target_type);
    if (!array_type) {
        push_error(node->line, node->col,
            "Cannot index into a non-array type ('" + target_type->to_string() + "').");
        last_type = target_type;
        return;
    }

    auto index_type = get_type(node->index.get());
    if (index_type->to_string() != "int") {
        push_error(node->line, node->col,
            "Array index must be an integer, but got '" + index_type->to_string() + "'.");
    }

    last_type = array_type->base_type;
}

void tc::TypeChecker::visit(FieldAccessExprAst* node) {
    auto obj_type    = get_type(node->object.get());
    auto struct_type = std::dynamic_pointer_cast<StructTypeAst>(obj_type);

    if (!struct_type) {
        push_error(node->line, node->col,
            "Cannot access field of non-struct type '" + obj_type->to_string() + "'.");
        last_type = obj_type;
        return;
    }

    auto it = struct_table.find(struct_type->name);
    if (it == struct_table.end()) {
        push_error(node->line, node->col,
            "Unknown struct type: '" + struct_type->name + "'.");
        last_type = TypeAst::get_int();
        return;
    }

    for (auto& field : it->second) {
        if (field.name == node->field_name) {
            last_type = field.type;
            return;
        }
    }

    push_error(node->line, node->col,
        "Struct '" + struct_type->name + "' has no field '" + node->field_name + "'.");
    last_type = TypeAst::get_int();
}

// -------------------------------------------------------------------------
// Unary / Binary expressions
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(UnaryExprAst* node) {
    auto operand_type = get_type(node->operand.get());
    std::string type_str = operand_type->to_string();

    if (node->op == tok_not) {
        if (type_str != "bool") {
            push_error(node->line, node->col, "Operator 'not' requires a boolean operand.");
        }
        last_type = TypeAst::get_bool();
    } else if (node->op == '-') {
        if (type_str != "int" && type_str != "float") {
            push_error(node->line, node->col, "Unary minus requires a numeric operand (int or float).");
        }
        last_type = operand_type;
    } else {
        push_error(node->line, node->col, "Unknown unary operator.");
    }
}

void tc::TypeChecker::visit(BinaryExprAst* node) {
    auto left_type  = get_type(node->left.get());
    auto right_type = get_type(node->right.get());

    if (left_type->to_string() == "string" || right_type->to_string() == "string") {
        push_error(node->line, node->col, "String operations are not supported.");
        last_type = left_type;
        return;
    }

    if (node->op == tok_and || node->op == tok_or || node->op == tok_xor) {
        if (left_type->to_string() != "bool" || right_type->to_string() != "bool") {
            push_error(node->line, node->col,
                "Logical operators 'and', 'or', 'xor' require boolean operands.");
        }
        last_type = TypeAst::get_bool();
        return;
    }

    if (!is_same_type(left_type.get(), right_type.get())) {
        push_error(node->line, node->col, "Mismatched types in binary expression.");
        last_type = left_type;
        return;
    }

    bool is_relational = (node->op == tok_eq || node->op == tok_neq ||
                          node->op == '<' || node->op == '>' ||
                          node->op == tok_less_or_eq || node->op == tok_more_or_eq);

    if (is_relational) {
        last_type = TypeAst::get_bool();
    } else {
        if (left_type->to_string() == "bool") {
            push_error(node->line, node->col,
                "Mathematical operations require numeric types, but got 'bool'.");
        }
        last_type = left_type;
    }
}

// -------------------------------------------------------------------------
// Function call
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(FuncCallExprAst* node) {
    auto it = func_table.find(node->name);
    if (it == func_table.end()) {
        push_error(node->line, node->col,
            "Undefined function: '" + node->name + "'.");
        last_type = TypeAst::get_int();
        return;
    }

    auto& sig = it->second;
    if (node->args.size() != sig.param_types.size()) {
        push_error(node->line, node->col,
            "Function '" + node->name + "' expects " +
            std::to_string(sig.param_types.size()) + " argument(s), got " +
            std::to_string(node->args.size()) + ".");
    } else {
        for (int i = 0; i < (int)node->args.size(); i++) {
            auto arg_type = get_type(node->args[i].get());
            if (!is_same_type(arg_type.get(), sig.param_types[i].get())) {
                push_error(node->args[i]->line, node->args[i]->col,
                    "Argument " + std::to_string(i + 1) +
                    " type mismatch in call to '" + node->name + "': expected '" +
                    sig.param_types[i]->to_string() + "' but got '" +
                    arg_type->to_string() + "'.");
            }
        }
    }

    last_type = sig.return_type;
}

// -------------------------------------------------------------------------
// Struct literal
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(StructLiteralExprAst* node) {
    auto it = struct_table.find(node->struct_name);
    if (it == struct_table.end()) {
        push_error(node->line, node->col,
            "Unknown struct type: '" + node->struct_name + "'.");
        last_type = TypeAst::get_int();
        return;
    }

    auto& def_fields = it->second;

    // Check each declared field has a matching init
    for (auto& def_field : def_fields) {
        bool found = false;
        for (auto& [init_name, init_expr] : node->fields) {
            if (init_name == def_field.name) {
                found = true;
                auto expr_type = get_type(init_expr.get());
                if (!is_same_type(expr_type.get(), def_field.type.get())) {
                    push_error(node->line, node->col,
                        "Field '" + def_field.name + "' type mismatch in struct literal '" +
                        node->struct_name + "': expected '" + def_field.type->to_string() +
                        "' but got '" + expr_type->to_string() + "'.");
                }
                break;
            }
        }
        if (!found) {
            push_error(node->line, node->col,
                "Missing field '" + def_field.name + "' in struct literal '" +
                node->struct_name + "'.");
        }
    }

    last_type = std::make_shared<StructTypeAst>(node->struct_name);
}

// -------------------------------------------------------------------------
// Variable declaration
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(VarDeclStmtAst* node) {
    // Top-level globals are fully validated in Pass 1.5; skip them here.
    if (global_vars.count(node->name) && scopes.size() == 1) return;

    if (node->type->to_string() == "void") {
        push_error(node->line, node->col, "Cannot declare a variable of type 'void'.");
        return;
    }

    if (scopes.back().count(node->name) > 0) {
        push_error(node->line, node->col,
            "Variable '" + node->name + "' is already declared in this scope.");
        return;
    }

    auto declared_type  = node->type.get();
    auto expression_type = get_type(node->init_expr.get());

    bool types_match = is_same_type(declared_type, expression_type.get());

    if (!types_match) {
        // Allow scalar-fill initialisation for arrays: int[5] = 0
        auto arr_type = std::dynamic_pointer_cast<tc::ArrayTypeAst>(node->type);
        if (arr_type) {
            auto leaf_type = get_deep_base_type(declared_type);
            if (is_same_type(leaf_type, expression_type.get())) {
                types_match = true;
            }
        }
    }

    if (!types_match) {
        push_error(node->line, node->col,
            "Initialisation type '" + expression_type->to_string() +
            "' does not match declared type '" + declared_type->to_string() + "'.");
    }

    scopes.back()[node->name] = node->type;
}

// -------------------------------------------------------------------------
// Assignment
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(AssignmentStmtAst* node) {
    auto target_type = get_type(node->target.get());
    auto init_type   = get_type(node->init_expr.get());

    if (!is_same_type(target_type.get(), init_type.get())) {
        push_error(node->line, node->col,
            "Type mismatch in assignment: cannot assign '" + init_type->to_string() +
            "' to '" + target_type->to_string() + "'.");
    }
}

// -------------------------------------------------------------------------
// Print / Read
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(PrintStmtAst* node) {
    node->expr->accept(*this);
}

void tc::TypeChecker::visit(ReadStmtAst* node) {
    auto type = get_type(node->target.get());
    if (std::dynamic_pointer_cast<tc::ArrayTypeAst>(type)) {
        push_error(node->line, node->col, "Cannot read directly into an array type.");
    }
    if (!dynamic_cast<VariableExprAst*>(node->target.get()) &&
        !dynamic_cast<ArrayAccessExprAst*>(node->target.get())) {
        push_error(node->line, node->col, "Target of 'read' must be a variable or array element.");
    }
}

// -------------------------------------------------------------------------
// Block
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(BlockAst* node) {
    scopes.push_back({});
    for (auto& stmt : node->statements) {
        stmt->accept(*this);
    }
    scopes.pop_back();
}

// -------------------------------------------------------------------------
// While / Break
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(WhileStmtAst* node) {
    auto cond_type = get_type(node->condition.get());
    if (cond_type->to_string() != "bool") {
        push_error(node->line, node->col, "Condition in 'while' must be a boolean expression.");
    }
    loop_depth++;
    node->body->accept(*this);
    loop_depth--;
}

void tc::TypeChecker::visit(BreakStmtAst* node) {
    if (loop_depth == 0) {
        push_error(node->line, node->col, "'break' used outside of a loop.");
    }
}

// -------------------------------------------------------------------------
// If
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(IfStmtAst* node) {
    auto cond_type = get_type(node->condition.get());
    if (cond_type->to_string() != "bool") {
        push_error(node->line, node->col, "Condition in 'if' must be a boolean expression.");
    }
    node->then_block->accept(*this);
    if (node->else_block) node->else_block->accept(*this);
}

// -------------------------------------------------------------------------
// Function declaration / return / expr-stmt
// -------------------------------------------------------------------------

void tc::TypeChecker::check_func_body(FuncDeclStmtAst* node) {
    scopes.push_back({});
    for (auto& p : node->params)
        scopes.back()[p.name] = p.type;
    auto prev_return_type = current_return_type;
    current_return_type   = node->return_type;
    node->body->accept(*this);
    current_return_type = prev_return_type;
    scopes.pop_back();
}

void tc::TypeChecker::visit(FuncDeclStmtAst* node) {
    FuncSignature sig;
    for (auto& p : node->params) sig.param_types.push_back(p.type);
    sig.return_type = node->return_type;
    func_table[node->name] = sig;
    check_func_body(node);
}

void tc::TypeChecker::visit(MethodDeclStmtAst* node) {
    check_func_body(node);
}

void tc::TypeChecker::visit(ReturnStmtAst* node) {
    if (!current_return_type) {
        push_error(node->line, node->col, "'return' used outside of a function.");
        return;
    }

    bool is_void_return   = (node->expr == nullptr);
    bool expects_void     = (current_return_type->to_string() == "void");

    if (is_void_return && !expects_void) {
        push_error(node->line, node->col,
            "Function must return a value of type '" + current_return_type->to_string() + "'.");
    } else if (!is_void_return && expects_void) {
        push_error(node->line, node->col, "Void function cannot return a value.");
    } else if (!is_void_return) {
        auto expr_type = get_type(node->expr.get());
        if (!is_same_type(expr_type.get(), current_return_type.get())) {
            push_error(node->line, node->col,
                "Return type mismatch: expected '" + current_return_type->to_string() +
                "' but got '" + expr_type->to_string() + "'.");
        }
    }
}

void tc::TypeChecker::visit(ExprStmtAst* node) {
    node->expr->accept(*this); // evaluate for side-effects; result discarded
}

// -------------------------------------------------------------------------
// Struct declaration
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(StructDeclStmtAst* node) {
    // Registration was done in ProgramAst pass 1; here we validate field types and method bodies.

    // Check for duplicate field names and unknown struct field types.
    std::map<std::string, bool> seen_fields;
    for (auto& field : node->fields) {
        if (seen_fields.count(field.name)) {
            push_error(node->line, node->col,
                "Duplicate field '" + field.name + "' in type '" + node->name + "'.");
        } else {
            seen_fields[field.name] = true;
        }
        auto st = std::dynamic_pointer_cast<StructTypeAst>(field.type);
        if (st && struct_table.find(st->name) == struct_table.end()) {
            push_error(node->line, node->col,
                "Field '" + field.name + "' has unknown struct type '" + st->name + "'.");
        }
    }

    // Type-check each method body
    for (auto& m : node->methods) {
        m->accept(*this); // reuses visit(FuncDeclStmtAst*) which handles scopes & return type
    }
}

// -------------------------------------------------------------------------
// Method call expression
// -------------------------------------------------------------------------

void tc::TypeChecker::visit(MethodCallExprAst* node) {
    // Determine the struct type of the receiver
    auto obj_type    = get_type(node->object.get());
    auto struct_type = std::dynamic_pointer_cast<StructTypeAst>(obj_type);

    if (!struct_type) {
        push_error(node->line, node->col,
            "Method call on non-struct type '" + obj_type->to_string() + "'.");
        last_type = TypeAst::get_int();
        return;
    }

    // Look up method signature
    auto sm_it = struct_methods.find(struct_type->name);
    if (sm_it == struct_methods.end()) {
        push_error(node->line, node->col,
            "Struct '" + struct_type->name + "' has no methods.");
        last_type = TypeAst::get_int();
        return;
    }

    auto m_it = sm_it->second.find(node->method_name);
    if (m_it == sm_it->second.end()) {
        push_error(node->line, node->col,
            "Struct '" + struct_type->name + "' has no method '" + node->method_name + "'.");
        last_type = TypeAst::get_int();
        return;
    }

    auto& sig = m_it->second;
    // param_types[0] is 'self'; user-supplied args are params[1..]
    int expected_args = (int)sig.param_types.size() - 1;
    if ((int)node->args.size() != expected_args) {
        push_error(node->line, node->col,
            "Method '" + node->method_name + "' expects " +
            std::to_string(expected_args) + " argument(s), got " +
            std::to_string(node->args.size()) + ".");
    } else {
        for (int i = 0; i < expected_args; i++) {
            auto arg_type = get_type(node->args[i].get());
            if (!is_same_type(arg_type.get(), sig.param_types[i + 1].get())) {
                push_error(node->args[i]->line, node->args[i]->col,
                    "Argument " + std::to_string(i + 1) +
                    " type mismatch in call to '" + node->method_name + "': expected '" +
                    sig.param_types[i + 1]->to_string() + "' but got '" +
                    arg_type->to_string() + "'.");
            }
        }
    }

    last_type = sig.return_type;
}

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

std::shared_ptr<tc::TypeAst> tc::TypeChecker::get_type(ExprAst* node) {
    node->accept(*this);
    return last_type;
}

bool tc::TypeChecker::is_same_type(TypeAst* t1, TypeAst* t2) const {
    return t1->to_string() == t2->to_string();
}

void tc::TypeChecker::push_error(int line, int col, std::string content) {
    errors.emplace_back(line, col, content);
}

tc::TypeAst* tc::TypeChecker::get_deep_base_type(tc::TypeAst* type) const {
    auto current = type;
    while (auto arr = dynamic_cast<tc::ArrayTypeAst*>(current)) {
        current = arr->base_type.get();
    }
    return current;
}
