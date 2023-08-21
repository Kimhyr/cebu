#pragma once
#include <format>
#define CEBU_INCLUDED_TOKEN_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string_view>

namespace cebu
{

enum class token_category
{
    none,
    valuable,
    punctuator,
    delimiter,
    primitive_type,
    determiner,
    nondeterminer
};

enum class token_type
{
    none = -1000,
    name,
    number,
    character,
    string,
    end = '\0',
    b8 = 1, // 'b8',
    b16,    // 'b16',
    b32,    // 'b32',
    b64,    // 'b64',
    i8,     // 'i8',
    i16,    // 'i16',
    i32,    // 'i32',
    i64,    // 'i64',
    f16,    // 'f16',
    f32,    // 'f32',
    f64,    // 'f64',
    equals_sign             = '=',
    plus_sign               = '+',
    minus_sign              = '-',
    vertical_line           = '|',
    commercial_at           = '@',
    colon                   = ':',
    semicolon               = ';',
    comma                   = ',',
    asterisk                = '*',
    slash                   = '/',
    percent_sign            = '%',
    left_parenthesis        = '(',
    right_parenthesis       = ')',
    left_angle_bracket      = '<',
    right_angle_bracket     = '>',
    left_square_bracket     = '[', // NOTE: New tokens: [left|right]_square_bracket
    right_square_bracket    = ']',
    left_curly_bracket      = '{',
    right_curly_bracket     = '}',
    double_equals_sign      = '=' + '=' + '\x7f',
    rightwards_double_arrow = '=' + '>' + '\x7f',
    double_plus_sign        = '+' + '+' + '\x7f',
    double_minus_sign       = '-' + '-' + '\x7f',
    rightwards_arrow        = '-' + '>' + '\x7f',
    double_vertical_line    = '|' + '|' + '\x7f',
    method = 1000, // 'method'
    trait,         // 'trait'
    type,          // 'type'
    static_,       // 'static'
    let = 1100,    // 'let'
    if_,           // 'if'
    else_,         // 'else'
    elif,          // 'elif'
    return_,       // 'return'
    _ = 1200
};

template<token_type Type, typename Value>
class basic_token
{
public:
    using value_type = Value;

    basic_token(value_type value) noexcept
        : m_value{value}
    {}

    [[nodiscard]]
    value_type const& value() const noexcept
    {
        return m_value;    
    }

    [[nodiscard]]
    token_type const& type() const noexcept
    {
        return m_type;
    }

private:
    value_type m_value;
    std::byte  m_padding[
        std::conditional_t<
            Type == token_type::character,
            std::integral_constant<int, 3>,
            std::integral_constant<int, 4>
    >::value];
    token_type m_type{Type};
};

class name
    : public basic_token<token_type::name, std::string_view>
{
public:
    using base_type = basic_token<token_type::name, std::string_view>;
    using value_type = std::string_view;

    name()
        : base_type{""} {}
    
    explicit name(value_type value) noexcept
        : base_type{value}
    {}
};

class character
    : public basic_token<token_type::character, char>
{
public:
    using base_type = basic_token<token_type::character, char>;
    using value_type = base_type::value_type;
        
    explicit character(value_type value) noexcept
        : base_type{value}
    {}
};

class number
    : public basic_token<token_type::number, std::size_t>
{
public:
    using base_type = basic_token<token_type::number, std::size_t>;
    using value_type = base_type::value_type;
        
    explicit number(value_type value) noexcept
        : base_type{value}
    {}

    [[nodiscard]]
    auto is_16bit() const noexcept -> bool
    {
        return this->value() > std::numeric_limits<std::uint8_t>::max();
    }

    [[nodiscard]]
    auto is_32bit() const noexcept -> bool
    {
        return this->value() > std::numeric_limits<std::uint16_t>::max();
    }

    [[nodiscard]]
    auto is_64bit() const noexcept -> bool
    {
        return this->value() > std::numeric_limits<std::uint32_t>::max();
    }
};

class string
    : public basic_token<token_type::string, std::string_view>
{
public:
    using base_type = basic_token<token_type::string, std::string_view>;
    using value_type = base_type::value_type;
        
    explicit string(value_type value) noexcept
        : base_type{value}
    {}
};

enum class primitive_type
{
    b8 = 0, // 'b8',
    b16,    // 'b16',
    b32,    // 'b32',
    b64,    // 'b64',
    i8,     // 'i8',
    i16,    // 'i16',
    i32,    // 'i32',
    i64,    // 'i64',
    f16,    // 'f16',
    f32,    // 'f32',
    f64     // 'f64',
};

class token
{
    friend class lexer;
        
public:
    token() = default;

