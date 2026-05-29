#pragma once

namespace tc {
    // Expression nodes
    class IntExprAst;
    class FloatExprAst;
    class StringExprAst;
    class BoolExprAst;
    class UnaryExprAst;
    class VariableExprAst;
    class BinaryExprAst;
    class ArrayAccessExprAst;
    class FieldAccessExprAst;
    class FuncCallExprAst;
    class StructLiteralExprAst;
    class MethodCallExprAst;

    // Statement nodes
    class VarDeclStmtAst;
    class AssignmentStmtAst;
    class PrintStmtAst;
    class ReadStmtAst;
    class BlockAst;
    class WhileStmtAst;
    class BreakStmtAst;
    class IfStmtAst;
    class ReturnStmtAst;
    class FuncDeclStmtAst;
    class MethodDeclStmtAst;
    class ExprStmtAst;
    class StructDeclStmtAst;

    // Program
    class ProgramAst;
}

namespace tc {
    class Visitor {
        public:
            virtual ~Visitor() = default;

            // Expressions
            virtual void visit(IntExprAst* node) = 0;
            virtual void visit(FloatExprAst* node) = 0;
            virtual void visit(StringExprAst* node) = 0;
            virtual void visit(BoolExprAst* node) = 0;
            virtual void visit(VariableExprAst* node) = 0;
            virtual void visit(ArrayAccessExprAst* node) = 0;
            virtual void visit(FieldAccessExprAst* node) = 0;
            virtual void visit(UnaryExprAst* node) = 0;
            virtual void visit(BinaryExprAst* node) = 0;
            virtual void visit(FuncCallExprAst* node) = 0;
            virtual void visit(StructLiteralExprAst* node) = 0;
            virtual void visit(MethodCallExprAst* node) = 0;

            // Statements
            virtual void visit(VarDeclStmtAst* node) = 0;
            virtual void visit(AssignmentStmtAst* node) = 0;
            virtual void visit(PrintStmtAst* node) = 0;
            virtual void visit(ReadStmtAst* node) = 0;
            virtual void visit(BlockAst* node) = 0;
            virtual void visit(WhileStmtAst* node) = 0;
            virtual void visit(BreakStmtAst* node) = 0;
            virtual void visit(IfStmtAst* node) = 0;
            virtual void visit(ReturnStmtAst* node) = 0;
            virtual void visit(FuncDeclStmtAst*   node) = 0;
            virtual void visit(MethodDeclStmtAst* node) = 0;
            virtual void visit(ExprStmtAst* node) = 0;
            virtual void visit(StructDeclStmtAst* node) = 0;

            // Program
            virtual void visit(ProgramAst* node) = 0;
    };
}
