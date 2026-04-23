#include "TypeChecker.h"
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
    if (symbol_table.count(node->name) <= 0) {
        push_error(node->line, node->col, "Undeclared variable: '" + name + "'");
    }
    last_type = symbol_table[node->name];
}

void tc::TypeChecker::visit(UnaryExprAst* node) {
    last_type = get_type(node->operand.get());
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

    if (!is_same_type(left_type.get(), right_type.get())) {
        push_error(node->line, node->col, "Mismatched types.");
        last_type = left_type; 
        return;
    }

    last_type = left_type;
}

void tc::TypeChecker::visit(VarDeclStmtAst* node) {
    if (symbol_table.count(node->name) > 0) {
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

    symbol_table[node->name] = node->type;
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

