#include "Parser.h"
#include "Visitor.h"
#include <llvm/IR/Value.h>
#include <map>
#include <vector>

namespace tc {
    class SemanticError {
        std::string content;
        int line;
        int col;

        public:
            SemanticError(int line, int col, std::string content) : line(line), col(col), content(content) {}
            std::string get_msg() {
                return "[Line: " + std::to_string(line) + " Column: " + std::to_string(col) + "]: " + content;
            };
    };

    class TypeChecker : public Visitor {
        std::map<std::string, std::shared_ptr<TypeAst>> symbol_table;
        std::vector<SemanticError> errors;
        std::shared_ptr<TypeAst> last_type = nullptr;
        public:
            virtual void visit(IntExprAst* node) override;
            virtual void visit(FloatExprAst* node) override;
            virtual void visit(VariableExprAst* node) override;
            virtual void visit(BinaryExprAst* node) override;
            virtual void visit(VarDeclStmtAst* node) override;
            virtual void visit(AssignmentStmtAst* node) override;
            virtual void visit(PrintStmtAst* node) override;
            virtual void visit(ReadStmtAst* node) override;
            virtual void visit(ProgramAst* node) override;
            virtual void visit(UnaryExprAst* node) override;

            const std::vector<SemanticError>& get_errors() { return errors; }
        private:
            std::shared_ptr<TypeAst> get_type(ExprAst* node);
            void push_error(int line, int col, std::string content);
            bool is_same_type(TypeAst* t1, TypeAst* t2) const;
    };
}
