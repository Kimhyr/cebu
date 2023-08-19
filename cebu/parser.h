#pragma once
#define CEBU_INCLUDED_PARSER_H

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>

#include <concepts>

#include <cebu/syntax.h>

namespace cebu
{

namespace detail
{

template<typename Needle, typename Current, typename ...Haystack>
struct find_type
    : std::conditional_t<
        std::same_as<Needle, Current>,
        Needle,
        std::conditional_t<
            sizeof...(Haystack) != 0,
            find_type<Needle, Haystack...>,
            void
        >
    >
{};
    
}

template<typename Needle, typename ...Haystack>
struct find_type : detail::find_type<Needle, Haystack...> {};

template<typename Needle, typename ...Haystack>
constexpr auto find_type_v = find_type<Needle, Haystack...>::value;

template<typename Needle, typename ...Haystack>
constexpr auto get_option_v = find_type_v<Needle, Haystack...>;

template<auto V>
static constexpr bool is_token_array_v =
    std::is_array_v<decltype(V)>
    && std::is_same_v<token_type, typename decltype(V)::value_type>;

struct ignore_error : basic_option {};
struct enable_on_error : basic_option {};
struct enable_on_success : basic_option {};

class parser;

template<typename T, typename ...Options>
concept parsable =
    requires(parser& self, T& out) {
        { T::template parse<Options...>(self, out) } -> std::same_as<void>;
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
            error : 1,
            padding : 31;
    };

    parser(std::string_view file_path)
        : m_file_path{file_path}
    {
        unsafely_load_file();
    }

    ~parser()
    {
        unload_file();
    }

    parser& consume();
    parser& peek(token& out);

    template<auto Token, option ...Options>
        requires std::is_same_v<decltype(Token), token_type>
            || std::is_same_v<decltype(Token), token_category>
    parser& consume_until(
        std::function<void()> on_success = do_nothing,
        std::function<void()> on_error = do_nothing
    )
    {
        for (;;) {
            class token token;
            peek<Options...>(token);                
            token.discard();
            if constexpr(std::is_same_v<decltype(Token), token_category>) {
                if (token.is_of<Token>()) {
                    if constexpr(get_option_v<enable_on_success, Options...>)
                        on_success();
                    return *this;
                }
            } else if constexpr(token == Token) {
                if constexpr(get_option_v<enable_on_success, Options...>)
                    on_success();
                return *this;
            }
        }
        if constexpr(get_option_v<enable_on_error, Options...>)
            resolve_error(on_error);
        return *this;
    }

    template<auto Tokens, option ...Options>
        requires is_token_array_v<Tokens>
    parser& consume_until_one_of(
        std::function<void()> on_error = do_nothing
    ) {
        for (;;) {
            class token token;
            peek(token);
            token.discard();
            if (token == Tokens)
                return *this;
            consume();
        }
        if constexpr(get_option_v<enable_on_error, Options...>)
            resolve_error(on_error);
        return *this;
    }

    template<parsable Syntax, option ...Options>
    parser& parse(
        Syntax& out,
        std::function<void()> on_success = do_nothing,
        std::function<void()> on_error = do_nothing
    ) {
        Syntax::template parse<Options...>(*this, out);
        if constexpr(!get_option_v<ignore_error, Options...>) {
            if (error()) {
                if constexpr(get_option_v<enable_on_error, Options...>) {
                    // use the first function if `on_success` is disabled.
                    if constexpr(get_option_v<enable_on_success, Options...>)
                        resolve_error(on_error);
                    else on_success();
                }
            }
        } else if constexpr(get_option_v<enable_on_success, Options...>)
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
        if (error()) [[unlikely]] {
            fn();
            clear_error();
        }
        return *this;
    }

    template<token_type Token, option ...Options>
    parser& expect(
        std::function<void()> on_success = do_nothing,
        std::function<void()> on_error = do_nothing
    )
    {
        consume<Options...>();
        if (token() != Token) [[unlikely]] {
            report_error<parsing_error::unexpected_token>();
            if constexpr(get_option_v<enable_on_error, Options...>)
                on_error();
        } else if constexpr(get_option_v<enable_on_success, Options...>)
            on_success();
        return *this;
    }

    template<auto Tokens, option ...Options>
        requires is_token_array_v<Tokens>
    parser& expect_one_of(
        std::function<void()> on_success = do_nothing,
        std::function<void()> on_error = do_nothing
    )
    {
        consume<Options...>();
        if (token() != Tokens) [[unlikely]] {
            report_error<parsing_error::unexpected_token_variant>();
            if constexpr(get_option_v<enable_on_error, Options...>)
                resolve_error(on_error);
            else if constexpr(!get_option_v<ignore_error, Options...>)
                set_error();
        } else if constexpr(get_option_v<enable_on_success, Options...>)
            on_success();
        return *this;
    }

    [[nodiscard]]
    flags& flags() noexcept
    {
        return m_flags;
    }

    [[nodiscard]]
    bool error() const noexcept
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

    template<parsing_error Error>
    void report_error() const noexcept;

    parser& load_file()
    {
        unload_file();
        unsafely_load_file();
        return *this;
    }

    parser& unload_file()
    {
        if (m_source.empty())
            munmap(m_source.data(), file_size());
        return *this;
    }

    [[nodiscard]]
    token const& token() const noexcept
    {
        return m_token;
    }

    [[nodiscard]]
    position position() const noexcept
    {
        return {
            .row = m_row_number,
            .column = static_cast<std::size_t>(m_pointer - m_row)
        };
    }

    [[nodiscard]]
    std::size_t file_size() const noexcept
    {
        return m_source.size();
    }

private:       
    std::string m_source;
    std::string::const_iterator m_pointer;
    std::string::const_iterator m_row;
    std::string_view m_file_path;
    std::size_t m_row_number;
    class token m_token;
    struct flags m_flags;
    int m_scope_depth;

    void unsafely_load_file();
};

}
