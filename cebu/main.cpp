#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <format>
#include <iostream>
#include <stdexcept>

#include <cebu/parser.h>
#include <cebu/lexer.h>

using namespace cebu;

void test_lexer();

int main(int argc, char** argv)
{
    (void)argc, (void)argv;
    // test_lexer();
    throw unexpected_token_variant_error(
        token_type::let,
        token_type::b8,
        token_type::b16,
        token_type::b32,
        token_type::b64,
        token_type::b128,
        token_type::i8,
        token_type::i16,
        token_type::i32,
        token_type::i64,
        token_type::i128,
        token_type::f16,
        token_type::f32,
        token_type::f64,
        token_type::f128
    );
}

void test_lexer()
{
    using namespace cebu;

    std::array<any_token, 47> tokens{
        any_token{token_type::identifier, "_abc123"},
        any_token{static_cast<std::size_t>(123)},
        any_token{'\''},
        any_token{token_type::string, "abc\\\'\""},
        any_token{token_type::fn},
        any_token{token_type::let},
        any_token{token_type::if_},
        any_token{token_type::else_},
        any_token{token_type::elif},
        any_token{token_type::ret},
        any_token{token_type::b8},
        any_token{token_type::b16},
        any_token{token_type::b32},
        any_token{token_type::b64},
        any_token{token_type::b128},
        any_token{token_type::i8},
        any_token{token_type::i16},
        any_token{token_type::i32},
        any_token{token_type::i64},
        any_token{token_type::i128},
        any_token{token_type::f16},
        any_token{token_type::f32},
        any_token{token_type::f64},
        any_token{token_type::f128},

        any_token{token_type::double_equals_sign},
        any_token{token_type::rightwards_double_arrow},
        any_token{token_type::equals_sign},

        any_token{token_type::double_plus_sign},
        any_token{token_type::plus_sign},

        any_token{token_type::double_minus_sign},
        any_token{token_type::rightwards_arrow},
        any_token{token_type::minus_sign},

        any_token{token_type::double_vertical_line},
        any_token{token_type::vertical_line},
        any_token{token_type::commercial_at},
        any_token{token_type::colon},
        any_token{token_type::semicolon},
        any_token{token_type::left_parenthesis},
        any_token{token_type::right_parenthesis},
        any_token{token_type::left_angle_bracket},
        any_token{token_type::right_angle_bracket},
        any_token{token_type::left_curly_bracket},
        any_token{token_type::right_curly_bracket},
        any_token{token_type::asterisk},
        any_token{token_type::slash},
        any_token{token_type::percent_sign},
        any_token{token_type::end},
    };

    source_file file{"../../../../test"};
    lexer lexer{file};
    any_token r;
    for (auto t : tokens) {
        try {
            lexer.next(r);
        } catch (incomplete_character_error const& e) {
            std::cerr << e.what() << ": " << std::format("{}, {}", e.position().row, e.position().column) << std::endl;
            throw e;
        }
        auto format{std::format("expected: {}, but recieved: {}", t, r)};
        switch (r.type()) {
        case token_type::identifier:
            delete static_cast<identifier_token>(r).value().begin();
            break;
        case token_type::string:
            delete static_cast<string_token>(r).value().begin();
            break;
        }
        std::cout << format << std::endl;
        if (r.type() != t.type())
            throw std::runtime_error{format.c_str()};
    }
}
