#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "Visitor.h"

namespace tc {

    // -------------------------------------------------------------------------
    // Type nodes
    // -------------------------------------------------------------------------

    struct TypeAst {
        public:
            virtual ~TypeAst() = default;
            virtual std::string to_string() const = 0;
            static std::shared_ptr<TypeAst> get_int();
            static std::shared_ptr<TypeAst> get_float();
            static std::shared_ptr<TypeAst> get_string();
            static std::shared_ptr<TypeAst> get_bool();
            static std::shared_ptr<TypeAst> get_void();
    };

    struct IntTypeAst : public TypeAst {
        std::string to_string() const override { return "int"; }
    };

    struct FloatTypeAst : public TypeAst {
        std::string to_string() const override { return "float"; }
    };

    struct StringTypeAst : public TypeAst {
        std::string to_string() const override { return "string"; }
    };

    struct BoolTypeAst : public TypeAst {
        std::string to_string() const override { return "bool"; }
    };

    struct VoidTypeAst : public TypeAst {
        std::string to_string() const override { return "void"; }
    };

    struct StructTypeAst : public TypeAst {
        std::string name;
        StructTypeAst(std::string name) : name(std::move(name)) {}
        std::string to_string() const override { return name; }
    };

    struct ArrayTypeAst : public TypeAst {
        unsigned int size;
        std::shared_ptr<TypeAst> base_type;

        ArrayTypeAst(unsigned int size, std::shared_ptr<TypeAst> base_type)
            : size(size), base_type(base_type) {}

        std::string to_string() const override {
            return base_type->to_string() + "[" + std::to_string(size) + "]";
        }
    };

    // Helpers
    struct FuncParam {
        std::string name;
        std::shared_ptr<TypeAst> type;
    };

    struct StructField {
        std::string name;
        std::shared_ptr<TypeAst> type;
    };

    // Base AST nodes
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

    // -------------------------------------------------------------------------
    // Expression nodes
    // -------------------------------------------------------------------------

