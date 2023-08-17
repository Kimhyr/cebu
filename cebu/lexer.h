#pragma once
#define CEBU_INCLUDED_LEXER_H

#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <cebu/diagnostics.h>

namespace cebu
{

enum class token_type
{
    identifier = std::numeric_limits<int>::min(),

    // Literals
    number,    // +digit
    character, // ''' character '''
    string,    // '"' +[character] '"'

    // Words
    fn,    // 'fn'
    let,   // 'let'
    if_,   // 'if'
    else_, // 'else'
    elif,  // 'elif'
    ret,   // 'ret'
    b8,    // 'b8',
    b16,   // 'b16',
    b32,   // 'b32',
    b64,   // 'b64',
    b128,  // 'b128'
    i8,    // 'i8',
    i16,   // 'i16',
    i32,   // 'i32',
    i64,   // 'i64',
    i128,  // 'i128'
    f16,   // 'f16',
    f32,   // 'f32',
    f64,   // 'f64',
    f128,  // 'f128'

    // Multi-Character Punctuators
    double_equals_sign      = '=' + '=' + '\x7f',
    rightwards_double_arrow = '=' + '>' + '\x7f',
    double_plus_sign        = '+' + '+' + '\x7f',
    double_minus_sign       = '-' + '-' + '\x7f',
    rightwards_arrow        = '-' + '>' + '\x7f',
    double_vertical_line    = '|' + '|' + '\x7f',

    end = 0,

    quotation_mark      = '\"',
    apostrophe          = '\'',
    equals_sign         = '=',
    plus_sign           = '+',
    minus_sign          = '-',
    vertical_line       = '|',
    commercial_at       = '@',
    colon               = ':',
    semicolon           = ';',
    left_parenthesis    = '(',
    right_parenthesis   = ')',
    left_angle_bracket  = '<',
    right_angle_bracket = '>',
    left_curly_bracket  = '{',
    right_curly_bracket = '}',
    asterisk            = '*',
    slash               = '/',
    percent_sign        = '%'
};

class any_token;

template<typename Value>
class basic_token
{
public:
    using value_type = Value;

    basic_token(any_token const& token) noexcept;

    [[nodiscard]]
    auto value() const noexcept -> value_type const&
    {
        return m_value;
    }

    [[nodiscard]]
    auto position() const noexcept -> position const&
    {
        return m_position;
    }

private:
    struct position m_position;
    value_type      m_value;
    std::array<
        std::byte,
        std::conditional_t<
            std::is_same_v<value_type, char>,
            std::integral_constant<int, 7>,
            std::conditional_t<
                std::is_same_v<value_type, std::string_view>
                || std::is_same_v<value_type, std::size_t>,
                std::integral_constant<int, 8>,
                std::integral_constant<int, 4>
            >
        >::value
    > m_padding;
};

class identifier_token
    : public basic_token<std::string_view>
{
public:
    using base_type = basic_token<std::string_view>;
        
    identifier_token(any_token const& token)
        : base_type{token}
    {}
};

class number_token
    : public basic_token<std::size_t>
{
public:
    using base_type = basic_token<std::size_t>;
       
    number_token(any_token const& token)
        : base_type{token}
    {}
};

class character_token
    : public basic_token<char>
{
public:       
    using base_type = basic_token<char>;

    character_token(any_token const& token)
        : base_type{token}
    {}
};

class string_token
    : public basic_token<std::string_view>
{
public:       
    using base_type = basic_token<std::string_view>;

    string_token(any_token const& token)
        : base_type{token}
    {}
};

enum class keyword_token_type
{
    fn = static_cast<int>(token_type::fn),
    let,
    if_,
    else_,
    elif,
    ret,
    b8,
    b16,
    b32,
    b64,
    b128,
    i8,
    i16,
    i32,
    i64,
    i128,
    f16,
    f32,
    f64,
    f128
};

class keyword_token
    : public basic_token<keyword_token_type>
{
public:
    using base_type = basic_token<keyword_token_type>;

    keyword_token(any_token const& token)
        : base_type{token}
    {}
};

enum class punctuator_token_type
{
    double_equals_sign = static_cast<int>(token_type::double_equals_sign),
    rightwards_double_arrow,
    double_plus_sign,
    double_minus_sign,
    rightwards_arrow,
    double_vertical_line,
    quotation_mark      = '\"',
    apostrophe          = '\'',
    equals_sign         = '=',
    plus_sign           = '+',
    minus_sign          = '-',
    vertical_line       = '|',
    commercial_at       = '@',
    colon               = ':',
    semicolon           = ';',
    left_parenthesis    = '(',
    right_parenthesis   = ')',
    left_angle_bracket  = '<',
    right_angle_bracket = '>',
    left_curly_bracket  = '{',
    right_curly_bracket = '}',
    asterisk            = '*',
    slash               = '/',
    percent_sign        = '%'
};

class punctuator_token
    : public basic_token<punctuator_token_type>
{
public:
    using base_type = basic_token<punctuator_token_type>;

    punctuator_token(any_token const& token)
        : base_type{token}
    {}
};

class any_token
{
    friend class lexer;
    template<typename>
    friend class basic_token;

public:
    operator token_type() const noexcept
    {
        return m_type;
    }

