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
    };


    struct TokenData {
        int col;
        int line;
        int type;
        std::variant<std::monostate, std::string, int, double> value;
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
