#pragma once
#define CEBU_INCLUDED_SYNTAX_H

#include <cebu/diagnostics.h>
#include <cebu/token.h>
#include <cebu/utilities/type_traits.h>
#include <cebu/parser.h>

namespace cebu
{

struct identifier;
enum class primitive_type;
struct value_definition;
struct method_definition;

struct identifier
{
    name name;

    template<typename ...Options>
    static void parse(parser& parser, identifier& out)
    {
        parser
            .expect<token_type::name, enable_on_success>([&] {
                out.name = parser.token();
            });
    }
};

enum class type_type
{
    primitive
};

struct type
{
    union {
        primitive_type primitive;
    } value;
    type_type type_;

    template<typename...>
    static void parse(parser&, type&);

    constexpr type(primitive_type primitive) noexcept
        : value{.primitive = primitive},
          type_{type_type::primitive}
    {}
};

constexpr type b8_type{primitive_type::b8};
constexpr type b16_type{primitive_type::b16};
constexpr type b32_type{primitive_type::b32};
constexpr type b64_type{primitive_type::b64};
constexpr type i8_type{primitive_type::i8};
constexpr type i16_type{primitive_type::i16};
constexpr type i32_type{primitive_type::i32};
constexpr type i64_type{primitive_type::i64};
constexpr type f16_type{primitive_type::f16};
constexpr type f32_type{primitive_type::f32};
constexpr type f64_type{primitive_type::f64};

template<typename Syntax>
struct reference
{
    Syntax* pointer;

    template<typename...>
    static void parse(parser&, reference&);
};

enum class expression_type
{
    equation,
    scoped_expression,
    implication,
    invocation,
    subtraction,
    disjunction,
    addition
};

struct expression
{
    union {
    } ;
};

enum class statement_type
{
    value_definition,
    method_definition,
    expression
};

struct statement
{
    union {
    } ;
};

struct body
{
    std::vector<statement> statements;

    template<typename...>
    static void parse(parser&, body&);
};

struct value_definition
{
    identifier identifier;
    reference<type> type;
    body body;

    template<typename...>
    static void parse(parser&, value_definition&);
};

struct parameters
{
    std::vector<value_definition> definitions;

    template<typename...>
    static void parse(parser&, parameters&);
};

struct method_definition
{
    identifier identifier;
    parameters parameters;
    reference<type> type;
    body body;

    template<typename ...Options>
    static void parse(parser& parser, method_definition& out)
    {
        parser
            .parse<struct identifier>(out.identifier)
            .parse<struct parameters>(out.parameters)
            .parse<reference<struct type>>(out.type)
            .parse<struct body>(out.body);
    }
};

}
