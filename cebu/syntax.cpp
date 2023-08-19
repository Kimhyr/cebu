#include "syntax.h"

#include <cebu/diagnostics.h>
#include <cebu/parser.h>

namespace cebu
{

template<option ...Options>
void value_definition::parse(parser& parser, value_definition& out)
{
    parser
        .parse<struct identifier, ignore_error>(out.identifier)
        .parse<struct path<struct type>, ignore_error>(out.type)
        .parse<struct body, enable_on_error>(out.body.value(), [&] {
            parser.consume_until<token_category::determiner>();
        });
}

template<option ...Options>
void parameters::parse(parser& parser, parameters& out)
{
    auto stop{false};
    do {
        out.definitions.resize(out.definitions.size() +  1);
        parser
            .parse<value_definition, enable_on_error>(
                out.definitions.back(), [&] {
                out.definitions.pop_back();
            })
            .template expect_one_of<
                std::array{token_type::comma, token_type::left_curly_bracket},
                enable_on_success, enable_on_error
            >([&]() {
                stop = parser.token() == token_type::right_parenthesis;
            }, [&] {
                stop = true;
            });
    } while (stop);
}

template<option ...Options>
void method_definition::parse(parser& parser, method_definition& out)
{
    parser
        .parse<struct identifier, ignore_error>(out.identifier)
        .parse<struct parameters, ignore_error>(out.parameters.value())
        .parse<path<struct type>, ignore_error>(out.type.value())
        .parse<struct body, enable_on_error>(
            out.body.value(), [&] {
            parser.consume_until<token_category::determiner>();
        });
}

}
