#include "Codegen.h"
#include "Parser.h"
#include "Visitor.h"
#include <llvm/IR/Value.h>

namespace tc {
    class LLVMVisitor : public Visitor {
        CodegenContext codegen_context;
        llvm::Value* last_value;
        public:
            virtual void visit(IntExprAst* node) override;
            virtual void visit(FloatExprAst* node) override;
            virtual void visit(VariableExprAst* node) override;
            virtual void visit(UnaryExprAst* node) override;
            virtual void visit(BinaryExprAst* node) override;
            virtual void visit(VarDeclStmtAst* node) override;
            virtual void visit(AssignmentStmtAst* node) override;
            virtual void visit(PrintStmtAst* node) override;
            virtual void visit(ReadStmtAst* node) override;
            virtual void visit(ProgramAst* node) override;

            void dump_ir(llvm::raw_ostream& out) const;
        private:
            llvm::Type* get_llvm_type(TypeAst* type);
            llvm::Value* get_value(tc::ExprAst* node);
            [[noreturn]] void report_error(const std::string& message, int line);
    };
}
