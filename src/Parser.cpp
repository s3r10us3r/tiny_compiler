#include "Parser.h"
#include "Lexer.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>

// -------------------------------------------------------------------------
// TypeAst factory methods
// -------------------------------------------------------------------------

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

std::shared_ptr<tc::TypeAst> tc::TypeAst::get_bool() {
    static auto instance = std::make_shared<tc::BoolTypeAst>();
    return instance;
}

std::shared_ptr<tc::TypeAst> tc::TypeAst::get_void() {
    static auto instance = std::make_shared<tc::VoidTypeAst>();
    return instance;
}

// -------------------------------------------------------------------------
// ProgramAst
// -------------------------------------------------------------------------

void tc::ProgramAst::add_stmt(std::unique_ptr<tc::StmtAst> stmt) {
    statements.push_back(std::move(stmt));
}

// -------------------------------------------------------------------------
// Top-level parse
// -------------------------------------------------------------------------

tc::ProgramAst tc::Parser::parse() {
    advance();
    while (cur_tok.type != tok_eof) {
        program.add_stmt(parse_stmt());
    }
    return std::move(program);
}

// -------------------------------------------------------------------------
// Statement dispatch
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_stmt() {
    std::unique_ptr<tc::StmtAst> stmt = nullptr;

    if (cur_tok.type == tok_let) {
        stmt = parse_var_decl();
    } else if (cur_tok.type == tok_read) {
        stmt = parse_read();
    } else if (cur_tok.type == tok_print) {
        stmt = parse_print();
    } else if (cur_tok.type == tok_id) {
        stmt = parse_assgn();
    } else if (cur_tok.type == '{') {
        stmt = parse_block();
    } else if (cur_tok.type == tok_while) {
        stmt = parse_while();
    } else if (cur_tok.type == tok_break) {
        stmt = parse_break();
    } else if (cur_tok.type == tok_if) {
        stmt = parse_if();
    } else if (cur_tok.type == tok_fn) {
        stmt = parse_func_decl();
    } else if (cur_tok.type == tok_return) {
        stmt = parse_return();
    } else if (cur_tok.type == tok_struct || cur_tok.type == tok_class) {
        stmt = parse_struct_decl();
    }

    if (stmt == nullptr) {
        report_error("Unrecognized statement.", cur_tok);
    }

    expect(';', "';' expected");
    advance();
    return stmt;
}

