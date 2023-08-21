#include "syntax.h"

#include <cebu/diagnostics.h>
#include <cebu/parser.h>

namespace cebu
{

template<typename ...Options>
void identifier::parse(parser& parser, identifier& out)
{
    parser
        .expect<token_type::name, enable_on_success>([&] {
            out.name = parser.token();
        });
}

template<typename ...Options>
void method_definition::parse(parser& parser, method_definition& out)
{
    parser
        .parse<struct identifier>(out.identifier);
}

}
