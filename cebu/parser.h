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

struct nothrow_option {};
struct ignore_failure_option {};
struct on_failure_option {};
struct on_success_option {};
struct dont_report_option {};

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
    unexpected_token
};

struct parser_flags
{
    unsigned char
        failed   : 1 = false,
        padding : 7;
};

inline void do_nothing() {}

class parser
{
public:
    parser() = default;
    ~parser() = default;

    /// `parse` - Parses a `Syntax`.
    template<parsable Syntax, typename ...Opts>
    parser& parse(Syntax& out,
                        std::function<void()> on_success = do_nothing,
                        std::function<void()> on_failure = do_nothing)
        noexcept(find_type_v<nothrow_option, Opts...>)
    {
        syntax_parser<Syntax, Opts...>::parse(*this, out);
        if (this->failed()) [[unlikely]] {
            if constexpr(find_type_v<on_failure_option, Opts...>) {
                if constexpr(find_type_v<on_success_option, Opts...>)
                    this->resolve_failure(on_failure);
                else this->resolve_failure(on_success);
            } else if constexpr(!find_type_v<ignore_failure_option, Opts...>)
                this->set_failed();
        } else if constexpr(find_type_v<on_success_option, Opts...>)
            on_success();
        return *this;
    }

    /// `consume` - Consumes the current token.
    /// 
    /// # Notes
    ///
    /// If the `peek` was previously invoked, the options are ignored.
    template<typename ...Opts>
    parser& consume(std::function<void()> on_success = do_nothing,
                          std::function<void()> on_failure   = do_nothing)
        noexcept(find_type_v<nothrow_option, Opts...>)
    {
        if (this->token() == token_type::end) [[unlikely]] {
            if constexpr(find_type_v<nothrow_option, Opts...>)
                return *this;
            throw end_of_file_error{};
        }
        if (this->m_lexer.lex(m_token)) [[unlikely]] {
            if constexpr(find_type_v<on_success_option, Opts...>)
                on_success();
        } else if constexpr(find_type_v<on_failure_option, Opts...>) {
            if constexpr(find_type_v<on_success_option, Opts...>)
                resolve_failure(on_failure);
            else resolve_failure(on_success);
        } else if constexpr(!find_type_v<ignore_failure_option, Opts...>)
            this->set_failed();
        return *this;
    }

    /// `expect` - Consumes and, if the consumed token is not equavalent to
    /// `Token`, the "failure" flag is set.
    template<auto Token, typename ...Opts>
    parser& expect(std::function<void()> on_success = do_nothing,
                         std::function<void()> on_failure = do_nothing)
    {
        this->consume<Opts...>();
        if (this->token() == Token) [[likely]] {
            if constexpr(find_type_v<on_success_option, Opts...>)
                on_success();
        } else {
            if constexpr(!find_type_v<dont_report_option, Opts...>)
                this->report<parsing_error::unexpected_token>(Token);
            if constexpr(find_type_v<on_failure_option, Opts...>)
                this->resolve_failure(on_failure);
            else if constexpr(!find_type_v<ignore_failure_option, Opts...>)
                this->set_failed();
        }
        return *this;
    }

    /// `peek` - Gives the current token then consumes the current token.
    template<typename ...Opts>
    parser& peek(token&          token,
                       std::function<void()> on_success = do_nothing,
                       std::function<void()> on_failure = do_nothing)
        noexcept(find_type_v<nothrow_option, Opts...>)
    {
        token = this->token();
        return this->consume<Opts...>(on_success, on_failure);
    }

    /// `then` - Invokes `fn`.
    parser& then(std::function<void()> fn)
    {
        fn();
        return *this;
    }

    /// `resolve_failure` - Invokes `fn` and unsets the "failed" flags.
    parser& resolve_failure(std::function<void()> fn)
    {
        fn();
        this->unset_failed();
        return *this;
    }

    /// `load` - Unloads then loads the file at `file_path`.
    parser& load(std::string_view const& file_path)
    { return this->unload().unsafely_load_file(file_path); }

    /// `unload` - Unloads the source.
    parser& unload()
    {
        this->m_source.resize(0);
        return *this;
    }

    /// `failed` - Returns the "failed" flag.
    [[nodiscard]]
    bool failed() const noexcept
    { return m_flags.failed; }

    /// `get_failed` - Same as `parser::failed` but returns to `failed`.
    parser& get_failed(bool& failed) noexcept
    {
        failed = this->failed();
        return *this;
    }

    /// `set_failed` - Sets the "failed" flag to true.
    parser& set_failed() noexcept
    {
        this->m_flags.failed = true;
        return *this;
    }

    /// `unset_failed` - Unsets the "failed" flag to false.
    parser& unset_failed() noexcept
    {
        this->m_flags.failed = false;
        return *this;
    }

    /// `location` - Returns the location of the cursor.
    location location() const noexcept
    { return this->m_lexer.location(); }

    /// `token` - Returns the current token.
    [[nodiscard]]
    token const& token() const noexcept
    { return this->m_token; }

    /// `source` - Returns the contents of the source.
    [[nodiscard]]
    std::string const& source() const noexcept
    { return this->m_source; }

    /// `file_path` - Returns the source's file path.
    [[nodiscard]]
    std::string_view const& file_path() const noexcept
    { return this->m_lexer.file_path(); }

private:       
    std::string  m_source;
    lexer        m_lexer;
    cebu::token  m_token;
    parser_flags m_flags;
    int          m_scope_depth{0};

    template<parsing_error Error, typename ...Args>
    void report(Args&&... args) const noexcept;

    parser& unsafely_load_file(std::string_view const& file_path);
};

//
// Parsers
//

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
