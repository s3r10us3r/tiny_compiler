#pragma once

#include "Ast.h"
#include "Visitor.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tc {

    class SemanticError {
        std::string content;
        int line;
        int col;

        public:
            SemanticError(int line, int col, std::string content)
                : line(line), col(col), content(content) {}
            std::string get_msg() {
                return "[Line: " + std::to_string(line) + " Column: " + std::to_string(col) + "]: " + content;
            }
    };

    struct FuncSignature {
        std::vector<std::shared_ptr<TypeAst>> param_types;
        std::shared_ptr<TypeAst>              return_type; // VoidTypeAst for void
    };

    class TypeChecker : public Visitor {
        // Variable scopes (stack of name→type maps)
        std::vector<std::map<std::string, std::shared_ptr<TypeAst>>> scopes;

        // Function, struct, and global variable registries
        std::map<std::string, FuncSignature>       func_table;
        std::map<std::string, std::vector<StructField>> struct_table;
        // struct_methods[StructName][methodName] = FuncSignature (includes self as param[0])
        std::map<std::string, std::map<std::string, FuncSignature>> struct_methods;
        std::map<std::string, std::shared_ptr<TypeAst>> global_vars;

        // Accumulated errors
        std::vector<SemanticError> errors;

        // State for current expression type
        std::shared_ptr<TypeAst> last_type = nullptr;

        // State for control-flow checks
        int  loop_depth = 0;
        std::shared_ptr<TypeAst> current_return_type = nullptr; // null outside functions

        public:
            TypeChecker() { scopes.push_back({}); }

            // Expressions
            virtual void visit(IntExprAst*          node) override;
            virtual void visit(FloatExprAst*         node) override;
            virtual void visit(StringExprAst*        node) override;
            virtual void visit(BoolExprAst*          node) override;
            virtual void visit(VariableExprAst*      node) override;
            virtual void visit(ArrayAccessExprAst*   node) override;
            virtual void visit(FieldAccessExprAst*   node) override;
            virtual void visit(UnaryExprAst*         node) override;
            virtual void visit(BinaryExprAst*        node) override;
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

            const std::vector<SemanticError>& get_errors() { return errors; }

        private:
            std::shared_ptr<TypeAst> get_type(ExprAst* node);
            void push_error(int line, int col, std::string content);
            bool is_same_type(TypeAst* t1, TypeAst* t2) const;
            TypeAst* get_deep_base_type(TypeAst* type) const;
            void check_func_body(FuncDeclStmtAst* node);
    };
}
