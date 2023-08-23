#pragma once
#define CEBU_INCLUDED_LEXER_H

#include <unordered_map>

#include <cebu/token.h>
#include <cebu/diagnostics.h>

namespace cebu
{

/// `lexer` - An incrementable token lexer.
///
/// This acts like 
class lexer
{
    friend class parser;

public:
    lexer() noexcept
    {}

    /// `load` - Loads the file at `file_path` and begins the cursor at
    /// `pointer`
    void load(std::string_view file_path, char const* pointer) noexcept
    {
        m_file_path = file_path;
        m_cursor.pointer = pointer;
        m_cursor.line_pointer = pointer;
        m_cursor.line_number = 1;
    }

    /// `lex` - Lexes a token into `token`.
    result lex(token& token) noexcept;

    /// `position` - Returns the position of the cursor.
    [[nodiscard]]
    position position() const noexcept
    {
        return {
            line_number(),
            static_cast<std::size_t>(pointer() - line_pointer())
        };
    }

    /// `location` - Returns the location of the cursor.
    [[nodiscard]]
    location location() const noexcept
    {
        return {
            file_path(),
            position()
        };
    }

    /// `get_keywords` - Returns the table of string-keyword pairs.
    [[nodiscard]]
    static std::unordered_map<std::string_view, token_type> const&
        get_keywords() noexcept
    {
        [[clang::no_destroy]]
        static std::unordered_map<std::string_view, token_type> keywords {
            {"b8"    , token_type::b8},
            {"b16"   , token_type::b16},
            {"b32"   , token_type::b32},
            {"b64"   , token_type::b64},
            {"i8"    , token_type::i8},
            {"i16"   , token_type::i16},
            {"i32"   , token_type::i32},
            {"i64"   , token_type::i64},
            {"f16"   , token_type::f16},
            {"f32"   , token_type::f32},
            {"f64"   , token_type::f64},
            {"method", token_type::method},
            {"trait" , token_type::trait},
            {"type"  , token_type::type},
            {"static", token_type::static_},
            {"let"   , token_type::let},
            {"if"    , token_type::if_},
            {"else"  , token_type::else_},
            {"elif"  , token_type::elif},
            {"return", token_type::return_},
        };
        return keywords;
    }

private:
    enum class error
    {
       incomplete_character,
       unknown_character,
       number_overflow,
       unknown_escaped_character,
       multiple_decimal_points
    };

    struct cursor
    {
        char const* pointer{nullptr};
        char const* line_pointer{pointer};
        std::size_t line_number{1}; 
    };

    std::string_view m_file_path;
    cursor           m_prior_cursor;
    cursor           m_cursor;

    enum class character_result
    {
        failure = -1,
        regular = 0,
        escaped = 1
    };

    template<error Error, typename ...Args>
    void report(struct position const&, Args&&...);

    character_result lex_escaped_character() noexcept;

    void consume() noexcept
    {
        if (current() == '\0') [[unlikely]]
            return;
        ++m_cursor.pointer;
        if (pointer()[-1] == '\n') [[unlikely]]
            m_cursor.line_pointer = pointer();
    }

    [[nodiscard]]
    std::string_view const& file_path() const noexcept
    { return m_file_path; }

    [[nodiscard]]
    char const* pointer() const noexcept
    { return m_cursor.pointer; }

    [[nodiscard]]
    char const* line_pointer() const noexcept
    { return m_cursor.line_pointer; }

    [[nodiscard]]
    char current() const noexcept
    { return *pointer(); }

    [[nodiscard]]
    char peek() const noexcept
    { return pointer()[1]; }

    [[nodiscard]]
    std::size_t line_number() const noexcept
    { return m_cursor.line_number; }
};

}
