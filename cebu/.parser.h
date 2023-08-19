#pragma once
#define CEBU_INCLUDED_PARSER_H

#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include <cebu/lexer.h>

namespace cebu
{


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

template<typename Syntax>
struct syntax_parser;

class parser
{
public:
    static std::unordered_map<std::string_view, token_type> const word_map;

    parser(source_file const& file)
        : m_file{&file}
        , m_pointer{m_file->map()}
        , m_row{m_pointer}
        , m_row_number{1}
    {}
        
    template<typename Syntax>
    auto parse(Syntax& out) -> parser&
    {
        syntax_parser<Syntax>::parse(*this, out);
        return *this;
    }

    auto consume() noexcept -> parser&
    {
        lex(m_token);
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

    [[nodiscard]]
    auto file() const noexcept -> source_file const&
    {
        return *m_file;
    }

    [[nodiscard]]
    auto position() const noexcept -> position
    {
        return {
            .row    = m_row_number,
            .column = static_cast<std::size_t>(m_pointer - m_row)
        };
    }


private:
    source_file const* m_file;
    char const*        m_pointer;
    char const*        m_row;
    std::size_t        m_row_number;
    any_token          m_token;

    auto lex(any_token& out) -> void;

    void increment() noexcept
    {
        if (*m_pointer == '\n')
            m_row = m_pointer;
        ++m_pointer;
    }

    char token() const noexcept
    {
        return *m_pointer;
    }

    char peek() const noexcept
    {
        return m_pointer[1];
    }

    bool next_escaped_character();
};
template<>
struct syntax_parser<identifier>
{
    static auto parse(parser& self, identifier& out) -> parser&
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
struct syntax_parser<type>
{
    static auto parse(parser& self, type& out) -> parser&
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
struct syntax_parser<type_cast>
{
    static auto parse(parser& self, type_cast& out) -> parser&
    {
        return self
            .expect<token_type::colon>()
            .parse(out.type);
    }
};

template<>
struct syntax_parser<let_statement>
{
    static auto parse(parser& self, let_statement& out) -> parser&
    {
        return self
            .parse(out.identifier)
            .parse(out.type_cast);
    }
};

template<>
struct syntax_parser<statement>
{
    static auto parse(parser& self, statement& out) -> parser&
    {
        return self;
    }
};

}
