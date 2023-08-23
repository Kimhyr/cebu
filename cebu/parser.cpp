#include <fstream>
#include <sstream>

#include "cebu/syntax.h"
#include "cebu/token.h"
#include "parser.h"

namespace cebu
{

char const* end_of_file_error::what() const noexcept
{ return "end of file"; }

template<cebu::parsing_error Error, typename ...Args>
void parser::report(Args&&... args) const noexcept
{
    std::string format{std::format("[{}] parsing error: ", this->location())};
    if constexpr(Error == cebu::parsing_error::unexpected_token) {
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

cebu::parser& parser::unsafely_load_file(std::string_view const& file_path)
{
    std::ifstream file{file_path.begin()};
    std::ostringstream sstr;
    sstr << file.rdbuf();
    m_source = sstr.str();
    m_lexer.load(file_path, m_source.data());
    return *this;
}

template<typename ...Ts>
void cebu::syntax_parser<cebu::method_declaration, Ts...>::
    parse(cebu::parser&             parser,
          cebu::method_declaration& out)
{
    parser
        .parse<cebu::identifier, cebu::on_failure_option>(out.identifier)
        .parse<cebu::lambda_type>(out.lambda)
        .parse<cebu::body>(out.body);
}

template<typename ...Ts>
void cebu::syntax_parser<cebu::lambda_type, Ts...>::
    parse(cebu::parser&      parser,
          cebu::lambda_type& out)
{
    parser
        .parse<cebu::tuple_type>(out.tuple)
        .expect<cebu::token_type::rightwards_arrow>()
        .parse<cebu::type>(out.return_type);
}

template<typename ...Ts>
void cebu::syntax_parser<cebu::tuple_type, Ts...>::
    parse(cebu::parser& parser, cebu::tuple_type& out)
{
    bool proceed{false};
    parser
        .expect<cebu::token_type::left_parenthesis, cebu::on_success_option>([&] {
            do {
                out.mappings.resize(out.mappings.size() + 1);
                parser
                    .parse<cebu::value_declaration>(out.mappings.back())
                    .expect<std::array{
                        cebu::token_type::comma,
                        cebu::token_type::right_parenthesis
                    }, cebu::on_success_option, cebu::on_failure_option>([&] {
                        proceed = parser.token() != cebu::token_type::right_parenthesis;
                    }, [&] { proceed = false; });
            } while (proceed);
        });
}

template<typename ...Ts>
void cebu::syntax_parser<cebu::type, Ts...>::
    parse(cebu::parser& parser, cebu::type& out)
{
    parser
        .expect<std::array{
            cebu::token_type::b8,
            cebu::token_type::b16,
            cebu::token_type::b32,
            cebu::token_type::b64,
            cebu::token_type::i8,
            cebu::token_type::i16,
            cebu::token_type::i32,
            cebu::token_type::i64,
            cebu::token_type::f16,
            cebu::token_type::f32,
            cebu::token_type::f64,
            cebu::token_type::left_parenthesis
        }, cebu::on_success_option>([&] {
            if (parser.token() == cebu::token_type::left_parenthesis) {
                out.type = cebu::type::tuple;
                cebu::tuple_type* tuple{new cebu::tuple_type};
                parser
                    .parse<cebu::tuple_type>(*tuple)
                    .expect<
                        cebu::token_type::rightwards_arrow,
                        cebu::on_success_option, cebu::on_failure_option,
                        cebu::dont_report_option
                    >([&] {
                        out.type = cebu::type::lambda;
                        out.value.lambda = new cebu::lambda_type;
                        out.value.lambda->tuple.mappings = std::move(tuple->mappings);
                        delete tuple;
                        parser
                            .parse<cebu::type>(out.value.lambda->return_type);
                    }, [&] {
                        out.type = cebu::type::tuple;
                    });
            } else {
                out.type = cebu::type::primitive;
                out.value.primitive = static_cast<cebu::primitive_type>(
                    parser.token().type);
            }
        });
}

template<typename ...Ts>
void cebu::syntax_parser<cebu::value_declaration, Ts...>::
    parse(cebu::parser& parser, cebu::value_declaration& out)
{
    parser
        .parse<cebu::identifier>(out.identifier)
        .expect<cebu::token_type::colon>()
        .parse<cebu::type>(out.type)
        .parse<cebu::body>(out.body);
}

}
