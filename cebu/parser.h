#pragma once
#define CEBU_INCLUDED_PARSER_H

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>

#include <concepts>

#include <cebu/lexer.h>
#include <cebu/syntax.h>
#include <cebu/utilities/type_traits.h>

namespace cebu
{

enum class syntax_type
{
    method_declaration
};

struct do_throw {};
struct dont_throw {};
struct ignore_error {};
struct enable_on_error {};
struct enable_on_success {};
struct dont_report {};

class end_of_file_error 
    : public std::out_of_range
{
public:
    using base_type = std::out_of_range;

    end_of_file_error() noexcept
        : base_type{"end of file"}
    {}

    [[nodiscard]]
    char const* what() const noexcept override;
};

class parser;

template<typename T, typename ...Ts>
struct syntax_parser;

template<typename T, typename ...Ts>
concept parsable =
    requires(parser& self, T& out) {
        { syntax_parser<T, Ts...>::parse(self, out) } -> std::same_as<void>;
    };

enum class parsing_error
{
    unexpected_token,
    unexpected_token_variant
};

inline auto do_nothing() -> void {}

class parser
{
public:
    struct flags
    {
        std::uint32_t
            error : 1 = false,
            padding : 31;
    };

    parser() = default;

    ~parser() = default;

    /// Lexes a token.
    template<typename ...Options>
    parser& consume() noexcept(has_type_v<dont_throw, Options...>)
    {
        lex(m_token);
        return *this;
    }

    /// Peeks a token
    template<typename ...Options>
    parser& peek(
            token& out,
            std::function<void()> then = do_nothing)
    {
        lex(out);
        m_lexer.revert();
        if constexpr(has_type_v<enable_on_success, Options...>)
        return *this;
    }

    /// Consumes until the current token is equavalent to `Token` or is of the
    /// token category `Token`.
    template<auto Token, typename ...Options>
    parser& consume_to(std::function<void()> on_success = do_nothing,
                       std::function<void()> on_error = do_nothing)
    {
        for (;;) {
            consume<Options...>();
            token().discard();
            if constexpr(token() == Token) {
                if constexpr(has_type_v<enable_on_success, Options...>)
                    on_success();
                return *this;
            }
        }
        if constexpr(has_type_v<enable_on_error, Options...>)
            resolve_error(on_error);
        else if constexpr(!has_type_v<ignore_error, Options...>)
            set_error();
        return *this;
    }

    template<parsable Syntax, typename ...Options>
    parser& parse(Syntax& out,
                  std::function<void()> on_success = do_nothing,
                  std::function<void()> on_error = do_nothing)
    {
        syntax_parser<Syntax>::parse(*this, out);
        if constexpr(!has_type_v<ignore_error, Options...>) {
            if (has_error()) {
                if constexpr(has_type_v<enable_on_error, Options...>) {
                    // use the first function if `on_success` is disabled.
                    if constexpr(has_type_v<enable_on_success, Options...>)
                        resolve_error(on_error);
                    else on_success();
                } else if constexpr(!has_type_v<ignore_error, Options...>)
                    set_error();
            }
        } else if constexpr(has_type_v<enable_on_success, Options...>)
            on_success();
        return *this;
    }

    parser& then(std::function<void()> fn)
    {
        fn();
        return *this;
    }

    parser& resolve_error(std::function<void()> fn)
    {
        if (has_error()) [[unlikely]] {
            fn();
            clear_error();
        }
        return *this;
    }

    template<auto Token, typename ...Options>
    parser& expect(std::function<void()> on_success = do_nothing,
                   std::function<void()> on_error = do_nothing)
    {
        consume<Options...>();
        if (token() != Token) [[unlikely]] {
            if constexpr(!has_type_v<dont_report, Options...>)
                report<parsing_error::unexpected_token>(Token);
            if constexpr(has_type_v<enable_on_error, Options...>)
                resolve_error(on_error);
            else if constexpr(!has_type_v<ignore_error, Options...>)
                set_error();
        } else if constexpr(has_type_v<enable_on_success, Options...>)
            on_success();
        return *this;
    }

