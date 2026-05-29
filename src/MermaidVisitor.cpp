#include "MermaidVisitor.h"
#include "Ast.h"

std::string tc::MermaidVisitor::create_node(const std::string& label) {
    std::string id = "n" + std::to_string(next_id++);
    // Escape quotes inside labels
    std::string safe_label;
    for (char c : label) {
        if (c == '"') safe_label += "&quot;";
        else          safe_label += c;
    }
    ss << "  " << id << "[\"" << safe_label << "\"]\n";
    return id;
}

void tc::MermaidVisitor::connect(const std::string& parent, const std::string& child) {
    ss << "  " << parent << " --> " << child << "\n";
}

// -------------------------------------------------------------------------
// Literal expressions
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// Variable / access expressions
// -------------------------------------------------------------------------

void tc::MermaidVisitor::visit(VariableExprAst* node) {
    last_node_id = create_node("ID: " + node->name);
}

void tc::MermaidVisitor::visit(ArrayAccessExprAst* node) {
    std::string my_id = create_node("Array Access []");
    node->target->accept(*this); connect(my_id, last_node_id);
    node->index->accept(*this);  connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(FieldAccessExprAst* node) {
    std::string my_id = create_node("Field: ." + node->field_name);
    node->object->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

// -------------------------------------------------------------------------
// Operator expressions
// -------------------------------------------------------------------------

void tc::MermaidVisitor::visit(UnaryExprAst* node) {
    std::string label = "Unary: ";
    if (node->op > 0) label += (char)node->op;
    else              label += "TOKEN(" + std::to_string(node->op) + ")";

    std::string my_id = create_node(label);
    node->operand->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(BinaryExprAst* node) {
    std::string label = "Binary: ";
    if (node->op > 0) label += (char)node->op;
    else              label += "TOKEN(" + std::to_string(node->op) + ")";

    std::string my_id = create_node(label);
    node->left->accept(*this);  connect(my_id, last_node_id);
    node->right->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

// -------------------------------------------------------------------------
// Function call / struct literal
// -------------------------------------------------------------------------

void tc::MermaidVisitor::visit(FuncCallExprAst* node) {
    std::string my_id = create_node("FuncCall: " + node->name + "()");
    for (auto& a : node->args) {
        a->accept(*this); connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(StructLiteralExprAst* node) {
    std::string my_id = create_node("StructLiteral: " + node->struct_name);
    for (auto& [fname, fexpr] : node->fields) {
        std::string field_id = create_node("." + fname);
        fexpr->accept(*this); connect(field_id, last_node_id);
        connect(my_id, field_id);
    }
    last_node_id = my_id;
}

// -------------------------------------------------------------------------
// Statements
// -------------------------------------------------------------------------

void tc::MermaidVisitor::visit(VarDeclStmtAst* node) {
    std::string my_id = create_node("VarDecl: " + node->name + " (" + node->type->to_string() + ")");
    node->init_expr->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(AssignmentStmtAst* node) {
    std::string my_id = create_node("Assignment");
    node->target->accept(*this);   connect(my_id, last_node_id);
    node->init_expr->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(PrintStmtAst* node) {
    std::string my_id = create_node("PRINT");
    node->expr->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(ReadStmtAst* node) {
    std::string my_id = create_node("READ");
    node->target->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(BlockAst* node) {
    std::string my_id = create_node("Block {}");
    for (auto& s : node->statements) {
        s->accept(*this); connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(WhileStmtAst* node) {
    std::string my_id = create_node("WHILE");
    node->condition->accept(*this); connect(my_id, last_node_id);
    node->body->accept(*this);      connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(BreakStmtAst* node) {
    last_node_id = create_node("BREAK");
}

void tc::MermaidVisitor::visit(IfStmtAst* node) {
    std::string my_id = create_node("IF");
    node->condition->accept(*this);  connect(my_id, last_node_id);
    node->then_block->accept(*this); connect(my_id, last_node_id);
    if (node->else_block) {
        node->else_block->accept(*this); connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(ReturnStmtAst* node) {
    std::string my_id = create_node("RETURN");
    if (node->expr) {
        node->expr->accept(*this); connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(FuncDeclStmtAst* node) {
    std::string my_id = create_node("FuncDecl: " + node->name + "() -> " + node->return_type->to_string());
    for (auto& p : node->params) {
        std::string param_id = create_node("param: " + p.name + ": " + p.type->to_string());
        connect(my_id, param_id);
    }
    node->body->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(MethodDeclStmtAst* node) {
    std::string my_id = create_node("Method: " + node->owner_struct + "::" + node->name + "() -> " + node->return_type->to_string());
    for (auto& p : node->params) {
        std::string param_id = create_node("param: " + p.name + ": " + p.type->to_string());
        connect(my_id, param_id);
    }
    node->body->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(ExprStmtAst* node) {
    std::string my_id = create_node("ExprStmt");
    node->expr->accept(*this); connect(my_id, last_node_id);
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(MethodCallExprAst* node) {
    std::string my_id = create_node("MethodCall: ." + node->method_name + "()");
    node->object->accept(*this); connect(my_id, last_node_id);
    for (auto& a : node->args) {
        a->accept(*this); connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}

void tc::MermaidVisitor::visit(StructDeclStmtAst* node) {
    std::string my_id = create_node("StructDecl: " + node->name);
    for (auto& f : node->fields) {
        std::string fid = create_node("field: " + f.name + ": " + f.type->to_string());
        connect(my_id, fid);
    }
    for (auto& m : node->methods) {
        m->accept(*this); connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}

// -------------------------------------------------------------------------
// Program
// -------------------------------------------------------------------------

void tc::MermaidVisitor::visit(ProgramAst* node) {
    std::string my_id = create_node("PROGRAM");
    for (auto& s : node->statements) {
        s->accept(*this); connect(my_id, last_node_id);
    }
    last_node_id = my_id;
}
