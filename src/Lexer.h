#pragma once
#include <istream>
#include <variant>
#include <string>
#include <optional>

namespace tc {
    // tokeny, które składają się z pojedynczego znaku zwracają po prostu int
    enum Token {
        tok_eof = -1,
        tok_let = -2,
        tok_read = -3,
        tok_print = -4,
        tok_type = -5,
        tok_id = -6,
        tok_int_lit = -7,
        tok_float_lit = -8,
        tok_str = -9,
        tok_bool_lit = -10,
        tok_and = -11,
        tok_or = -12,
        tok_eq = -13,
        tok_more_or_eq = -14,
        tok_less_or_eq = -15,
        tok_not = -16,
        tok_neq = -17,
        tok_while = -18,
        tok_xor = -19,
    };


    struct TokenData {
        int col;
        int line;
        int type;
        std::variant<std::monostate, std::string, int, double, bool> value;
    };

    class Lexer {
        private:
            std::istream& input;
            int last_char = ' ';
            int curr_line = 1;
            int curr_col = 1;
        public:
            Lexer(std::istream& in) : input(in) {}
            TokenData get_next_token();
        private:
            std::optional<TokenData> match_keyword(std::string keyword, int line, int col);
            [[noreturn]] void report_error(const std::string& message);
            void next_char();
            bool is_known_single_char(int ch);
    };
}