    token(token_type type, std::string_view string) noexcept
        : m_value{.string = string}
        , m_type{type}
    {}

    token(char character) noexcept
        : m_value{.character = character}
        , m_type{token_type::character}
    {}

    token(std::size_t number) noexcept
        : m_value{.number = number}
        , m_type{token_type::number}
    {}

    operator name() const noexcept
    {
        return name{m_value.string};
    }

    operator character() const noexcept
    {
        return character{m_value.character};
    }

    operator number() const noexcept
    {
        return number{m_value.number};
    }

    operator string() const noexcept
    {
        return string{m_value.string};
    }

    template<token_category Category>
    auto is_of() const noexcept -> bool
    {
        if constexpr(Category == token_category::valuable)
            return is_valuable();
        else if constexpr(Category == token_category::punctuator)
            return is_punctuator();
        else if constexpr(Category == token_category::delimiter)
            return is_delimiter();
        else if constexpr(Category == token_category::primitive_type)
            return is_primitive_type();
        else if constexpr(Category == token_category::determiner)
            return is_determiner();
    }

    auto is_valuable() const noexcept -> bool
    {
        return type() >= token_type::name && type() <= token_type::string;
    }

    auto is_delimiter() const noexcept -> bool
    {
        switch (type()) {
        case token_type::left_parenthesis:
        case token_type::left_curly_bracket:
        case token_type::left_square_bracket:
        case token_type::left_angle_bracket:
        case token_type::right_parenthesis:
        case token_type::right_curly_bracket:
        case token_type::right_square_bracket:
        case token_type::right_angle_bracket:
            return true;
        default:
            return false;
        }
    }

    auto is_punctuator() const noexcept -> bool
    {
        return !is_delimiter()
            && static_cast<int>(type()) >= 33
            && static_cast<int>(type()) < 1000;
    }

    bool is_primitive_type() const noexcept
    {
        return type() >= token_type::b8
            && type() < static_cast<token_type>(33);
    }

    bool is_determiner() const noexcept
    {
        return type() >= token_type::method
            && type() < token_type::let;
    }

    bool is_nondeterminer() const noexcept
    {
        return type() >= token_type::let
            && type() < token_type::_;
    }

    friend constexpr
    bool operator==(token const& left, token const& right)
    {
        return left.type() == right.type();
    }

    template<int Size>
    friend constexpr
    bool operator==(token const& left,
                    std::array<token_type, Size> const& right)
    {
        return std::find(right.begin(), right.end(), left) == left;
    }  

    friend constexpr
    bool operator==(token const& left,
                    token_type const& right)
    {
        return left.type() == right;
    }

    friend constexpr
    bool operator==(token const& left,
                    token_category const& right)
    {
        switch (right) {
        case token_category::valuable:
            return left.is_valuable();
        case token_category::punctuator:
            return left.is_punctuator();
        case token_category::delimiter:
            return left.is_delimiter();
        case token_category::primitive_type:
            return left.is_primitive_type();
        case token_category::determiner:
            return left.is_determiner();
        case token_category::nondeterminer:
            return left.is_nondeterminer();
        default:
            return false;
        }
    }

    [[nodiscard]]
    auto type() const noexcept -> token_type const&
    {
        return m_type;
    }

    auto discard() const noexcept -> void
    {
        if (*this == token_type::string) [[unlikely]]
            delete m_value.string.data();
    }

private:
    union {
        std::string_view string;
        std::size_t      number;
        char             character;
    }          m_value{};
    enum token_type m_type{token_type::none};
    std::byte  m_padding[[maybe_unused]][4];
};

}

