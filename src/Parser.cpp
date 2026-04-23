#include "Parser.h"
#include "Lexer.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>

std::shared_ptr<tc::TypeAst> tc::TypeAst::get_int() {
    static auto instance = std::make_shared<tc::IntTypeAst>();
    return instance;
}

std::shared_ptr<tc::TypeAst> tc::TypeAst::get_float() {
    static auto instance = std::make_shared<tc::FloatTypeAst>();
    return instance;
}

std::shared_ptr<tc::TypeAst> tc::TypeAst::get_string() {
    static auto instance = std::make_shared<tc::StringTypeAst>();
    return instance;
}

void tc::ProgramAst::add_stmt(std::unique_ptr<tc::StmtAst> stmt) {
    statements.push_back(std::move(stmt));
}

tc::ProgramAst tc::Parser::parse() {
    advance();
    while (cur_tok.type != tok_eof) {
        program.add_stmt(parse_stmt());
    }
    return std::move(program);
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_stmt() {
    std::unique_ptr<tc::StmtAst> stmt = nullptr;
    if (cur_tok.type == tok_let) {
        stmt = parse_var_decl();
    }
    else if (cur_tok.type == tok_read) {
        stmt = parse_read();
    }
    else if (cur_tok.type == tok_print) {
        stmt = parse_print();
    }
    else if (cur_tok.type == tok_id) {
        stmt = parse_assgn();
    }

    if (stmt == nullptr) {
        report_error("Unrecognized statement.", cur_tok);
    }

    expect(';', "';' expected");
    advance();
    return stmt;
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_var_decl() {
    expect(tok_let, "Expected 'let' at the beginning of variable declaration.");
    int line = cur_tok.line; int col = cur_tok.col;
    advance();
    expect(tok_id, "Expected identifier.");
    auto name = std::get<std::string>(cur_tok.value);
    advance();
    expect(':', "':' expected.");
    advance();
    expect(tok_type, "Type expected.");
    auto type = parse_type();
    expect('=', "'=' expected.");
    advance();
    auto expr = parse_expr();
    return std::make_unique<VarDeclStmtAst>(line, col, name, type, std::move(expr));
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_print() {
    expect(tok_print, "To nawet nie powinno sie móc wydarzyć");
    int line = cur_tok.line; int col = cur_tok.col;
    advance();
    auto expr = parse_expr();
    return std::make_unique<PrintStmtAst>(line, col, std::move(expr));
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_read() {
    expect(tok_read, "To nawet nie powinno sie móc wydarzyć");
    int line = cur_tok.line; int col = cur_tok.col;
    advance();
    auto target_expr = parse_var_expr();
    return std::make_unique<ReadStmtAst>(line, col, std::move(target_expr));
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_assgn() {
    int line = cur_tok.line; int col = cur_tok.col;
    auto target_expr = parse_var_expr();
    expect('=', "'=' expected");
    advance();
    auto expr = parse_expr();
    return std::make_unique<AssignmentStmtAst>(line, col, std::move(target_expr), std::move(expr));
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_expr() {
    int line = cur_tok.line; int col = cur_tok.col;
    auto left = parse_term();
    while (cur_tok.type == '+' || cur_tok.type == '-') {
        int op = cur_tok.type;
        advance();
        auto right = parse_term();
        left = std::make_unique<BinaryExprAst>(line, col, op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_term() {
    int line = cur_tok.line; int col = cur_tok.col;
    auto left = parse_unary();
    while (cur_tok.type == '*' || cur_tok.type == '/') {
        int op = cur_tok.type;
        advance();
        auto right = parse_unary();
        left = std::make_unique<BinaryExprAst>(line, col, op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_unary() {
    if (cur_tok.type == '-') {
        int line = cur_tok.line; 
        int col = cur_tok.col;
        advance();
        
        auto operand = parse_unary(); 
        return std::make_unique<UnaryExprAst>(line, col, '-', std::move(operand));
    }
    
    return parse_factor();
}


std::unique_ptr<tc::ExprAst> tc::Parser::parse_factor() {
    int line = cur_tok.line; int col = cur_tok.col;
    if (cur_tok.type == tok_str) {
        auto value = std::get<std::string>(cur_tok.value);
        advance();
        return std::make_unique<StringExprAst>(line, col, value);
    }
    if (cur_tok.type == tok_id) {
        return parse_var_expr();
    }
    if (cur_tok.type == tok_int_lit) {
        auto value = std::get<int>(cur_tok.value);
        advance();
        return std::make_unique<IntExprAst>(line, col, value);
    }
    if (cur_tok.type == tok_float_lit) {
        auto value = std::get<double>(cur_tok.value);
        advance();
        return std::make_unique<FloatExprAst>(line, col, value);
    }
    if (cur_tok.type == '(') {
        advance();
        auto expr = parse_expr();
        expect(')', "Expected closing bracket.");
        advance();
        return expr;
    }
    report_error("Unexpected token at the end of expression.", cur_tok);
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_var_expr() {
    int line = cur_tok.line;
    int col = cur_tok.col;
    
    expect(tok_id, "Expected identifier.");
    auto name = std::get<std::string>(cur_tok.value);
    advance();

    std::unique_ptr<tc::ExprAst> node = std::make_unique<VariableExprAst>(line, col, name);

    while (cur_tok.type == '[') {
        int access_line = cur_tok.line;
        int access_col = cur_tok.col;
        advance(); 

        auto index_expr = parse_expr();

        expect(']', "Expected ']' after array index.");
        advance(); 

        node = std::make_unique<ArrayAccessExprAst>(access_line, access_col, std::move(node), std::move(index_expr));
    }

    return node;
}

[[noreturn]] void tc::Parser::report_error(const std::string& message, const tc::TokenData& token) {
    std::string full_message = "Unexpected token at: " + std::to_string(token.line)
        + ", Column " + std::to_string(token.col) + "]: "+ message;
    throw std::runtime_error(full_message);
}

void tc::Parser::advance() {
    cur_tok = lexer.get_next_token();
}

void tc::Parser::expect(int expected_type, std::string msg) {
    if (cur_tok.type != expected_type) {
        report_error(msg, cur_tok);
    }
}

std::shared_ptr<tc::TypeAst> tc::Parser::parse_type() {
    auto type_str = std::get<std::string>(cur_tok.value);
    advance();
    auto base_type = parse_base_type(type_str);

    // is array
    std::vector<unsigned int> dimensions;
    while (cur_tok.type == '[') {
        advance();
        expect(tok_int_lit, "Expected int literal.");
        int signed_size = std::get<int>(cur_tok.value);
        if (signed_size <= 0) {
            report_error("Expected positive integer, got " + std::to_string(signed_size), cur_tok);
        }
        unsigned int size = (unsigned int)signed_size;
        dimensions.push_back(size);

        advance();
        expect(']', "Expected ']'.");
        advance();
    } 

    if (dimensions.empty()) return base_type;
    std::reverse(dimensions.begin(), dimensions.end());

    for (unsigned int dimension : dimensions) {
        base_type = std::make_shared<ArrayTypeAst>(dimension, base_type);
    }

    return base_type;
}


std::shared_ptr<tc::TypeAst> tc::Parser::parse_base_type(std::string& type_str) {
    if (type_str == "float") {
        return TypeAst::get_float();
    }
    if (type_str == "int") {
        return TypeAst::get_int();
    }
    if (type_str == "str") {
        return TypeAst::get_string();
    }
    report_error("Unexpected type '" + type_str + "'", cur_tok);
}
