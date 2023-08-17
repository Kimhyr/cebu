#pragma once
#include <utility>
#define CEBU_INCLUDED_PARSER_H

#include <type_traits>
#include <tuple>

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
{

};

class unexpected_token_error
    : public parsing_error
{
public:
    unexpected_token_error(token_type expected, token_type recieved) noexcept
        : m_expected{expected}
        , m_recieved{recieved}
    {
        // TODO: Error message.
    }

    token_type m_expected;
    token_type m_recieved;
};

template<token_type... Variants>
class unexpected_token_variant_error
    : public parsing_error
{
public:
    unexpected_token_variant_error(token_type recieved) noexcept
        : m_recieved{recieved}
    {
        // TODO: Error message.
    }

    token_type m_recieved;
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

    template<token_type Token>
    auto expect() -> parser&
    {
        auto token{this->consume()};
        if (token != Token)
            throw unexpected_token_error{token, Token};
        return *this;
    }

    template<token_type Token, token_type ...Tokens>
    auto expect_one_of_then(std::function<void(token_type)> fn) -> parser&
    {
        auto token{this->consume()};
        if (!this->m_expect_one_of<Token, Tokens...>())
            throw unexpected_token_variant_error<Token, Tokens...>{token};
        fn(this->token());
        return *this;
    }

    auto consume_then(std::function<void(token_type)> fn) -> parser&
    {
        this->consume();
        fn(this->token());
        return *this;
    }

    auto consume() -> token_type
    {
        this->m_lexer.next(this->m_token);
        return this->token();
    }

    template<typename Token = any_token>
    auto then(std::function<void(Token const&)> fn) -> parser&
    {
        fn(static_cast<Token>(this->token()));
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

    template<token_type Token, token_type ...Tokens>
    auto m_expect_one_of() -> bool
    {
        if (this->token() != Token)
            return false;
        if constexpr(sizeof...(Tokens) != 0)
            return m_expect_one_of<Tokens...>();
        return true;
    }
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
        enum {
            b8,
            i8
        } primitive;
    } value;
    type_type type;
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
            .then<identifier_token>([&out](auto token) -> void
            {
                out.name = token.value();
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
            .expect_one_of_then<
                token_type::b8,
                token_type::i8
            >([&out](token_type token) {
                
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
        switch (self.consume()) {
        case token_type::let:
            self.parse(out.value.let);
            break;
        default:
            throw unexpected_token_variant_error<
                token_type::let,
                token_type::if_
            >{self.token()};
        }
        out.type = static_cast<statement_type>(self.token().type());
        return self;
    }
};

}
