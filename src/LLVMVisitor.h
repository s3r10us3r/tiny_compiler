#pragma once

#include "Codegen.h"
#include "Visitor.h"

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <map>
#include <string>
#include <vector>

namespace tc {
    class LLVMVisitor : public Visitor {
        CodegenContext codegen_context;
        llvm::Value*   last_value = nullptr;

        // Break targets: each while-loop pushes its "after" block here
        std::vector<llvm::BasicBlock*> break_target_stack;

        // Struct type registry
        std::map<std::string, llvm::StructType*>       struct_types;
        std::map<std::string, std::vector<StructField>> struct_field_defs;

        public:
            // Expressions
            virtual void visit(IntExprAst*          node) override;
            virtual void visit(FloatExprAst*         node) override;
            virtual void visit(BoolExprAst*          node) override;
            virtual void visit(StringExprAst*        node) override;
            virtual void visit(VariableExprAst*      node) override;
            virtual void visit(UnaryExprAst*         node) override;
            virtual void visit(BinaryExprAst*        node) override;
            virtual void visit(ArrayAccessExprAst*   node) override;
            virtual void visit(FieldAccessExprAst*   node) override;
            virtual void visit(FuncCallExprAst*      node) override;
            virtual void visit(StructLiteralExprAst* node) override;
            virtual void visit(MethodCallExprAst*    node) override;

            // Statements
            virtual void visit(VarDeclStmtAst*    node) override;
            virtual void visit(AssignmentStmtAst* node) override;
            virtual void visit(PrintStmtAst*      node) override;
            virtual void visit(ReadStmtAst*       node) override;
            virtual void visit(BlockAst*          node) override;
            virtual void visit(WhileStmtAst*      node) override;
            virtual void visit(BreakStmtAst*      node) override;
            virtual void visit(IfStmtAst*         node) override;
            virtual void visit(ReturnStmtAst*     node) override;
            virtual void visit(FuncDeclStmtAst*   node) override;
            virtual void visit(MethodDeclStmtAst* node) override;
            virtual void visit(ExprStmtAst*       node) override;
            virtual void visit(StructDeclStmtAst* node) override;

            // Program
            virtual void visit(ProgramAst* node) override;

            void dump_ir(llvm::raw_ostream& out) const;

        private:
            void emit_function(FuncDeclStmtAst* node, const std::string& llvm_name, bool has_self_ptr);
            void emit_array_init(llvm::Value* alloca, llvm::Type* llvm_type,
                                 llvm::Value* init_val, int total_elements);
            int  count_array_elements(tc::TypeAst* type);
            tc::PointerInfo get_pointer_info(tc::ExprAst* node);
            llvm::Type*  get_llvm_type(TypeAst* type);
            llvm::Value* get_value(tc::ExprAst* node);
            [[noreturn]] void report_error(const std::string& message, int line);
    };
}