namespace std
{

template<>
struct formatter<cebu::token_type>
    : formatter<string>
{
    auto format(cebu::token_type const& self, format_context& ctx) const
    {
        string str;
        switch (self) {
        case cebu::token_type::none:                    str = "none"; break;
        case cebu::token_type::name:                    str = "name"; break;
        case cebu::token_type::number:                  str = "number"; break;
        case cebu::token_type::character:               str = "character"; break;
        case cebu::token_type::string:                  str = "string"; break;
        case cebu::token_type::end:                     str = "end"; break;
        case cebu::token_type::b8:                      str = "b8"; break;
        case cebu::token_type::b16:                     str = "b16"; break;
        case cebu::token_type::b32:                     str = "b32"; break;
        case cebu::token_type::b64:                     str = "b64"; break;
        case cebu::token_type::i8:                      str = "i8"; break;
        case cebu::token_type::i16:                     str = "i16"; break;
        case cebu::token_type::i32:                     str = "i32"; break;
        case cebu::token_type::i64:                     str = "i64"; break;
        case cebu::token_type::f16:                     str = "f16"; break;
        case cebu::token_type::f32:                     str = "f32"; break;
        case cebu::token_type::f64:                     str = "f64"; break;
        case cebu::token_type::equals_sign:             str = "equals_sign"; break;
        case cebu::token_type::plus_sign:               str = "plus_sign"; break;
        case cebu::token_type::minus_sign:              str = "minus_sign"; break;
        case cebu::token_type::vertical_line:           str = "vertical_line"; break;
        case cebu::token_type::commercial_at:           str = "commercial_at"; break;
        case cebu::token_type::colon:                   str = "colon"; break;
        case cebu::token_type::semicolon:               str = "semicolon"; break;
        case cebu::token_type::comma:                   str = "comma"; break;
        case cebu::token_type::asterisk:                str = "asterisk"; break;
        case cebu::token_type::slash:                   str = "slash"; break;
        case cebu::token_type::percent_sign:            str = "percent_sign"; break;
        case cebu::token_type::left_parenthesis:        str = "left_parenthesis"; break;
        case cebu::token_type::right_parenthesis:       str = "right_parenthesis"; break;
        case cebu::token_type::left_angle_bracket:      str = "left_angle_bracket"; break;
        case cebu::token_type::right_angle_bracket:     str = "right_angle_bracket"; break;
        case cebu::token_type::left_square_bracket:     str = "left_square_bracket"; break;
        case cebu::token_type::right_square_bracket:    str = "right_square_bracket"; break;
        case cebu::token_type::left_curly_bracket:      str = "left_curly_bracket"; break;
        case cebu::token_type::right_curly_bracket:     str = "right_curly_bracket"; break;
        case cebu::token_type::double_equals_sign:      str = "double_equals_sign"; break;
        case cebu::token_type::rightwards_double_arrow: str = "rightwards_double_arrow"; break;
        case cebu::token_type::double_plus_sign:        str = "double_plus_sign"; break;
        case cebu::token_type::double_minus_sign:       str = "double_minus_sign"; break;
        case cebu::token_type::rightwards_arrow:        str = "rightwards_arrow"; break;
        case cebu::token_type::double_vertical_line:    str = "double_vertical_line"; break;
        case cebu::token_type::method:                  str = "method"; break;
        case cebu::token_type::trait:                   str = "trait"; break;
        case cebu::token_type::type:                    str = "type"; break;
        case cebu::token_type::static_:                 str = "static"; break;
        case cebu::token_type::let:                     str = "let"; break;
        case cebu::token_type::if_:                     str = "if"; break;
        case cebu::token_type::else_:                   str = "else"; break;
        case cebu::token_type::elif:                    str = "elif"; break;
        case cebu::token_type::return_:                 str = "return"; break;
        }
        return formatter<string>::format(std::format(
            "{}",
            str
        ), ctx);
    }
};

template<>
struct formatter<cebu::name>
    : formatter<string>
{
    auto format(cebu::name const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "{}",
            self.value()
        ), ctx);
    }
};

template<>
struct formatter<cebu::character>
    : formatter<string>
{
    auto format(cebu::character const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "'{}'",
            self.value()
        ), ctx);
    }
};

template<>
struct formatter<cebu::string>
    : formatter<string>
{
    auto format(cebu::string const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "\"{}\"",
            self.value()
        ), ctx);
    }
};

template<>
struct formatter<cebu::number>
    : formatter<string>
{
    auto format(cebu::string const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "{}",
            self.value()
        ), ctx);
    }
};

template<>
struct formatter<cebu::token>
    : formatter<string>
{
    auto format(cebu::string const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "{}: {}",
            self.type(), self.value()
        ), ctx);
    }
};

}
