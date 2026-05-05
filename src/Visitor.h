#pragma once

namespace tc {
    class IntExprAst;
    class FloatExprAst;
    class StringExprAst;
    class UnaryExprAst;
    class VariableExprAst;
    class BinaryExprAst;
    class VarDeclStmtAst;
    class AssignmentStmtAst;
    class PrintStmtAst;
    class ReadStmtAst;
    class ArrayAccessExprAst;
    class ProgramAst;
    class BlockAst;
    class BoolExprAst;
    class WhileStmtAst;
}

namespace tc {
    class Visitor {
        public:
            virtual ~Visitor() = default;

            virtual void visit(IntExprAst* node) = 0;
            virtual void visit(FloatExprAst* node) = 0;
            virtual void visit(StringExprAst* node) = 0;
            virtual void visit(BoolExprAst* node) = 0;
            virtual void visit(VariableExprAst* node) = 0;
            virtual void visit(ArrayAccessExprAst* node) = 0;
            virtual void visit(UnaryExprAst* node) = 0;
            virtual void visit(BinaryExprAst* node) = 0;

            virtual void visit(VarDeclStmtAst* node) = 0;
            virtual void visit(WhileStmtAst* node) = 0;

            virtual void visit(AssignmentStmtAst* node) = 0;
            virtual void visit(PrintStmtAst* node) = 0;
            virtual void visit(ReadStmtAst* node) = 0;
            virtual void visit(BlockAst* node) = 0;
            virtual void visit(ProgramAst* node) = 0;
    };
}
