#pragma once

#include "Ast.h"
#include "Lexer.h"

namespace tc {

    class Parser {
        TokenData cur_tok;
        ProgramAst program;
        Lexer lexer;
        public:
            Parser(Lexer lexer) : lexer(lexer) {}
            Parser(std::istream& in) : lexer(Lexer(in)) {}
            ProgramAst parse();
        private:
            // Statement parsers
            std::unique_ptr<StmtAst> parse_stmt();
            std::unique_ptr<StmtAst> parse_var_decl();
            std::unique_ptr<StmtAst> parse_print();
            std::unique_ptr<StmtAst> parse_read();
            std::unique_ptr<StmtAst> parse_assgn();
            std::unique_ptr<BlockAst> parse_block();
            std::unique_ptr<StmtAst> parse_while();
            std::unique_ptr<StmtAst> parse_if();
            std::unique_ptr<StmtAst> parse_break();
            std::unique_ptr<StmtAst> parse_func_decl();
            std::unique_ptr<StmtAst> parse_return();
            std::unique_ptr<StmtAst> parse_struct_decl();
            std::unique_ptr<MethodDeclStmtAst> parse_method_decl(const std::string& struct_name);

            // Expression parsers
            std::unique_ptr<ExprAst> parse_expr();
            std::unique_ptr<ExprAst> parse_logical_and();
            std::unique_ptr<ExprAst> parse_relational();
            std::unique_ptr<ExprAst> parse_math_expr();
            std::unique_ptr<ExprAst> parse_logical_xor();
            std::unique_ptr<ExprAst> parse_term();
            std::unique_ptr<ExprAst> parse_unary();
            std::unique_ptr<ExprAst> parse_factor();
            std::unique_ptr<ExprAst> parse_var_expr();
            std::unique_ptr<ExprAst> parse_postfix(std::unique_ptr<ExprAst> node);
            std::unique_ptr<ExprAst> parse_func_call(const std::string& name, int line, int col);
            std::unique_ptr<ExprAst> parse_struct_literal(const std::string& name, int line, int col);

            // Type parsers
            std::shared_ptr<TypeAst> parse_type();
            std::shared_ptr<TypeAst> parse_base_type(std::string& type_str);

            // Helpers
            void advance();
            void expect(int expected_type, std::string msg);
            void expect_identifier(const std::string& context);
            [[noreturn]] void report_error(const std::string& message, const tc::TokenData& token);
    };
}
