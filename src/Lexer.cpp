#include "Lexer.h"

#include <cctype>
#include <stdexcept>
#include <optional>
#include <string>

tc::TokenData tc::Lexer::get_next_token() {
    // whitespace
    while (std::isspace(last_char)) {
        next_char();
    }

    // identifiers and keywords
    if (std::isalpha(last_char)) {
        int line = curr_line;
        int col = curr_col;

        std::string id_str;
        while (std::isalnum(last_char) || last_char == '_') {
            id_str += (char)last_char;
            next_char();
        }

        auto keyword_opt = match_keyword(id_str, line, col);
        if (keyword_opt)
            return keyword_opt.value();

        return {col, line, tok_id, id_str};
    }

    // literals
    if (std::isdigit(last_char)) {
        int line = curr_line;
        int col = curr_col;

        std::string num_str;
        bool is_float = false;

        while (std::isdigit(last_char) || last_char == '.') {
            num_str += (char)(last_char);
            if (last_char == '.' && is_float)
                report_error("Unexpected character in number literal '.'");
            if (last_char == '.')
                is_float = true;
            next_char();
        }

        if (std::isalpha(last_char) || last_char == '_') {
            report_error(std::string("Unexpected character '") + (char)last_char + "' right after number literal.");
        }

        if (is_float) {
            return { col, line, tok_float_lit, std::stod(num_str) };
        }
        return { col, line, tok_int_lit, std::stoi(num_str) };
    }

    if (last_char == '"') {
        int line = curr_line;
        int col = curr_col;

        next_char();
        std::string str_literal = "";
        while (last_char != '"' && last_char != EOF && !input.eof()) {
            str_literal += last_char;
            next_char();
        }
        if (last_char != '"') {
            report_error("Unterminated string literal");
        }
        next_char();
        return { col, line, tok_str, str_literal };
    }

    if (last_char == '=') {
        int line = curr_line;
        int col = curr_col;

        next_char();
        if (last_char == '=') {
            next_char();
            return {col, line, tok_eq, std::monostate()};
        }
        return {col, line, '=', std::monostate()};
    }

    if (last_char == '!') {
        int line = curr_line;
        int col = curr_col;

        next_char();
        if (last_char == '=') {
            next_char();
            return {col, line, tok_neq, std::monostate()};
        }
        report_error("Expected '=' after '!'");
    }

    if (last_char == '<') {
        int line = curr_line;
        int col = curr_col;

        next_char();
        if (last_char == '=') {
            next_char();
            return {col, line, tok_less_or_eq, std::monostate()};
        }
        return {col, line, '<', std::monostate()};
    }

    if (last_char == '>') {
        int line = curr_line;
        int col = curr_col;

        next_char();
        if (last_char == '=') {
            next_char();
            return {col, line, tok_more_or_eq, std::monostate()};
        }
        return {col, line, '>', std::monostate()};
    }

    // '/' handled separately to detect '//' line comments
    if (last_char == '/') {
        int line = curr_line;
        int col  = curr_col;
        next_char();
        if (last_char == '/') {
            // consume the rest of the line
            while (last_char != '\n' && last_char != EOF && !input.eof()) {
                next_char();
            }
            return get_next_token(); // skip to next real token
        }
        return {col, line, '/', std::monostate()};
    }

    // '-' handled separately to detect '->' arrow token
    if (last_char == '-') {
        int line = curr_line;
        int col = curr_col;

        next_char();
        if (last_char == '>') {
            next_char();
            return {col, line, tok_arrow, std::monostate()};
        }
        return {col, line, '-', std::monostate()};
    }

    if (last_char == EOF || input.eof()) {
        return {curr_col, curr_line, tok_eof, std::monostate()};
    }

    if (is_known_single_char(last_char)) {
        int symbol = last_char;
        next_char();
        return { curr_col, curr_line, symbol, std::monostate() };
    }

    report_error(std::string("Unexpected character '") + (char)last_char + "'");
}

std::optional<tc::TokenData> tc::Lexer::match_keyword(std::string keyword, int line, int col) {
    // keywords
    if (keyword == "let") {
        return { {col, line, tok_let, std::monostate()} };
    }
    if (keyword == "read") {
        return { {col, line, tok_read, std::monostate()} };
    }
    if (keyword == "print") {
        return { {col, line, tok_print, std::monostate()} };
    }
    if (keyword == "and") {
        return { {col, line, tok_and, std::monostate()} };
    }
    if (keyword == "or") {
        return { {col, line, tok_or, std::monostate()} };
    }
    if (keyword == "not") {
        return { {col, line, tok_not, std::monostate()} };
    }
    if (keyword == "while") {
        return { {col, line, tok_while, std::monostate()} };
    }
    if (keyword == "xor") {
        return { {col, line, tok_xor, std::monostate()} };
    }
    if (keyword == "if") {
        return { {col, line, tok_if, std::monostate()} };
    }
    if (keyword == "else") {
        return { {col, line, tok_else, std::monostate()} };
    }
    if (keyword == "fn") {
        return { {col, line, tok_fn, std::monostate()} };
    }
    if (keyword == "return") {
        return { {col, line, tok_return, std::monostate()} };
    }
    if (keyword == "break") {
        return { {col, line, tok_break, std::monostate()} };
    }
    if (keyword == "struct") {
        return { {col, line, tok_struct, std::monostate()} };
    }
    if (keyword == "class") {
        return { {col, line, tok_class, std::monostate()} };
    }
    if (keyword == "new") {
        return { {col, line, tok_new, std::monostate()} };
    }

    // builtin types
    if (keyword == "int") {
        return { {col, line, tok_type, std::string("int")} };
    }
    if (keyword == "float") {
        return { {col, line, tok_type, std::string("float")} };
    }
    if (keyword == "str") {
        return { {col, line, tok_type, std::string("str")} };
    }
    if (keyword == "bool") {
        return { {col, line, tok_type, std::string("bool")} };
    }
    if (keyword == "void") {
        return { {col, line, tok_type, std::string("void")} };
    }

    // values
    if (keyword == "true") {
        return { {col, line, tok_bool_lit, true} };
    }
    if (keyword == "false") {
        return { {col, line, tok_bool_lit, false} };
    }

    return std::nullopt;
}


bool tc::Lexer::is_known_single_char(int ch) {
    // Note: '-' is handled separately (to detect '->'), '=' is handled for '=='
    // Note: '-' handled separately (->), '/' handled separately (//)
    std::string known_chars = "=+*();:][{},.";
    return !(known_chars.find((char)ch) == std::string::npos);
}

[[noreturn]] void tc::Lexer::report_error(const std::string& message) {
    std::string full_message = "Lexical error [Line " + std::to_string(curr_line)
        + ", Column " + std::to_string(curr_col) + "]: "+ message;
    throw std::runtime_error(full_message);
}

void tc::Lexer::next_char() {
    if (last_char == '\n') {
        curr_line += 1;
        curr_col = 0;
    }
    last_char = input.get();
    curr_col += 1;
}
