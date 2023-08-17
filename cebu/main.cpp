#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <format>
#include <iostream>
#include <stdexcept>

#include <cebu/lexer.h>

void test_lexer()
{
    using namespace cebu;

    std::array<token, 47> tokens{
        token{ .value.identifier = "_abc123", .type = token_type::identifier},
        token{ .value.number = 123, .type = token_type::number},
        token{ .value.character = '\'', .type = token_type::character},
        token{ .value.string = "abc\\\'\"", .type = token_type::string},
        token{.type = token_type::fn, },
        token{.type = token_type::let, },
        token{.type = token_type::if_, },
        token{.type = token_type::else_, },
        token{.type = token_type::elif, },
        token{.type = token_type::ret, },
        token{.type = token_type::b8, },
        token{.type = token_type::b16, },
        token{.type = token_type::b32, },
        token{.type = token_type::b64, },
        token{.type = token_type::b128, },
        token{.type = token_type::i8, },
        token{.type = token_type::i16, },
        token{.type = token_type::i32, },
        token{.type = token_type::i64, },
        token{.type = token_type::i128, },
        token{.type = token_type::f16, },
        token{.type = token_type::f32, },
        token{.type = token_type::f64, },
        token{.type = token_type::f128, },

        token{.type = token_type::double_equals_sign, },
        token{.type = token_type::rightwards_double_arrow, },
        token{.type = token_type::equals_sign, },

        token{.type = token_type::double_plus_sign, },
        token{.type = token_type::plus_sign, },

        token{.type = token_type::double_minus_sign, },
        token{.type = token_type::rightwards_arrow, },
        token{.type = token_type::minus_sign, },

        token{.type = token_type::double_vertical_line, },
        token{.type = token_type::vertical_line, },
        token{.type = token_type::commercial_at, },
        token{.type = token_type::colon, },
        token{.type = token_type::semicolon, },
        token{.type = token_type::left_parenthesis, },
        token{.type = token_type::right_parenthesis, },
        token{.type = token_type::left_angle_bracket, },
        token{.type = token_type::right_angle_bracket, },
        token{.type = token_type::left_curly_bracket, },
        token{.type = token_type::right_curly_bracket, },
        token{.type = token_type::asterisk, },
        token{.type = token_type::slash, },
        token{.type = token_type::percent_sign, },
        token{.type = token_type::end, },
    };

    source_file file{"../../../../test"};
    lexer lexer{file};
    token r;
    for (auto t : tokens) {
        try {
            lexer.next(r);
        } catch (incomplete_character_error const& e) {
            std::cerr << e.what() << ": " << std::format("{}, {}", e.position().row, e.position().column) << std::endl;
            throw e;
        }
        auto format{std::format("expected: {}, but recieved: {}", t, r)};
        switch (r.type) {
        case token_type::identifier:
            delete r.value.identifier.begin();
            break;
        case token_type::string:
            delete r.value.string.begin();
            break;
        }
        std::cout << format << std::endl;
        if (r.type != t.type)
            throw std::runtime_error{format.c_str()};
    }
}

int main(int argc, char** argv)
{
    using namespace cebu;
    test_lexer();
    return 0;
}
