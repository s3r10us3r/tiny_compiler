#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Lexer.h"
#include "Visitor.h"

namespace tc {
    struct TypeAst {
        public:
            virtual ~TypeAst() = default;
            virtual std::string to_string() const = 0;
            static std::shared_ptr<TypeAst> get_int();
            static std::shared_ptr<TypeAst> get_float();
    };

    struct IntTypeAst : public TypeAst {
        std::string to_string() const override {return "int";}
    };

    struct FloatTypeAst : public TypeAst {
        std::string to_string() const override {return "float";}
    };

    struct BaseAst {
        public:
            virtual void accept(Visitor& v) = 0;
    };

    struct ExprAst : public BaseAst {
        int line;
        int col;
        public:
            ExprAst(int line, int col) : line(line), col(col) {}
            virtual ~ExprAst() = default;
            virtual std::string dump(int indent = 0) const = 0;
    };

    inline std::string pad(int n) { return std::string(n * 2, ' '); }


    struct UnaryExprAst : public ExprAst {
        int op; // '-'
        std::unique_ptr<ExprAst> operand;

        UnaryExprAst(int line, int col, int op, std::unique_ptr<ExprAst> operand)
            : ExprAst(line, col), op(op), operand(std::move(operand)) {}

        void accept(Visitor& v) override { v.visit(this); }
        
        std::string dump(int indent) const override {
            return pad(indent) + "UnaryOp('" + (char)op + "')\n" + operand->dump(indent + 1);
        }
    };

    template <typename T>
    struct LiteralExprAst : public ExprAst {
        T val;
        public:
            LiteralExprAst(int line, int col, T val) : ExprAst(line, col), val(val) {}
    };

    struct IntExprAst : public LiteralExprAst<int> {
        public:
            IntExprAst(int line, int col, int val) : LiteralExprAst<int>(line, col, val) {}
            std::string dump(int indent = 0) const override {
                return pad(indent) + "IntLiteral(" + std::to_string(val) + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };

    struct FloatExprAst : public LiteralExprAst<double> {
        public:
            FloatExprAst(int line, int col, double val) : LiteralExprAst<double>(line, col, val) {}
            std::string dump(int indent = 0) const override {
                return pad(indent) + "FloatLiteral(" + std::to_string(val) + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };

    struct VariableExprAst : public ExprAst {
        std::string name;
        public:
            VariableExprAst(int line, int col, std::string name) : ExprAst(line, col), name(name) {}
            std::string dump(int indent = 0) const override {
                return pad(indent) + "Variable(" + name + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };

    struct BinaryExprAst : public ExprAst {
        int op;
        std::unique_ptr<ExprAst> left, right;
        public:
            BinaryExprAst(int line, int col, int op, std::unique_ptr<ExprAst> left, std::unique_ptr<ExprAst> right) :
                ExprAst(line, col), op(op), left(std::move(left)), right(std::move(right)) {}
            std::string dump(int indent) const override {
                std::string s = pad(indent) + "BinaryOp('" + (char)op + "') [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]:\n";
                s += left->dump(indent + 1) + "\n";
                s += right->dump(indent + 1);
                return s;
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };
    
    struct StmtAst : public BaseAst {
        int line;
        int col;
        public:
            StmtAst(int line, int col) : line(line), col(col) {}
            virtual ~StmtAst() = default;
            virtual std::string dump(int indent = 0) const = 0;
    };

    struct VarDeclStmtAst : public StmtAst {
        std::string name;
        std::shared_ptr<TypeAst> type;
        std::unique_ptr<ExprAst> init_expr;
        public:
            VarDeclStmtAst(int line, int col, std::string name, std::shared_ptr<TypeAst> type, std::unique_ptr<ExprAst> init_expr) 
                : StmtAst(line, col), name(name), type(type), init_expr(std::move(init_expr)) {}
            
            std::string dump(int indent) const override {
                std::string s = pad(indent) + "VarDecl(name: " + name + ", type: " + type->to_string() + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n";
                s += pad(indent + 1) + "Init:\n" + init_expr->dump(indent + 2);
                return s;
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };

    struct AssignmentStmtAst : public StmtAst {
        std::string name;
        std::unique_ptr<ExprAst> init_expr;
        public:
            AssignmentStmtAst(int line, int col, std::string name, std::unique_ptr<ExprAst> init_expr) 
                : StmtAst(line, col), name(name), init_expr(std::move(init_expr)) {}
            
            std::string dump(int indent) const override {
                std::string s = pad(indent) + "Assignment(name: " + name + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n";
                s += pad(indent + 1) + "Init:\n" + init_expr->dump(indent + 2);
                return s;
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };

    struct PrintStmtAst : public StmtAst {
        std::unique_ptr<ExprAst> expr;
        public:
            PrintStmtAst(int line, int col, std::unique_ptr<ExprAst> expr) 
                : StmtAst(line, col), expr(std::move(expr)) {}
            
            std::string dump(int indent) const override {
                return pad(indent) + "PRINT [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n" + expr->dump(indent + 1);
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };

    struct ReadStmtAst : public StmtAst {
        std::string name;
        public:
            ReadStmtAst(int line, int col, std::string name) 
                : StmtAst(line, col), name(name) {}
            
            std::string dump(int indent) const override {
                return pad(indent) + "READ " + name + " [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };

    struct ProgramAst : public BaseAst {
        std::vector<std::unique_ptr<StmtAst>> statements;
        public:
            void add_stmt(std::unique_ptr<StmtAst> stmt);
            std::string dump() const {
                std::string r = "";
                for (const auto& s : statements) {
                     r += s->dump(0) + "\n" + std::string(20, '-') + "\n";
                }
                return r;
            }
            void accept(Visitor& v) override {
                v.visit(this); 
            }
    };


    class Parser {
        TokenData cur_tok;
        ProgramAst program;
        Lexer lexer;
        public:
            Parser(Lexer lexer) : lexer(lexer) {}
            Parser(std::istream& in) : lexer(Lexer(in)) {}
            ProgramAst parse();
        private:
            std::unique_ptr<StmtAst> parse_stmt();

            std::unique_ptr<StmtAst> parse_var_decl();
            std::unique_ptr<StmtAst> parse_print();
            std::unique_ptr<StmtAst> parse_read();
            std::unique_ptr<StmtAst> parse_assgn();

            std::unique_ptr<ExprAst> parse_expr();
            std::unique_ptr<ExprAst> parse_term();
            std::unique_ptr<ExprAst> parse_unary();
            std::unique_ptr<ExprAst> parse_factor();

            std::shared_ptr<TypeAst> parse_type(std::string& type_str);
            void advance();
            void expect(int expected_type, std::string msg);
            [[noreturn]] void report_error(const std::string& message, const tc::TokenData& token);
    };
}