// -------------------------------------------------------------------------
// While and Break
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_while() {
    int line = cur_tok.line; int col = cur_tok.col;
    advance(); // consume 'while'

    auto condition = parse_expr();

    if (cur_tok.type != '{') {
        report_error("Expected '{' to start while loop body.", cur_tok);
    }

    auto body = parse_block();
    return std::make_unique<WhileStmtAst>(line, col, std::move(condition), std::move(body));
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_break() {
    int line = cur_tok.line; int col = cur_tok.col;
    advance(); // consume 'break'
    return std::make_unique<BreakStmtAst>(line, col);
}

// -------------------------------------------------------------------------
// If / Else
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_if() {
    int line = cur_tok.line; int col = cur_tok.col;
    advance(); // consume 'if'

    auto condition = parse_expr();

    if (cur_tok.type != '{') {
        report_error("Expected '{' to start 'if' body.", cur_tok);
    }
    auto then_block = parse_block();

    std::unique_ptr<StmtAst> else_block = nullptr; // BlockAst or IfStmtAst (else-if)
    if (cur_tok.type == tok_else) {
        advance(); // consume 'else'
        if (cur_tok.type == tok_if) {
            // else-if chain: parse another if directly (no extra ';' here)
            else_block = parse_if();
            // parse_if() returned without consuming the ';' — that's correct
            // because the outer parse_stmt() will consume the final ';'
            return std::make_unique<IfStmtAst>(line, col,
                std::move(condition), std::move(then_block), std::move(else_block));
        } else if (cur_tok.type == '{') {
            else_block = parse_block();
        } else {
            report_error("Expected '{' or 'if' after 'else'.", cur_tok);
        }
    }

    return std::make_unique<IfStmtAst>(line, col,
        std::move(condition), std::move(then_block), std::move(else_block));
}

// -------------------------------------------------------------------------
// Block
// -------------------------------------------------------------------------

std::unique_ptr<tc::BlockAst> tc::Parser::parse_block() {
    int line = cur_tok.line;
    int col = cur_tok.col;

    expect('{', "Expected '{' at the beginning of a block.");
    advance();

    std::unique_ptr<BlockAst> block = std::make_unique<BlockAst>(line, col);

    while (cur_tok.type != '}' && cur_tok.type != tok_eof) {
        auto stmt = parse_stmt();
        block->push_stmt(std::move(stmt));
    }
    if (cur_tok.type == tok_eof) {
        report_error("Unterminated block, expected '}'", cur_tok);
    }
    advance(); // consume '}'
    return block;
}

// -------------------------------------------------------------------------
// Variable declaration
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_var_decl() {
    expect(tok_let, "Expected 'let' at the beginning of variable declaration.");
    int line = cur_tok.line; int col = cur_tok.col;
    advance();
    expect_identifier("variable");
    auto name = std::get<std::string>(cur_tok.value);
    advance();
    expect(':', "':' expected.");
    advance();
    // parse_type() handles both built-in types (tok_type) and struct names (tok_id)
    auto type = parse_type();
    expect('=', "'=' expected.");
    advance();
    auto expr = parse_expr();
    return std::make_unique<VarDeclStmtAst>(line, col, name, type, std::move(expr));
}

// -------------------------------------------------------------------------
// Print / Read
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_print() {
    expect(tok_print, "This should never happen.");
    int line = cur_tok.line; int col = cur_tok.col;
    advance();
    auto expr = parse_expr();
    return std::make_unique<PrintStmtAst>(line, col, std::move(expr));
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_read() {
    expect(tok_read, "This should never happen.");
    int line = cur_tok.line; int col = cur_tok.col;
    advance();
    auto target_expr = parse_var_expr();
    return std::make_unique<ReadStmtAst>(line, col, std::move(target_expr));
}

// -------------------------------------------------------------------------
// Assignment / function-call statement
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_assgn() {
    int line = cur_tok.line; int col = cur_tok.col;
    expect(tok_id, "Expected identifier.");
    auto name = std::get<std::string>(cur_tok.value);
    advance();

    // Function call used as a statement: foo(args)
    if (cur_tok.type == '(') {
        auto call_expr = parse_func_call(name, line, col);
        return std::make_unique<ExprStmtAst>(line, col, std::move(call_expr));
    }

    // Build lvalue / method-call chain starting from the identifier
    std::unique_ptr<ExprAst> target = std::make_unique<VariableExprAst>(line, col, name);
    target = parse_postfix(std::move(target));

    // If no '=' follows, this must be a method call (or other expr) used as a statement
    if (cur_tok.type != '=') {
        return std::make_unique<ExprStmtAst>(line, col, std::move(target));
    }

    advance(); // consume '='
    auto expr = parse_expr();
    return std::make_unique<AssignmentStmtAst>(line, col, std::move(target), std::move(expr));
}

// -------------------------------------------------------------------------
// Function declaration / return
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_func_decl() {
    int line = cur_tok.line; int col = cur_tok.col;
    advance(); // consume 'fn'

    expect_identifier("function");
    auto name = std::get<std::string>(cur_tok.value);
    advance();

    expect('(', "Expected '(' after function name.");
    advance();

    // Parameter list
    std::vector<FuncParam> params;
    while (cur_tok.type != ')') {
        expect_identifier("parameter");
        auto param_name = std::get<std::string>(cur_tok.value);
        advance();

        expect(':', "Expected ':' after parameter name.");
        advance();

        auto param_type = parse_type();
        params.push_back({param_name, param_type});

        if (cur_tok.type == ',') {
            advance(); // consume ','
        } else if (cur_tok.type != ')') {
            report_error("Expected ',' or ')' in parameter list.", cur_tok);
        }
    }
    advance(); // consume ')'

    expect(tok_arrow, "Expected '->' after parameter list.");
    advance();

    auto return_type = parse_type();

    if (cur_tok.type != '{') {
        report_error("Expected '{' to start function body.", cur_tok);
    }
    auto body = parse_block();

    return std::make_unique<FuncDeclStmtAst>(line, col,
        std::move(name), std::move(params), return_type, std::move(body));
}

std::unique_ptr<tc::StmtAst> tc::Parser::parse_return() {
    int line = cur_tok.line; int col = cur_tok.col;
    advance(); // consume 'return'

    // Void return: next token is ';'
    if (cur_tok.type == ';') {
        return std::make_unique<ReturnStmtAst>(line, col, nullptr);
    }

    auto expr = parse_expr();
    return std::make_unique<ReturnStmtAst>(line, col, std::move(expr));
}

// -------------------------------------------------------------------------
// Struct declaration
// -------------------------------------------------------------------------

std::unique_ptr<tc::StmtAst> tc::Parser::parse_struct_decl() {
    int line = cur_tok.line; int col = cur_tok.col;
    advance(); // consume 'struct'

    expect_identifier("struct");
    auto name = std::get<std::string>(cur_tok.value);
    advance();

    expect('{', "Expected '{' to start struct body.");
    advance();

    std::vector<StructField> fields;
    std::vector<std::unique_ptr<MethodDeclStmtAst>> methods;

    while (cur_tok.type != '}') {
        if (cur_tok.type == tok_fn) {
            // Method declaration inside struct body
            auto method = parse_method_decl(name);
            methods.push_back(std::move(method));
            expect(';', "Expected ';' after method declaration.");
            advance();
        } else {
            // Field declaration
            expect_identifier("field");
            auto field_name = std::get<std::string>(cur_tok.value);
            advance();

            expect(':', "Expected ':' after field name.");
            advance();

            auto field_type = parse_type();
            fields.push_back({field_name, field_type});

            expect(';', "Expected ';' after field declaration.");
            advance();
        }
    }
    advance(); // consume '}'

    return std::make_unique<StructDeclStmtAst>(line, col,
        std::move(name), std::move(fields), std::move(methods));
}

// -------------------------------------------------------------------------
// Method declaration inside a struct body
// -------------------------------------------------------------------------

std::unique_ptr<tc::MethodDeclStmtAst> tc::Parser::parse_method_decl(const std::string& struct_name) {
    int line = cur_tok.line; int col = cur_tok.col;
    advance(); // consume 'fn'

    expect_identifier("method");
    auto name = std::get<std::string>(cur_tok.value);
    advance();

    expect('(', "Expected '(' after method name.");
    advance();

    std::vector<FuncParam> params;
    while (cur_tok.type != ')') {
        expect_identifier("parameter");
        auto param_name = std::get<std::string>(cur_tok.value);
        advance();

        std::shared_ptr<TypeAst> param_type;
        if (param_name == "self" && (cur_tok.type == ',' || cur_tok.type == ')')) {
            // 'self' with implicit struct type — no ':' annotation needed
            param_type = std::make_shared<StructTypeAst>(struct_name);
        } else {
            expect(':', "Expected ':' after parameter name.");
            advance();
            param_type = parse_type();
        }
        params.push_back({param_name, param_type});

        if (cur_tok.type == ',') {
            advance();
        } else if (cur_tok.type != ')') {
            report_error("Expected ',' or ')' in method parameter list.", cur_tok);
        }
    }
    advance(); // consume ')'

    expect(tok_arrow, "Expected '->' after method parameter list.");
    advance();

    auto return_type = parse_type();

    if (cur_tok.type != '{') {
        report_error("Expected '{' to start method body.", cur_tok);
    }
    auto body = parse_block();

    return std::make_unique<MethodDeclStmtAst>(line, col,
        std::move(name), std::move(params), return_type, std::move(body), struct_name);
}

// -------------------------------------------------------------------------
// Expression parsing (recursive descent)
// -------------------------------------------------------------------------

std::unique_ptr<tc::ExprAst> tc::Parser::parse_expr() {
    int line = cur_tok.line; int col = cur_tok.col;
    auto left = parse_logical_xor();
    while (cur_tok.type == tok_or) {
        int op = cur_tok.type; advance();
        auto right = parse_logical_and();
        left = std::make_unique<BinaryExprAst>(line, col, op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_logical_xor() {
    int line = cur_tok.line; int col = cur_tok.col;
    auto left = parse_logical_and();
    while (cur_tok.type == tok_xor) {
        int op = cur_tok.type; advance();
        auto right = parse_logical_and();
        left = std::make_unique<BinaryExprAst>(line, col, op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_logical_and() {
    int line = cur_tok.line; int col = cur_tok.col;
    auto left = parse_relational();
    while (cur_tok.type == tok_and) {
        int op = cur_tok.type;
        advance();
        auto right = parse_relational();
        left = std::make_unique<BinaryExprAst>(line, col, op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_relational() {
    int line = cur_tok.line; int col = cur_tok.col;
    auto left = parse_math_expr();
    while (cur_tok.type == tok_eq || cur_tok.type == tok_neq ||
           cur_tok.type == '<'    || cur_tok.type == '>'    ||
           cur_tok.type == tok_less_or_eq || cur_tok.type == tok_more_or_eq) {
        int op = cur_tok.type; advance();
        auto right = parse_math_expr();
        left = std::make_unique<BinaryExprAst>(line, col, op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<tc::ExprAst> tc::Parser::parse_math_expr() {
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
    if (cur_tok.type == tok_not) {
        int line = cur_tok.line;
        int col = cur_tok.col;
        advance();
        auto operand = parse_unary();
        return std::make_unique<UnaryExprAst>(line, col, tok_not, std::move(operand));
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

    // Struct literal: new StructName { field = expr, ... }
    // Using 'new' keyword removes the ambiguity with block '{' after identifiers.
    if (cur_tok.type == tok_new) {
        advance(); // consume 'new'
        expect(tok_id, "Expected struct name after 'new'.");
        auto name = std::get<std::string>(cur_tok.value);
        advance();
        return parse_struct_literal(name, line, col);
    }

    if (cur_tok.type == tok_id) {
        auto name = std::get<std::string>(cur_tok.value);
        advance();

        if (cur_tok.type == '(') {
            // Function call expression: foo(args)
            return parse_func_call(name, line, col);
        }

        // Regular variable with optional postfix chains ([index], .field)
        std::unique_ptr<ExprAst> node = std::make_unique<VariableExprAst>(line, col, name);
        return parse_postfix(std::move(node));
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
        expect(')', "Expected closing ')'.");
        advance();
        return expr;
    }
    if (cur_tok.type == tok_bool_lit) {
        auto value = std::get<bool>(cur_tok.value);
        advance();
        return std::make_unique<BoolExprAst>(line, col, value);
    }
    report_error("Unexpected token at the end of expression.", cur_tok);
}

// -------------------------------------------------------------------------
// Function call expression
// -------------------------------------------------------------------------

std::unique_ptr<tc::ExprAst> tc::Parser::parse_func_call(
    const std::string& name, int line, int col)
{
    advance(); // consume '('

    std::vector<std::unique_ptr<ExprAst>> args;
    while (cur_tok.type != ')') {
        args.push_back(parse_expr());
        if (cur_tok.type == ',') {
            advance(); // consume ','
        } else if (cur_tok.type != ')') {
            report_error("Expected ',' or ')' in argument list.", cur_tok);
        }
    }
    advance(); // consume ')'

    return std::make_unique<FuncCallExprAst>(line, col, name, std::move(args));
}

// -------------------------------------------------------------------------
// Struct literal expression
// -------------------------------------------------------------------------

std::unique_ptr<tc::ExprAst> tc::Parser::parse_struct_literal(
    const std::string& name, int line, int col)
{
    advance(); // consume '{'

    std::vector<std::pair<std::string, std::unique_ptr<ExprAst>>> fields;
    while (cur_tok.type != '}') {
        expect(tok_id, "Expected field name in struct literal.");
        auto field_name = std::get<std::string>(cur_tok.value);
        advance();

        expect('=', "Expected '=' after field name.");
        advance();

        auto value = parse_expr();
        fields.emplace_back(field_name, std::move(value));

        if (cur_tok.type == ',') {
            advance(); // consume ','
        } else if (cur_tok.type != '}') {
            report_error("Expected ',' or '}' in struct literal.", cur_tok);
        }
    }
    advance(); // consume '}'

    return std::make_unique<StructLiteralExprAst>(line, col, name, std::move(fields));
}

// -------------------------------------------------------------------------
// Postfix chain: [index] and .field
// -------------------------------------------------------------------------

std::unique_ptr<tc::ExprAst> tc::Parser::parse_postfix(std::unique_ptr<ExprAst> node) {
    while (true) {
        if (cur_tok.type == '[') {
            int access_line = cur_tok.line;
            int access_col = cur_tok.col;
            advance(); // consume '['

            auto index_expr = parse_expr();

            expect(']', "Expected ']' after array index.");
            advance(); // consume ']'

            node = std::make_unique<ArrayAccessExprAst>(
                access_line, access_col, std::move(node), std::move(index_expr));

        } else if (cur_tok.type == '.') {
            int dot_line = cur_tok.line;
            int dot_col  = cur_tok.col;
            advance(); // consume '.'

            expect(tok_id, "Expected field/method name after '.'.");
            auto member_name = std::get<std::string>(cur_tok.value);
            advance();

            if (cur_tok.type == '(') {
                // Method call: obj.method(args)
                advance(); // consume '('
                std::vector<std::unique_ptr<ExprAst>> args;
                while (cur_tok.type != ')') {
                    args.push_back(parse_expr());
                    if (cur_tok.type == ',') {
                        advance();
                    } else if (cur_tok.type != ')') {
                        report_error("Expected ',' or ')' in method argument list.", cur_tok);
                    }
                }
                advance(); // consume ')'
                node = std::make_unique<MethodCallExprAst>(
                    dot_line, dot_col, std::move(node), member_name, std::move(args));
            } else {
                // Field access: obj.field
                node = std::make_unique<FieldAccessExprAst>(
                    dot_line, dot_col, std::move(node), member_name);
            }

        } else {
            break;
        }
    }
    return node;
}

// -------------------------------------------------------------------------
// Variable expression (for lvalue contexts: assignment / read targets)
// -------------------------------------------------------------------------

std::unique_ptr<tc::ExprAst> tc::Parser::parse_var_expr() {
    int line = cur_tok.line;
    int col  = cur_tok.col;

    expect(tok_id, "Expected identifier.");
    auto name = std::get<std::string>(cur_tok.value);
    advance();

    std::unique_ptr<ExprAst> node = std::make_unique<VariableExprAst>(line, col, name);
    return parse_postfix(std::move(node));
}

// -------------------------------------------------------------------------
// Type parsing
// -------------------------------------------------------------------------

std::shared_ptr<tc::TypeAst> tc::Parser::parse_type() {
    // User-defined struct type: just an identifier
    if (cur_tok.type == tok_id) {
        auto name = std::get<std::string>(cur_tok.value);
        advance();
        return std::make_shared<StructTypeAst>(name);
    }

    expect(tok_type, "Expected a type.");
    auto type_str = std::get<std::string>(cur_tok.value);
    advance();
    auto base_type = parse_base_type(type_str);

    // Array dimensions: type[size1][size2]...
    std::vector<unsigned int> dimensions;
    while (cur_tok.type == '[') {
        advance();
        expect(tok_int_lit, "Expected integer array size.");
        int signed_size = std::get<int>(cur_tok.value);
        if (signed_size <= 0) {
            report_error("Expected positive integer, got " + std::to_string(signed_size), cur_tok);
        }
        dimensions.push_back((unsigned int)signed_size);
        advance();
        expect(']', "Expected ']'.");
        advance();
    }

    if (dimensions.empty()) return base_type;

    std::reverse(dimensions.begin(), dimensions.end());
    for (unsigned int dim : dimensions) {
        base_type = std::make_shared<ArrayTypeAst>(dim, base_type);
    }
    return base_type;
}

std::shared_ptr<tc::TypeAst> tc::Parser::parse_base_type(std::string& type_str) {
    if (type_str == "float")  return TypeAst::get_float();
    if (type_str == "int")    return TypeAst::get_int();
    if (type_str == "str")    return TypeAst::get_string();
    if (type_str == "bool")   return TypeAst::get_bool();
    if (type_str == "void")   return TypeAst::get_void();
    report_error("Unexpected type '" + type_str + "'", cur_tok);
}

// -------------------------------------------------------------------------
// Parser helpers
// -------------------------------------------------------------------------

// Map a non-identifier token type back to its source keyword string.
// Returns empty string if the token is not a keyword.
static std::string keyword_token_name(int type) {
    switch (type) {
        case tc::tok_let:    return "let";
        case tc::tok_read:   return "read";
        case tc::tok_print:  return "print";
        case tc::tok_if:     return "if";
        case tc::tok_else:   return "else";
        case tc::tok_while:  return "while";
        case tc::tok_break:  return "break";
        case tc::tok_fn:     return "fn";
        case tc::tok_return: return "return";
        case tc::tok_struct: return "struct";
        case tc::tok_class:  return "class";
        case tc::tok_new:    return "new";
        case tc::tok_and:    return "and";
        case tc::tok_or:     return "or";
        case tc::tok_not:    return "not";
        case tc::tok_xor:    return "xor";
        default:             return "";
    }
}

// Verify that the current token is a plain identifier.
// If it is a reserved keyword or built-in type name, throw a targeted error
// explaining exactly why that token cannot be used as a name here.
void tc::Parser::expect_identifier(const std::string& context) {
    if (cur_tok.type == tok_id) return; // OK

    // Built-in type keywords — their text lives in cur_tok.value
    if (cur_tok.type == tok_type) {
        auto name = std::get<std::string>(cur_tok.value);
        report_error(
            "'" + name + "' is a built-in type name and cannot be used as a "
            + context + " name.", cur_tok);
    }

    // Boolean literals
    if (cur_tok.type == tok_bool_lit) {
        report_error(
            "'true' and 'false' are reserved literals and cannot be used as a "
            + context + " name.", cur_tok);
    }

    // Every other reserved keyword
    auto kw = keyword_token_name(cur_tok.type);
    if (!kw.empty()) {
        report_error(
            "'" + kw + "' is a reserved keyword and cannot be used as a "
            + context + " name.", cur_tok);
    }

    // Anything else (operator, EOF, …)
    report_error("Expected a valid " + context + " name here.", cur_tok);
}

[[noreturn]] void tc::Parser::report_error(const std::string& message, const tc::TokenData& token) {
    std::string full_message = "Parse error [Line " + std::to_string(token.line)
        + ", Column " + std::to_string(token.col) + "]: " + message;
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
