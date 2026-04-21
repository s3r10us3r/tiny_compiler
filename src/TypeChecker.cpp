#include "TypeChecker.h"
#include "Parser.h"
#include <string>

void tc::TypeChecker::visit(ProgramAst* node) {
    for (auto &stmt : node->statements) {
        stmt->accept(*this);
    }
}

void tc::TypeChecker::visit(ReadStmtAst* node) {
    auto name = node->name;
    if (symbol_table.count(node->name) <= 0) {
        push_error(node->line, node->col, "Undeclared variable: '" + name + "'");
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

void tc::TypeChecker::visit(FloatExprAst* node) {
    last_type = tc::TypeAst::get_float();
}

void tc::TypeChecker::visit(BinaryExprAst* node) {
    auto left_type = get_type(node->left.get());
    auto right_type = get_type(node->right.get());

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
    if (!is_same_type(declared_type, expression_type.get())) {
        push_error(node->line, node->col, "Initialization type different than declared.");
    }
    symbol_table[node->name] = node->type;
}

void tc::TypeChecker::visit(AssignmentStmtAst* node) {
    if (symbol_table.count(node->name) <= 0) {
        push_error(node->line, node->col, "Undeclared variable: '" + node->name + "'");
        return;
    }
    auto declared_type = symbol_table[node->name].get();
    auto expression_type = get_type(node->init_expr.get());
    if (!is_same_type(declared_type, expression_type.get())) {
        push_error(node->line, node->col, "Mismatched types.");
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

