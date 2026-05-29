#pragma once
#include "Visitor.h"
#include <sstream>
#include <string>
#include <vector>

namespace tc {
    class MermaidVisitor : public Visitor {
    private:
        std::stringstream ss;
        int next_id = 0;
        std::string last_node_id;

        std::string create_node(const std::string& label);
        void connect(const std::string& parent, const std::string& child);

    public:
        MermaidVisitor() { ss << "graph TD\n"; }
        std::string get_code() const { return ss.str(); }

        // Expressions
        void visit(IntExprAst*          node) override;
        void visit(FloatExprAst*         node) override;
        void visit(StringExprAst*        node) override;
        void visit(BoolExprAst*          node) override;
        void visit(VariableExprAst*      node) override;
        void visit(ArrayAccessExprAst*   node) override;
        void visit(FieldAccessExprAst*   node) override;
        void visit(UnaryExprAst*         node) override;
        void visit(BinaryExprAst*        node) override;
        void visit(FuncCallExprAst*      node) override;
        void visit(StructLiteralExprAst* node) override;
        void visit(MethodCallExprAst*    node) override;

        // Statements
        void visit(VarDeclStmtAst*    node) override;
        void visit(AssignmentStmtAst* node) override;
        void visit(PrintStmtAst*      node) override;
        void visit(ReadStmtAst*       node) override;
        void visit(BlockAst*          node) override;
        void visit(WhileStmtAst*      node) override;
        void visit(BreakStmtAst*      node) override;
        void visit(IfStmtAst*         node) override;
        void visit(ReturnStmtAst*     node) override;
        void visit(FuncDeclStmtAst*   node) override;
        void visit(MethodDeclStmtAst* node) override;
        void visit(ExprStmtAst*       node) override;
        void visit(StructDeclStmtAst* node) override;

        // Program
        void visit(ProgramAst* node) override;
    };
}
