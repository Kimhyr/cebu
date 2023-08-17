#pragma once
#define CEBU_INCLUDED_PARSER_H

#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include <cebu/lexer.h>

namespace cebu
{

struct parsing_error_info
{
    std::string_view file_path;
    position         position;
    std::string_view view;
};

class parsing_error
    : public error
{
};

template<auto V>
using value_tag_t = char;

class unexpected_token_error
    : public parsing_error
{
public:
    unexpected_token_error(token_type recieved, token_type expected) noexcept
    {
        std::cerr << std::format("expected: {}, but recieved: {}", expected, recieved) << std::endl;
    }
};

class unexpected_token_variant_error
    : public parsing_error
{
public:
    template<typename ...TokenTypes>
        requires (std::is_same_v<token_type, TokenTypes>, ...)
    unexpected_token_variant_error(token_type recieved, TokenTypes... tokens) noexcept
    {
        std::string format{std::format("expected one of:\n")};
        format += (... + std::format("\t{}\n", tokens));
        format += std::format("but recieved: {}", recieved);
        std::cerr << format << std::endl;
    }
};

template<typename T>
class parser;

template<>
class parser<void>
{
public:
    template<typename Syntax>
    auto parse(Syntax& out) -> parser&
    {
        parser<Syntax>::parse(*this, out);
        return *this;
    }

    auto consume() noexcept -> parser&
    {
        m_lexer.next(m_token);
        return *this;
    }

    template<token_type Expectation>
    auto expect() -> parser&
    {
        consume();
        if (token() != Expectation)
            throw unexpected_token_error{Expectation, token()};
        return *this;
    }

    template<token_type ...Expectations>
    auto expect_one_of() -> parser&
    {
        consume();
        if (!token_is_one_of<Expectations...>()) [[unlikely]]
            throw unexpected_token_variant_error{token(), Expectations...};
        return *this;
    }

    template<token_type ...Values>
    auto token_is_one_of() const noexcept
        -> bool
    {
        return !((Values != token()) && ...);
    }

    auto then(std::function<void()> fn) -> parser&
    {
        fn();
        return *this;
    }

    [[nodiscard]]
    auto token() const noexcept -> any_token const&
    {
        return m_token;
    }

private:
    lexer     m_lexer;
    any_token m_token;
};

struct identifier
{
    std::string_view name;
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
    type_type type;

    operator primitive_type&()
    {
        return value.primitive;
    }
};

struct type_cast
{
    type type;
};

struct value_definition
{
    identifier identifier;
    type_cast  type_cast;   
};

struct let_statement
{
    identifier identifier;
    type_cast  type_cast;
};

enum class statement_type
{
    let = static_cast<int>(token_type::let)
};

struct statement
{
    union {
        let_statement let;
    } value;
    statement_type type;
};

template<>
class parser<identifier>
{
public:
    static auto parse(parser<void>& self, identifier& out) -> parser<void>&
    {
        return self
            .expect<token_type::identifier>()
            .then([&]() -> void
            {
                out.name = static_cast<identifier_token>(self.token()).value();
            });
    }
};

template<>
class parser<type>
{
public:
    static auto parse(parser<void>& self, type& out) -> parser<void>&
    {
        return self
            .consume()
            .then([&]
            {
                if (self.token() <= token_type::b8 && self.token() >= token_type::f128) {
                    out.type            = type_type::primitive;
                    out.value.primitive = self.token();
                    return;
                }
                throw unexpected_token_variant_error(
                    self.token(),
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
            });
    }
};

template<>
class parser<type_cast>
{
public:
    static auto parse(parser<void>& self, type_cast& out) -> parser<void>&
    {
        return self
            .expect<token_type::colon>()
            .parse(out.type);
    }
};

template<>
class parser<let_statement>
{
public:
    static auto parse(parser<void>& self, let_statement& out) -> parser<void>&
    {
        return self
            .parse(out.identifier)
            .parse(out.type_cast);
    }
};

template<>
class parser<statement>
{
public:
    static auto parse(parser<void>& self, statement& out) -> parser<void>&
    {
        return self;
    }
};

}
