#pragma once
#define CEBU_INCLUDED_SYNTAX_H

#include <cebu/diagnostics.h>
#include <cebu/token.h>
#include <cebu/utilities/type_traits.h>

namespace cebu
{

class parser;

struct identifier
{
    name name;

    template<typename...>
    static void parse(parser&, identifier&);
};

}

template<>
struct std::formatter<cebu::identifier>
    : formatter<string>
{
    auto format(cebu::identifier const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "{}",
            self.name
        ), ctx);
    }
};

namespace cebu
{

struct type
{
    union {
        primitive_type primitive;
    } value;
    enum : int {
        primitive
    } variant;

    using type_t = unsigned char;

    template<typename...>
    static void parse(parser&, type&);

    constexpr type(primitive_type primitive) noexcept
        : value{.primitive = primitive}
        , variant{type::primitive}
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
struct path
{
    template<typename...>
    static void parse(parser&, path&);
};

struct body
{
       
    template<typename...>
    static void parse(parser&, body&);
};

struct value_definition
{
    identifier identifier;
    path<type> type;
    body body;

    template<typename...>
    static void parse(parser&, value_definition&);
};

}

template<>
struct std::formatter<cebu::value_definition>
    : formatter<string>
{
    auto format(cebu::value_definition const& self, format_context& ctx)
    {
        std::string format{std::format(
            "{}: {} ",
            self.identifier, self.type
        )};
        if (self.body.has_value()) {
        }
        return formatter<string>::format(std::format(
            "{}", format
        ), ctx);
    }
};

namespace cebu
{

struct parameters
{
    std::vector<value_definition> definitions;

    template<typename...>
    static void parse(parser&, parameters&);
};

}

template<>
struct std::formatter<cebu::parameters>
    : formatter<string>
{
    auto format(cebu::parameters const& self, format_context& ctx)
    {
        std::string format{'('};
        if (!self.definitions.empty()) {
            format += std::format("{}", self.definitions[0]);
            for (std::size_t i{1}; i < self.definitions.size(); ++i)
                format += std::format(", {}", self.definitions[i]);
        }
        return formatter<string>::format(std::format(
            "{})", format
        ), ctx);
    }
};

namespace cebu
{

struct method_definition
{
    identifier identifier;
    parameters parameters;
    std::optional<path<type>> type;
    std::optional<body> body;

    template<typename...>
    static void parse(parser&, method_definition&);
};

}

template<>
struct std::formatter<cebu::method_definition>
    : formatter<string>
{
    auto format(cebu::method_definition const& self, format_context& ctx) const
    {
        std::string format{std::format(
            "method {} ({})",
            self.identifier, self.parameters
        )};
        if (self.type.has_value())
            format += std::format(", ");
        if (self.body.has_value())
            format += std::format(", ");
        return formatter<string>::format(format += ')', ctx); }
};

