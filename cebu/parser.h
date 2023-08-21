#pragma once
#include <concepts>
#define CEBU_INCLUDED_PARSER_H

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>

#include <cebu/lexer.h>
#include <cebu/syntax.h>
#include <cebu/utilities/type_traits.h>

namespace cebu
{

template<auto V>
static constexpr bool is_token_array_v =
    std::is_array_v<decltype(V)>
    && (std::is_same_v<token_type, typename decltype(V)::value_type>
        || std::is_same_v<token_category, typename decltype(V)::value_type>);

struct do_throw {};
struct dont_throw {};
struct ignore_error {};
struct enable_on_error {};
struct enable_on_success {};

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

template<typename T, typename ...Options>
concept parsable =
    requires(parser& self, T& out) {
        { T::template parse<Options...>(self, out) } -> std::same_as<void>;
    };

enum class parsing_error
{
    unexpected_token,
    incomplete_character_error,
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
        if (token() == token_type::end) [[unlikely]] {
            if constexpr(has_type_v<dont_throw, Options...>)
                return *this;
            throw end_of_file_error{};
        }
        if (!m_lexer.lex(m_token))
            m_flags.error = true;
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
        return *this;
    }

    template<parsable Syntax, typename ...Options>
    parser& parse(Syntax& out,
                  std::function<void()> on_success = do_nothing,
                  std::function<void()> on_error = do_nothing)
    {
        Syntax::template parse<Options...>(*this, out);
        if constexpr(!has_type_v<ignore_error, Options...>) {
            if (has_error()) {
                if constexpr(has_type_v<enable_on_error, Options...>) {
                    // use the first function if `on_success` is disabled.
                    if constexpr(has_type_v<enable_on_success, Options...>)
                        resolve_error(on_error);
                    else on_success();
                }
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
            report_error<parsing_error::unexpected_token>();
            if constexpr(has_type_v<enable_on_error, Options...>)
                on_error();
        } else if constexpr(has_type_v<enable_on_success, Options...>)
            on_success();
        return *this;
    }

    template<auto Tokens, typename ...Options>
        requires is_token_array_v<Tokens>
    parser& expect_one_of(std::function<void()> on_success = do_nothing,
                          std::function<void()> on_error = do_nothing)
    {
        consume<Options...>();
        if (token() != Tokens) [[unlikely]] {
            report_error<parsing_error::unexpected_token_variant>();
            if constexpr(has_type_v<enable_on_error, Options...>)
                resolve_error(on_error);
            else if constexpr(!has_type_v<ignore_error, Options...>)
                set_error();
        } else if constexpr(has_type_v<enable_on_success, Options...>)
            on_success();
        return *this;
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

    template<parsing_error Error, typename ...Args>
    parser& report_error(Args... args) const noexcept;

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

private:       
    std::string m_source;
    class lexer m_lexer;
    class token m_token;
    struct flags m_flags;
    int m_scope_depth{0};

    template<parsing_error Error, typename ...Args>
    void report(Args&&... args) const noexcept
    {
        discard(args...);
        std::string format{std::format("[{}] parsing error: ", location())};
        std::cerr << format << std::endl;
    }

    void unsafely_load_file(std::string_view const& file_path);
};

}
