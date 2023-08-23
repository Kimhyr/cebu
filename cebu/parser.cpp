#include <fstream>
#include <sstream>

#include "cebu/syntax.h"
#include "parser.h"

namespace cebu
{

char const* end_of_file_error::what() const noexcept
{
    return "end of file";
}

template<parsing_error Error, typename ...Args>
void parser::report(Args&&... args) const noexcept
{
    std::string format{std::format("[{}] parsing error: ", location())};
    if constexpr(Error == parsing_error::unexpected_token) {
        format += [&](auto const& expected) -> std::string {
            return std::format("expected token `{}` instead of token `{}`", expected, token());
        }(args...);
    } else if constexpr(Error == parsing_error::unexpected_token_variant) {
        format += "expected one of tokens:\n";
        [&](auto const& tokens) {
            for (auto const& t : tokens)
                format += std::format("\t`{}`\n", t);
        }(args...);
        format += std::format("intead of token `{}`", token());
    }
    std::cerr << format << std::endl;
}

void parser::unsafely_load_file(std::string_view const& file_path)
{
    std::ifstream file{file_path.begin()};
    std::ostringstream sstr;
    sstr << file.rdbuf();
    m_source = sstr.str();
    m_lexer.load(file_path, m_source.data());
}

template<typename ...Ts>
void syntax_parser<method_declaration, Ts...>::parse(
        parser& parser,
        method_declaration& out)
{
    parser
        .parse<identifier, enable_on_error>(out.identifier)
        .parse<lambda_type>(out.lambda)
        .parse<body>(out.body);
}

template<typename ...Ts>
void syntax_parser<lambda_type, Ts...>::parse(
        parser& parser,
        lambda_type& out)
{
    parser
        .parse<tuple_type>(out.tuple)
        .expect<token_type::rightwards_arrow>()
        .parse<type>(out.return_type);
}

template<typename ...Ts>
void syntax_parser<tuple_type, Ts...>::parse(
        parser& parser,
        tuple_type& out)
{
    bool proceed{false};
    parser
        .expect<token_type::left_parenthesis, enable_on_success>([&] {
            do {
                out.mappings.resize(out.mappings.size() + 1);
                parser
                    .parse<value_declaration>(out.mappings.back())
                    .expect_one_of<std::array{
                        token_type::comma,
                        token_type::right_parenthesis
                    }, enable_on_success, enable_on_error>([&] {
                        proceed = parser.token() != token_type::right_parenthesis;
                    }, [&] { proceed = false; });
            } while (proceed);
        });
}

template<typename ...Ts>
void syntax_parser<type, Ts...>::parse(
        parser& parser,
        type& out)
{
    parser
        .peek_one_of<std::array{
            token_type::b8,
            token_type::b16,
            token_type::b32,
            token_type::b64,
            token_type::i8,
            token_type::i16,
            token_type::i32,
            token_type::i64,
            token_type::f16,
            token_type::f32,
            token_type::f64,
            token_type::left_parenthesis
        }, enable_on_success>([&] {
            if (parser.token() == token_type::left_parenthesis) {
                out.type = type::tuple;
                cebu::token peek;
                auto lambda{new lambda_type};
                parser
                    .parse<tuple_type>(lambda->tuple)
                    .expect<
                        token_type::rightwards_arrow,
                        enable_on_success, enable_on_error,
                        dont_report
                    >([&] {
                        parser
                            .consume()
                            .parse<type>(lambda->return_type);
                        out.value.lambda = lambda;
                    }, [&] {
                        (out.value.tuple = new tuple_type)->mappings =
                            std::move(lambda->tuple.mappings);
                        delete lambda;
                    });
            } else {
                out.type = type::primitive;
                out.value.primitive = static_cast<primitive_type>(
                    parser.token().type);
            }
        });
}

template<typename ...Ts>
void syntax_parser<value_declaration, Ts...>::parse(
        parser& parser,
        value_declaration& out)
{
    parser
        .parse<identifier>(out.identifier)
        .expect<token_type::colon>()
        .parse<type>(out.type)
        .parse<body>(out.body);
}

}
