#include "MermaidVisitor.h"
#include "Parser.h" 

std::string tc::MermaidVisitor::create_node(const std::string& label) {
    std::string id = "n" + std::to_string(next_id++);
    ss << "  " << id << "[\"" << label << "\"]\n";
    return id;
}

void tc::MermaidVisitor::connect(const std::string& parent, const std::string& child) {
    ss << "  " << parent << " --> " << child << "\n";
}

// --- Wyrażenia (Liście) ---

void tc::MermaidVisitor::visit(IntExprAst* node) {
    last_node_id = create_node("Int: " + std::to_string(node->val));
}

void tc::MermaidVisitor::visit(FloatExprAst* node) {
    last_node_id = create_node("Float: " + std::to_string(node->val));
}

void tc::MermaidVisitor::visit(StringExprAst* node) {
    last_node_id = create_node("String: " + node->val);
}

void tc::MermaidVisitor::visit(BoolExprAst* node) {
    last_node_id = create_node(node->val ? "bool: true" : "bool: false");
}

void tc::MermaidVisitor::visit(VariableExprAst* node) {
    last_node_id = create_node("ID: " + node->name);
}

// --- Wyrażenia (Złożone) ---

void tc::MermaidVisitor::visit(UnaryExprAst* node) {
    std::string my_id = create_node("Unary: " + std::string(1, (char)node->op));
    node->operand->accept(*this);
    connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(BinaryExprAst* node) {
    std::string label = "Binary: ";
    if (node->op > 0) label += (char)node->op; else label += "TOKEN(" + std::to_string(node->op) + ")";
    
    std::string my_id = create_node(label);
    
    node->left->accept(*this);
    connect(my_id, last_node_id);
    
    node->right->accept(*this);
    connect(my_id, last_node_id);
    
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(ArrayAccessExprAst* node) {
    std::string my_id = create_node("Array Access");
    node->target->accept(*this);
    connect(my_id, last_node_id);
    node->index->accept(*this);
    connect(my_id, last_node_id);
    last_node_id = my_id;
}

// --- Instrukcje ---

void tc::MermaidVisitor::visit(VarDeclStmtAst* node) {
    std::string my_id = create_node("VarDecl: " + node->name + " (" + node->type->to_string() + ")");
    node->init_expr->accept(*this);
    connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(AssignmentStmtAst* node) {
    std::string my_id = create_node("Assignment");
    node->target->accept(*this);
    connect(my_id, last_node_id);
    node->init_expr->accept(*this);
    connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(WhileStmtAst* node) {
    std::string my_id = create_node("WHILE");
    node->condition->accept(*this);
    connect(my_id, last_node_id);
    node->body->accept(*this);
    connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(PrintStmtAst* node) {
    std::string my_id = create_node("PRINT");
    node->expr->accept(*this);
    connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(ReadStmtAst* node) {
    std::string my_id = create_node("READ");
    node->target->accept(*this);
    connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(BlockAst* node) {
    std::string my_id = create_node("Block {}");
    for (auto& s : node->statements) {
        s->accept(*this);
        connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(ProgramAst* node) {
    std::string my_id = create_node("PROGRAM");
    for (auto& s : node->statements) {
        s->accept(*this);
        connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}