    operator identifier_token() const noexcept
    {
        return {*this};
    }

    operator number_token() const noexcept
    {
        return {*this};
    }

    operator character_token() const noexcept
    {
        return {*this};
    }

    operator string_token() const noexcept
    {
        return {*this};
    }

    operator keyword_token() const noexcept
    {
        return {*this};
    }

    operator punctuator_token() const noexcept
    {
        return {*this};
    }

    [[nodiscard]]
    auto position() const noexcept -> position const&
    {
        return this->m_position;
    }

    [[nodiscard]]
    auto type() const noexcept -> token_type const&
    {
        return this->m_type;
    }

private:
    union {
        std::size_t      number;
        char             character;
        std::string_view string;
    }               m_value{};
    struct position m_position;
    token_type      m_type;
    std::byte       m_padding[[maybe_unused]][4];
};

template<typename T>
basic_token<T>::basic_token(any_token const& token) noexcept
{
    if constexpr(std::is_same_v<T, std::size_t>)
        m_value = token.m_value.number;
    else if constexpr(std::is_same_v<T, char>)
        m_value = token.m_value.character;
    else if constexpr(std::is_same_v<T, std::string_view>)
        m_value = token.m_value.string;
    m_position = token.position();
}

class lexing_error
    : public std::exception
{
public:
    lexing_error(struct position position)
        : m_position{position}
    {}
        
    [[nodiscard]]
    auto position() const noexcept -> position const&
    {
        return m_position;
    } 

    [[nodiscard]]
    auto what() const noexcept -> char const* override;

private:
    struct position m_position;   
};

class number_overflow_error
    : public lexing_error
{
public:
    using base_type = lexing_error;

    number_overflow_error(
        struct position     position,
        std::vector<char>&& buffer
    ) noexcept
        : base_type{position}
        , m_buffer{std::move(buffer)}
    {}

    [[nodiscard]]
    auto buffer() const noexcept -> std::vector<char> const&
    {
        return m_buffer;
    } 

    [[nodiscard]]
    auto what() const noexcept -> char const* override;

private:
    std::vector<char> m_buffer;
};

class unknown_character_error
    : public lexing_error
{
public:
    using base_type = lexing_error;

    unknown_character_error(
        struct position position,
        char            character
    ) noexcept
        : base_type{position}
        , m_character{character}
    {}

    [[nodiscard]]
    auto character() const noexcept -> char const&
    {
        return m_character;
    }
        
    [[nodiscard]]
    auto what() const noexcept -> char const* override;

private:
    char      m_character;
    std::byte m_padding[7];
};

class invalid_escaped_character_error
    : public lexing_error
{
public:
    using base_type = lexing_error;
        
    invalid_escaped_character_error(
        struct position position,
        char            character
    ) noexcept
        : base_type{position}
        , m_character{character}
    {}

    [[nodiscard]]
    auto character() const noexcept -> char const&
    {
        return m_character;
    }
        
    [[nodiscard]]
    auto what() const noexcept -> char const* override;

private:
    char      m_character;
    std::byte m_padding[7];
};

class incomplete_character_error
    : public lexing_error
{
public:
    using base_type = lexing_error;
        
    incomplete_character_error(
        struct position position
    ) noexcept
        : base_type{position}
    {}
        
    [[nodiscard]]
    auto what() const noexcept -> char const* override;
};

class lexer
{
public:
    static std::unordered_map<std::string_view, token_type> word_map;
        
