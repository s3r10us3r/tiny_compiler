#include "TypeChecker.h"
#include "Lexer.h"
#include "Parser.h"
#include <string>

void tc::TypeChecker::visit(ProgramAst* node) {
    for (auto &stmt : node->statements) {
        stmt->accept(*this);
    }
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

void tc::TypeChecker::visit(VariableExprAst* node) {
    auto name = node->name;
    for (int i = scopes.size() - 1; i >= 0; i--) {
        if (scopes[i].count(node->name) > 0) {
            last_type = scopes[i][node->name];
            return;
        }
    }
    push_error(node->line, node->col, "Undeclared variable: '" + name + "'");
}

void tc::TypeChecker::visit(WhileStmtAst* node) {
    auto cond_type = get_type(node->condition.get());
    if (cond_type->to_string() != "bool") {
        push_error(node->line, node->col, "Condition in 'while' loop must be a boolean.");
    }
    node->body->accept(*this); 
}

void tc::TypeChecker::visit(UnaryExprAst* node) {
    auto operand_type = get_type(node->operand.get());
    std::string type_str = operand_type->to_string();

    if (node->op == tok_not) {
        if (type_str != "bool") {
            push_error(node->line, node->col, "Operator 'not' requires a boolean operand.");
        }
        last_type = TypeAst::get_bool();
    }
    else if (node->op == '-') {
        if (type_str != "int" && type_str != "float") {
            push_error(node->line, node->col, "Unary minus requires a numeric operand (int or float).");
        }
        last_type = operand_type;
    }
    else {
        push_error(node->line, node->col, "Unknown unary operator.");
    }
}

void tc::TypeChecker::visit(IntExprAst* node) {
    last_type = tc::TypeAst::get_int();
}

void tc::TypeChecker::visit(StringExprAst* node) {
    last_type = tc::TypeAst::get_string();
}

void tc::TypeChecker::visit(FloatExprAst* node) {
    last_type = tc::TypeAst::get_float();
}

void tc::TypeChecker::visit(BoolExprAst* node) {
    last_type = tc::TypeAst::get_bool();
}

void tc::TypeChecker::visit(tc::BlockAst* node) {
    scopes.push_back({});
    for (int i = 0; i < node->statements.size(); i++) {
        node->statements[i]->accept(*this);
    }
    scopes.pop_back();
}

void tc::TypeChecker::visit(tc::ArrayAccessExprAst* node) {
    auto target_type = get_type(node->target.get());
    auto array_type = std::dynamic_pointer_cast<ArrayTypeAst>(target_type);
    if (!array_type) {
        push_error(node->line, node->col, 
            "Type error: Cannot index into a non-array type ('" + target_type->to_string() + "').");
            
        last_type = target_type; 
        return;
    }

    auto index_type = get_type(node->index.get());
    if (index_type->to_string() != "int") {
        push_error(node->line, node->col, 
            "Type error: Array index must be an integer, but got '" + index_type->to_string() + "'.");
    }

    last_type = array_type->base_type;
}

void tc::TypeChecker::visit(BinaryExprAst* node) {
    auto left_type = get_type(node->left.get());
    auto right_type = get_type(node->right.get());

    if (left_type->to_string() == "string" || right_type->to_string() == "string") {
        push_error(node->line, node->col, "String operations are not implemented.");
        return;
    }

    if (node->op == tok_and || node->op == tok_or || node->op == tok_xor) {
        if (left_type->to_string() != "bool" || right_type->to_string() != "bool") {
            push_error(node->line, node->col, "Logical operators 'and' and 'or' require boolean operands.");
        }
        last_type = tc::TypeAst::get_bool();
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
        last_type = tc::TypeAst::get_bool();
    } else {
        if (left_type->to_string() == "bool" && left_type->to_string() != "string") {
            push_error(node->line, node->col, "Mathematical operations (+, -, *, /) require a number, but got '" + left_type->to_string() + "'.");
        }
        
        last_type = left_type;
    }
}

void tc::TypeChecker::visit(VarDeclStmtAst* node) {
    if (scopes.back().count(node->name) > 0) {
        push_error(node->line, node->col, "Variable '" + node->name + "' is already declared.");
        return; 
    }

    auto declared_type = node->type.get();
    auto expression_type = get_type(node->init_expr.get());

    bool types_match = is_same_type(declared_type, expression_type.get());

    if (!types_match) {
        auto arr_type = std::dynamic_pointer_cast<tc::ArrayTypeAst>(node->type);
        if (arr_type) {
            // Wyciągamy typ bazowy (np. int z int[5][10])
            auto leaf_type = get_deep_base_type(declared_type);
            
            // Jeśli skalar z prawej strony pasuje do liścia tablicy - jest git (fill)
            if (is_same_type(leaf_type, expression_type.get())) {
                types_match = true;
            }
        }
    }

    if (!types_match) {
        push_error(node->line, node->col, "Initialization type different than declared.");
    }

    scopes.back()[node->name] = node->type;
}

void tc::TypeChecker::visit(AssignmentStmtAst* node) {
    auto target_type = get_type(node->target.get());
    auto init_type = get_type(node->init_expr.get());

    if (!is_same_type(target_type.get(), init_type.get())) {
        push_error(node->line, node->col, "Type mismatch in assignment.");
    }
}

void tc::TypeChecker::visit(PrintStmtAst* node) {
    node->expr->accept(*this);
}

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