    template<auto Tokens, typename ...Options>
    parser& expect_one_of(std::function<void()> on_success = do_nothing,
                          std::function<void()> on_error = do_nothing)
    {
        consume<Options...>();
        if (token() != Tokens) [[unlikely]] {
            if constexpr(!has_type_v<dont_report, Options...>)
                report<parsing_error::unexpected_token_variant>(Tokens);
            if constexpr(has_type_v<enable_on_error, Options...>)
                resolve_error(on_error);
            else if constexpr(!has_type_v<ignore_error, Options...>)
                set_error();
        } else if constexpr(has_type_v<enable_on_success, Options...>)
            on_success();
        return *this;
    }

    template<auto Tokens, typename ...Options>
    parser& peek_one_of(std::function<void()> on_success = do_nothing,
                        std::function<void()> on_error = do_nothing)
    {
    }

    [[nodiscard]]
    flags& flags() noexcept
    {
        return m_flags;
    }

    [[nodiscard]]
    bool has_error() const noexcept
    {
        return m_flags.error;
    }

    parser& set_error() noexcept
    {
        m_flags.error = true;
        return *this;
    }

    parser& clear_error() noexcept
    {
        m_flags.error = false;
        return *this;
    }

    void load(std::string_view const& file_path)
    {
        unload();
        unsafely_load_file(file_path);
    }

    location location() const noexcept
    {
        return lexer().location();
    }

    parser& unload()
    {
        m_source.resize(0);
        return *this;
    }

    [[nodiscard]]
    token const& token() const noexcept
    {
        return m_token;
    }

    [[nodiscard]]
    std::size_t file_size() const noexcept
    {
        return m_source.size();
    }

    [[nodiscard]]
    std::string const& source() const noexcept
    {
        return m_source;
    }

    [[nodiscard]]
    lexer const& lexer() const noexcept
    {
        return m_lexer;
    }

    [[nodiscard]]
    std::string_view const& file_path() const noexcept
    {
        return lexer().file_path();
    }

private:       
    std::string m_source;
    class lexer m_lexer;
    class token m_token;
    struct flags m_flags;
    int m_scope_depth{0};


    template<typename ...Options>
    void lex(class token& out) noexcept(has_type_v<dont_throw, Options...>)
    {
        if (token() == token_type::end) [[unlikely]] {
            if constexpr(has_type_v<dont_throw, Options...>)
                return;
            throw end_of_file_error{};
        }
        if (!m_lexer.lex(out)) [[unlikely]] {
            if constexpr(!has_type_v<ignore_error, Options...>)
                m_flags.error = true;
        }
    }

    template<parsing_error Error, typename ...Args>
    void report(Args&&... args) const noexcept;

    void unsafely_load_file(std::string_view const& file_path);
};

template<typename ...Ts>
struct syntax_parser<identifier, Ts...>
{
    static void parse(parser& parser, identifier& out);
};

template<typename ...Ts>
struct syntax_parser<body, Ts...>
{
    static void parse(parser& parser, body& out);
};

template<typename ...Ts>
struct syntax_parser<type, Ts...>
{
    static void parse(parser&, type&);
};

template<typename ...Ts>
struct syntax_parser<value_declaration, Ts...>
{
    static void parse(parser& parser, value_declaration& out);
};

template<typename ...Ts>
struct syntax_parser<tuple_type, Ts...>
{
    static void parse(parser& parser, tuple_type& out);
};

template<typename ...Ts>
struct syntax_parser<lambda_type, Ts...>
{
    static void parse(parser& parser, lambda_type& out);
};

template<typename ...Ts>
struct syntax_parser<method_declaration, Ts...>
{
    static void parse(parser& parser, method_declaration& out);
};

}