    struct UnaryExprAst : public ExprAst {
        int op; // '-' or tok_not
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
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct FloatExprAst : public LiteralExprAst<double> {
        public:
            FloatExprAst(int line, int col, double val) : LiteralExprAst<double>(line, col, val) {}
            std::string dump(int indent = 0) const override {
                return pad(indent) + "FloatLiteral(" + std::to_string(val) + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct StringExprAst : public LiteralExprAst<std::string> {
        public:
            StringExprAst(int line, int col, std::string val)
                : LiteralExprAst<std::string>(line, col, std::move(val)) {}

            std::string dump(int indent = 0) const override {
                return pad(indent) + "StringLiteral(\"" + val + "\") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }

            void accept(Visitor& v) override { v.visit(this); }
    };

    struct BoolExprAst : public LiteralExprAst<bool> {
        public:
            BoolExprAst(int line, int col, bool val) : LiteralExprAst<bool>(line, col, val) {}
            std::string dump(int indent = 0) const override {
                return pad(indent) + "BoolLiteral(" + (val ? "true" : "false") + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct VariableExprAst : public ExprAst {
        std::string name;
        public:
            VariableExprAst(int line, int col, std::string name)
                : ExprAst(line, col), name(name) {}
            std::string dump(int indent = 0) const override {
                return pad(indent) + "Variable(" + name + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct ArrayAccessExprAst : public ExprAst {
        std::unique_ptr<ExprAst> target;
        std::unique_ptr<ExprAst> index;

        ArrayAccessExprAst(int line, int col, std::unique_ptr<ExprAst> target, std::unique_ptr<ExprAst> index)
            : ExprAst(line, col), target(std::move(target)), index(std::move(index)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "ArrayAccess [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n";
            s += pad(indent + 1) + "Target:\n" + target->dump(indent + 2) + "\n";
            s += pad(indent + 1) + "Index:\n" + index->dump(indent + 2);
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct FieldAccessExprAst : public ExprAst {
        std::unique_ptr<ExprAst> object;
        std::string field_name;

        FieldAccessExprAst(int line, int col, std::unique_ptr<ExprAst> object, std::string field_name)
            : ExprAst(line, col), object(std::move(object)), field_name(std::move(field_name)) {}

        std::string dump(int indent = 0) const override {
            return pad(indent) + "FieldAccess(." + field_name + ") [L:" + std::to_string(line) + "]\n"
                 + object->dump(indent + 1);
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct BinaryExprAst : public ExprAst {
        int op;
        std::unique_ptr<ExprAst> left, right;
        public:
            BinaryExprAst(int line, int col, int op, std::unique_ptr<ExprAst> left, std::unique_ptr<ExprAst> right)
                : ExprAst(line, col), op(op), left(std::move(left)), right(std::move(right)) {}
            std::string dump(int indent) const override {
                std::string label = op > 0
                    ? std::string(1, (char)op)
                    : "TOKEN(" + std::to_string(op) + ")";
                std::string s = pad(indent) + "BinaryOp('" + label + "') [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]:\n";
                s += left->dump(indent + 1) + "\n";
                s += right->dump(indent + 1);
                return s;
            }
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct FuncCallExprAst : public ExprAst {
        std::string name;
        std::vector<std::unique_ptr<ExprAst>> args;

        FuncCallExprAst(int line, int col, std::string name,
                        std::vector<std::unique_ptr<ExprAst>> args)
            : ExprAst(line, col), name(std::move(name)), args(std::move(args)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "FuncCall(" + name + ") [L:" + std::to_string(line) + "]\n";
            for (auto& a : args) s += a->dump(indent + 1) + "\n";
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct StructLiteralExprAst : public ExprAst {
        std::string struct_name;
        std::vector<std::pair<std::string, std::unique_ptr<ExprAst>>> fields;

        StructLiteralExprAst(int line, int col, std::string struct_name,
                             std::vector<std::pair<std::string, std::unique_ptr<ExprAst>>> fields)
            : ExprAst(line, col), struct_name(std::move(struct_name)), fields(std::move(fields)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "StructLiteral(" + struct_name + ") [L:" + std::to_string(line) + "]\n";
            for (auto& [n, e] : fields)
                s += pad(indent + 1) + "." + n + ":\n" + e->dump(indent + 2) + "\n";
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    // -------------------------------------------------------------------------
    // Statement nodes
    // -------------------------------------------------------------------------

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
            VarDeclStmtAst(int line, int col, std::string name,
                           std::shared_ptr<TypeAst> type, std::unique_ptr<ExprAst> init_expr)
                : StmtAst(line, col), name(name), type(type), init_expr(std::move(init_expr)) {}

            std::string dump(int indent) const override {
                std::string s = pad(indent) + "VarDecl(name: " + name + ", type: " + type->to_string() + ") [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n";
                s += pad(indent + 1) + "Init:\n" + init_expr->dump(indent + 2);
                return s;
            }
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct AssignmentStmtAst : public StmtAst {
        std::unique_ptr<ExprAst> target;
        std::unique_ptr<ExprAst> init_expr;
        public:
            AssignmentStmtAst(int line, int col, std::unique_ptr<ExprAst> target,
                              std::unique_ptr<ExprAst> init_expr)
                : StmtAst(line, col), target(std::move(target)), init_expr(std::move(init_expr)) {}

            std::string dump(int indent = 0) const override {
                std::string s = pad(indent) + "Assignment [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n";
                s += pad(indent + 1) + "Target:\n" + target->dump(indent + 2) + "\n";
                s += pad(indent + 1) + "Init:\n" + init_expr->dump(indent + 2);
                return s;
            }

            void accept(Visitor& v) override { v.visit(this); }
    };

    struct PrintStmtAst : public StmtAst {
        std::unique_ptr<ExprAst> expr;
        public:
            PrintStmtAst(int line, int col, std::unique_ptr<ExprAst> expr)
                : StmtAst(line, col), expr(std::move(expr)) {}

            std::string dump(int indent) const override {
                return pad(indent) + "PRINT [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n" + expr->dump(indent + 1);
            }
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct ReadStmtAst : public StmtAst {
        std::unique_ptr<ExprAst> target;
        public:
            ReadStmtAst(int line, int col, std::unique_ptr<ExprAst> target)
                : StmtAst(line, col), target(std::move(target)) {}

            std::string dump(int indent) const override {
                return pad(indent) + "READ " + target->dump() + " [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]";
            }
            void accept(Visitor& v) override { v.visit(this); }
    };

    struct BlockAst : public StmtAst {
        std::vector<std::unique_ptr<StmtAst>> statements;
        public:
            BlockAst(int line, int col) : StmtAst(line, col) {}
            void push_stmt(std::unique_ptr<StmtAst> stmt) { statements.push_back(std::move(stmt)); }

            std::string dump(int indent) const override {
                std::string r = pad(indent) + "{\n";
                for (const auto& s : statements)
                    r += s->dump(indent + 1) + "\n";
                r += pad(indent) + "}";
                return r;
            }

            void accept(Visitor& v) override { v.visit(this); }
    };

    struct WhileStmtAst : public StmtAst {
        std::unique_ptr<ExprAst>  condition;
        std::unique_ptr<BlockAst> body;

        WhileStmtAst(int line, int col, std::unique_ptr<ExprAst> condition, std::unique_ptr<BlockAst> body)
            : StmtAst(line, col), condition(std::move(condition)), body(std::move(body)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "WHILE [L:" + std::to_string(line) + ", C:" + std::to_string(col) + "]\n";
            s += pad(indent + 1) + "Condition:\n" + condition->dump(indent + 2) + "\n";
            s += pad(indent + 1) + "Body:\n" + body->dump(indent + 2);
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct BreakStmtAst : public StmtAst {
        BreakStmtAst(int line, int col) : StmtAst(line, col) {}

        std::string dump(int indent = 0) const override {
            return pad(indent) + "BREAK [L:" + std::to_string(line) + "]";
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct IfStmtAst : public StmtAst {
        std::unique_ptr<ExprAst>  condition;
        std::unique_ptr<BlockAst> then_block;
        std::unique_ptr<StmtAst>  else_block; // nullptr if no else; BlockAst or IfStmtAst (else-if)

        IfStmtAst(int line, int col,
                  std::unique_ptr<ExprAst>  condition,
                  std::unique_ptr<BlockAst> then_block,
                  std::unique_ptr<StmtAst>  else_block)
            : StmtAst(line, col),
              condition(std::move(condition)),
              then_block(std::move(then_block)),
              else_block(std::move(else_block)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "IF [L:" + std::to_string(line) + "]\n";
            s += pad(indent + 1) + "Cond:\n" + condition->dump(indent + 2) + "\n";
            s += pad(indent + 1) + "Then:\n" + then_block->dump(indent + 2);
            if (else_block)
                s += "\n" + pad(indent + 1) + "Else:\n" + else_block->dump(indent + 2);
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct ReturnStmtAst : public StmtAst {
        std::unique_ptr<ExprAst> expr; // nullptr for void return

        ReturnStmtAst(int line, int col, std::unique_ptr<ExprAst> expr)
            : StmtAst(line, col), expr(std::move(expr)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "RETURN [L:" + std::to_string(line) + "]";
            if (expr) s += "\n" + expr->dump(indent + 1);
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct FuncDeclStmtAst : public StmtAst {
        std::string              name;
        std::vector<FuncParam>   params;
        std::shared_ptr<TypeAst> return_type;
        std::unique_ptr<BlockAst> body;

        FuncDeclStmtAst(int line, int col,
                        std::string name,
                        std::vector<FuncParam> params,
                        std::shared_ptr<TypeAst> return_type,
                        std::unique_ptr<BlockAst> body)
            : StmtAst(line, col),
              name(std::move(name)),
              params(std::move(params)),
              return_type(std::move(return_type)),
              body(std::move(body)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "FuncDecl(" + name + ") -> " + return_type->to_string() + " [L:" + std::to_string(line) + "]\n";
            for (auto& p : params)
                s += pad(indent + 1) + p.name + ": " + p.type->to_string() + "\n";
            s += body->dump(indent + 1);
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct MethodDeclStmtAst : public FuncDeclStmtAst {
        std::string owner_struct;

        MethodDeclStmtAst(int line, int col,
                          std::string name,
                          std::vector<FuncParam> params,
                          std::shared_ptr<TypeAst> return_type,
                          std::unique_ptr<BlockAst> body,
                          std::string owner_struct)
            : FuncDeclStmtAst(line, col, std::move(name), std::move(params),
                               std::move(return_type), std::move(body)),
              owner_struct(std::move(owner_struct)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "Method(" + owner_struct + "::" + name + ") -> " + return_type->to_string() + " [L:" + std::to_string(line) + "]\n";
            for (auto& p : params)
                s += pad(indent + 1) + p.name + ": " + p.type->to_string() + "\n";
            s += body->dump(indent + 1);
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    // obj.method(args)
    struct MethodCallExprAst : public ExprAst {
        std::unique_ptr<ExprAst> object;
        std::string method_name;
        std::vector<std::unique_ptr<ExprAst>> args; // does NOT include self

        MethodCallExprAst(int line, int col,
                          std::unique_ptr<ExprAst> object,
                          std::string method_name,
                          std::vector<std::unique_ptr<ExprAst>> args)
            : ExprAst(line, col),
              object(std::move(object)),
              method_name(std::move(method_name)),
              args(std::move(args)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "MethodCall(." + method_name + ") [L:" + std::to_string(line) + "]\n";
            s += pad(indent + 1) + "Object:\n" + object->dump(indent + 2) + "\n";
            for (auto& a : args) s += a->dump(indent + 1) + "\n";
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct ExprStmtAst : public StmtAst {
        std::unique_ptr<ExprAst> expr;

        ExprStmtAst(int line, int col, std::unique_ptr<ExprAst> expr)
            : StmtAst(line, col), expr(std::move(expr)) {}

        std::string dump(int indent = 0) const override {
            return pad(indent) + "ExprStmt [L:" + std::to_string(line) + "]\n"
                 + expr->dump(indent + 1);
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    struct StructDeclStmtAst : public StmtAst {
        std::string name;
        std::vector<StructField> fields;
        std::vector<std::unique_ptr<MethodDeclStmtAst>> methods;

        StructDeclStmtAst(int line, int col,
                          std::string name,
                          std::vector<StructField> fields,
                          std::vector<std::unique_ptr<MethodDeclStmtAst>> methods = {})
            : StmtAst(line, col),
              name(std::move(name)),
              fields(std::move(fields)),
              methods(std::move(methods)) {}

        std::string dump(int indent = 0) const override {
            std::string s = pad(indent) + "StructDecl(" + name + ") [L:" + std::to_string(line) + "]\n";
            for (auto& f : fields)
                s += pad(indent + 1) + f.name + ": " + f.type->to_string() + "\n";
            for (auto& m : methods)
                s += m->dump(indent + 1) + "\n";
            return s;
        }

        void accept(Visitor& v) override { v.visit(this); }
    };

    // -------------------------------------------------------------------------
    // Program node
    // -------------------------------------------------------------------------

    struct ProgramAst : public BaseAst {
        std::vector<std::unique_ptr<StmtAst>> statements;
        public:
            void add_stmt(std::unique_ptr<StmtAst> stmt);
            std::string dump() const {
                std::string r = "";
                for (const auto& s : statements)
                    r += s->dump(0) + "\n" + std::string(20, '-') + "\n";
                return r;
            }

            void accept(Visitor& v) override { v.visit(this); }
    };

}
