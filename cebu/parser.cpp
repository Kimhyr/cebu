#include <fstream>
#include <sstream>

#include "cebu/syntax.h"
#include "cebu/token.h"
#include "parser.h"

namespace cebu
{

char const* end_of_file_error::what() const noexcept
{ return "end of file"; }

template<parsing_error Error, typename ...Args>
void parser::report(Args&&... args) const noexcept
{
    std::string format{std::format("[{}] parsing error: ", this->location())};
    if constexpr(Error == parsing_error::unexpected_token) {
        [&](auto const& tokens) {
            if constexpr(requires {
                typename decltype(tokens)::value_type;
            }) {
                format += "expected one of tokens:\n";
                for (cebu::token const& t : tokens)
                    format += std::format("\t`{}`\n", t);
            } else format += std::format("expected token `{}``",
                                         tokens, token());
        }(args...);
        format += std::format("intead of token `{}`", this->token());
    }
    std::cerr << format << std::endl;
}

parser& parser::unsafely_load_file(std::string_view const& file_path)
{
    std::ifstream file{file_path.begin()};
    std::ostringstream sstr;
    sstr << file.rdbuf();
    m_source = sstr.str();
    m_lexer.load(file_path, m_source.data());
    return *this;
}

template<typename ...Ts>
void syntax_parser<method_declaration, Ts...>::
    parse(parser&             parser,
          method_declaration& out)
{
    parser
        .parse<identifier, on_failure_option>(out.identifier)
        .parse<lambda_type>(out.lambda)
        .parse<body>(out.body);
}

template<typename ...Ts>
void syntax_parser<lambda_type, Ts...>::
    parse(parser&      parser,
          lambda_type& out)
{
    parser
        .parse<tuple_type>(out.tuple)
        .expect<token_type::rightwards_arrow>()
        .parse<type>(out.return_type);
}

template<typename ...Ts>
void syntax_parser<tuple_type, Ts...>::
    parse(parser& parser, tuple_type& out)
{
    bool proceed{false};
    parser
        .expect<token_type::left_parenthesis, on_success_option>([&] {
            do {
                out.mappings.resize(out.mappings.size() + 1);
                parser
                    .parse<value_declaration>(out.mappings.back())
                    .expect<std::array{
                        token_type::comma,
                        token_type::right_parenthesis
                    }, on_success_option, on_failure_option>([&] {
                        proceed = parser.token() != token_type::right_parenthesis;
                    }, [&] { proceed = false; });
            } while (proceed);
        });
}

template<typename ...Ts>
void syntax_parser<type, Ts...>::
    parse(parser& parser, type& out)
{
    parser
        .expect<std::array{
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
        }, on_success_option>([&] {
            if (parser.token() == token_type::left_parenthesis) {
                out.type = type::tuple;
                tuple_type* tuple{new tuple_type};
                parser
                    .parse<tuple_type>(*tuple)
                    .expect<
                        token_type::rightwards_arrow,
                        on_success_option, on_failure_option,
                        dont_report_option
                    >([&] {
                        out.type = type::lambda;
                        out.value.lambda = new lambda_type;
                        out.value.lambda->tuple.mappings = std::move(tuple->mappings);
                        delete tuple;
                        parser
                            .parse<type>(out.value.lambda->return_type);
                    }, [&] {
                        out.type = type::tuple;
                    });
            } else {
                out.type = type::primitive;
                out.value.primitive = static_cast<primitive_type>(
                    parser.token().type);
            }
        });
}

template<typename ...Ts>
void syntax_parser<value_declaration, Ts...>::
    parse(parser& parser, value_declaration& out)
{
    parser
        .parse<identifier>(out.identifier)
        .expect<token_type::colon>()
        .parse<type>(out.type)
        .parse<body>(out.body);
}

}