    lexer(source_file const& file)
        : m_file{&file}
        , m_pointer{m_file->map()}
        , m_row{m_pointer}
        , m_row_number{1}
    {}

    ~lexer() = default;

    void next(any_token& out);

    void swap_file(source_file const& file) noexcept
    {
        m_file = &file;
    }

    [[nodiscard]]
    source_file const& file() const noexcept
    {
        return *m_file;
    }

    [[nodiscard]]
    position position() const noexcept
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

    void consume() noexcept
    {
        if (*m_pointer == '\n')
            m_row = m_pointer;
        ++m_pointer;
    }

    char current() const noexcept
    {
        return *m_pointer;
    }

    char peek() const noexcept
    {
        return m_pointer[1];
    }

    bool next_escaped_character();
};

}

namespace std
{

using namespace cebu;

template<>
struct formatter<token_type>
    : formatter<string>
{
    auto format(token_type const& token, format_context& ctx) const
    {
        string_view str;
        switch (token) {
        case token_type::identifier:              str = "identifier"; break;
        case token_type::number:                  str = "number"; break;
        case token_type::character:               str = "character"; break;
        case token_type::string:                  str = "string"; break;
        case token_type::fn:                      str = "fn"; break;
        case token_type::let:                     str = "let"; break;
        case token_type::if_:                     str = "if_"; break;
        case token_type::else_:                   str = "else_"; break;
        case token_type::elif:                    str = "elif"; break;
        case token_type::ret:                     str = "ret"; break;
        case token_type::b8:                      str = "b8"; break;
        case token_type::b16:                     str = "b16"; break;
        case token_type::b32:                     str = "b32"; break;
        case token_type::b64:                     str = "b64"; break;
        case token_type::b128:                    str = "b128"; break;
        case token_type::i8:                      str = "i8"; break;
        case token_type::i16:                     str = "i16"; break;
        case token_type::i32:                     str = "i32"; break;
        case token_type::i64:                     str = "i64"; break;
        case token_type::i128:                    str = "i128"; break;
        case token_type::f16:                     str = "f16"; break;
        case token_type::f32:                     str = "f32"; break;
        case token_type::f64:                     str = "f64"; break;
        case token_type::f128:                    str = "f128"; break;
        case token_type::double_equals_sign:      str = "double_equals_sign"; break;
        case token_type::rightwards_double_arrow: str = "rightwards_double_arrow"; break;
        case token_type::double_plus_sign:        str = "double_plus_sign"; break;
        case token_type::double_minus_sign:       str = "double_minus_sign"; break;
        case token_type::rightwards_arrow:        str = "rightwards_arrow"; break;
        case token_type::double_vertical_line:    str = "double_vertical_line"; break;
        case token_type::equals_sign:             str = "equals_sign"; break;
        case token_type::plus_sign:               str = "plus_sign"; break;
        case token_type::minus_sign:              str = "minus_sign"; break;
        case token_type::vertical_line:           str = "vertical_line"; break;
        case token_type::commercial_at:           str = "commercial_at"; break;
        case token_type::colon:                   str = "colon"; break;
        case token_type::semicolon:               str = "semicolon"; break;
        case token_type::left_parenthesis:        str = "left_parenthesis"; break;
        case token_type::right_parenthesis:       str = "right_parenthesis"; break;
        case token_type::left_angle_bracket:      str = "left_angle_bracket"; break;
        case token_type::right_angle_bracket:     str = "right_angle_bracket"; break;
        case token_type::left_curly_bracket:      str = "left_curly_bracket"; break;
        case token_type::right_curly_bracket:     str = "right_curly_bracket"; break;
        case token_type::asterisk:                str = "asterisk"; break;
        case token_type::slash:                   str = "slash"; break;
        case token_type::percent_sign:            str = "percent_sign"; break;
        case token_type::end:                     str = "end"; break;
        }
        return formatter<string>::format(std::format(
            "token_type::{}", str
        ), ctx);
    }
};

template<>
struct formatter<any_token>
    : formatter<string>
{
    auto format(any_token const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "token {{ type: {}, position: {} }}",
            self.type(),
            self.position()
        ), ctx);
    }
};

}
