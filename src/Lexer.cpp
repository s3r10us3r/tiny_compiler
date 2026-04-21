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
            if (last_char == '.') {
                if (is_float) {
                    report_error("Unexpected character in number literal '.'");
                }
                is_float = true;
            }
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
    // empty keywords
    if (keyword == "let") {
        return { {line, col, tok_let, std::monostate()} };
    }
    if (keyword == "read") {
        return { {line, col, tok_read, std::monostate()} };
    }
    if (keyword == "print") {
        return { {line, col, tok_print, std::monostate()} };
    }

    // builtin types
    if (keyword == "int") {
        return { {line, col, tok_type, "int"} };
    } 
    if (keyword == "float") {
        return { {line, col, tok_type, "float"} };
    } 

    return std::nullopt;
}


bool tc::Lexer::is_known_single_char(int ch) {
    std::string known_chars = "=+-*/();:][";
    return !(known_chars.find((char)ch) == std::string::npos);
}

[[noreturn]] void tc::Lexer::report_error(const std::string& message) {
    std::string full_message = "Lexical error [Line " + std::to_string(curr_line)
        + ", Column " + std::to_string(curr_col) + "]: "+ message;
    throw std::runtime_error(full_message);
}

void tc::Lexer::next_char() {
    last_char = input.get();
    if (last_char == '\n') {
        curr_line += 1;
        curr_col = 1;
    } else {
        curr_col += 1;
    }
}
