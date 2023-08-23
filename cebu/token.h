#pragma once
#define CEBU_INCLUDED_TOKEN_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
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
    decimal,
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

class token
{
    friend class lexer;
        
public:
    bool is_valuable() const noexcept
    { return type >= token_type::name && type <= token_type::string; }

    bool is_delimiter() const noexcept
    {
        switch (type) {
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

    bool is_punctuator() const noexcept
    {
        return !is_delimiter()
            && static_cast<int>(type) >= 33
            && static_cast<int>(type) < 1000;
    }

    bool is_primitive_type() const noexcept
    {
        return type >= token_type::b8
            && type < static_cast<token_type>(33);
    }

    bool is_determiner() const noexcept
    {
        return type >= token_type::method
            && type < token_type::let;
    }

    bool is_nondeterminer() const noexcept
    {
        return type >= token_type::let
            && type < token_type::_;
    }

    friend constexpr bool operator==(token const& left, token const& right)
    { return left.type == right.type; }

    template<typename T> friend constexpr
    bool operator==(token const& left, T const& right)
    { return *std::find(right.begin(), right.end(), left) == left; }  

    friend constexpr
    bool operator==(token const& left, token_type const& right)
    { return left.type == right; }

    friend constexpr
    bool operator==(token const& left, token_category const& right)
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

    void discard() const noexcept
    {
        if (*this == token_type::string) [[unlikely]]
            delete value.string.data();
    }

    union {
        std::string_view string;
        std::size_t      number;
        double           decimal;
        char             character;
    }               value{};
    enum token_type type{token_type::none};
    std::byte       padding[[maybe_unused]][4];

    operator std::string_view const&() const { return value.string; }
    operator std::uint64_t const&() const { return value.number; }
    operator double const&() const { return value.decimal; }
    operator char const&() const { return value.character; }
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
        string format{"token_type: "};
        switch (self) {
        case cebu::token_type::none:
            format += "none";
            break;
        case cebu::token_type::name:
            format += "name";
            break;
        case cebu::token_type::number:
            format += "number";
            break;
        case cebu::token_type::character:
            format += "character";
            break;
        case cebu::token_type::string:
            format += "string";
            break;
        case cebu::token_type::end:
            format += "end";
            break;
        case cebu::token_type::b8:
            format += "b8";
            break;
        case cebu::token_type::b16:
            format += "b16";
            break;
        case cebu::token_type::b32:
            format += "b32";
            break;
        case cebu::token_type::b64:
            format += "b64";
            break;
        case cebu::token_type::i8:
            format += "i8";
            break;
        case cebu::token_type::i16:
            format += "i16";
            break;
        case cebu::token_type::i32:
            format += "i32";
            break;
        case cebu::token_type::i64:
            format += "i64";
            break;
        case cebu::token_type::f16:
            format += "f16";
            break;
        case cebu::token_type::f32:
            format += "f32";
            break;
        case cebu::token_type::f64:
            format += "f64";
            break;
        case cebu::token_type::equals_sign:
            format += "equals_sign";
            break;
        case cebu::token_type::plus_sign:
            format += "plus_sign";
            break;
        case cebu::token_type::minus_sign:
            format += "minus_sign";
            break;
        case cebu::token_type::vertical_line:
            format += "vertical_line";
            break;
        case cebu::token_type::commercial_at:
            format += "commercial_at";
            break;
        case cebu::token_type::colon:
            format += "colon";
            break;
        case cebu::token_type::semicolon:
            format += "semicolon";
            break;
        case cebu::token_type::comma:
            format += "comma";
            break;
        case cebu::token_type::asterisk:
            format += "asterisk";
            break;
        case cebu::token_type::slash:
            format += "slash";
            break;
        case cebu::token_type::percent_sign:
            format += "percent_sign";
            break;
        case cebu::token_type::left_parenthesis:
            format += "left_parenthesis";
            break;
        case cebu::token_type::right_parenthesis:
            format += "right_parenthesis";
            break;
        case cebu::token_type::left_angle_bracket:
            format += "left_angle_bracket";
            break;
        case cebu::token_type::right_angle_bracket:
            format += "right_angle_bracket";
            break;
        case cebu::token_type::left_square_bracket:
            format += "left_square_bracket";
            break;
        case cebu::token_type::right_square_bracket:
            format += "right_square_bracket";
            break;
        case cebu::token_type::left_curly_bracket:
            format += "left_curly_bracket";
            break;
        case cebu::token_type::right_curly_bracket:
            format += "right_curly_bracket";
            break;
        case cebu::token_type::double_equals_sign:
            format += "double_equals_sign";
            break;
        case cebu::token_type::rightwards_double_arrow:
            format += "rightwards_double_arrow";
            break;
        case cebu::token_type::double_plus_sign:
            format += "double_plus_sign";
            break;
        case cebu::token_type::double_minus_sign:
            format += "double_minus_sign";
            break;
        case cebu::token_type::rightwards_arrow:
            format += "rightwards_arrow";
            break;
        case cebu::token_type::double_vertical_line:
            format += "double_vertical_line";
            break;
        case cebu::token_type::method:
            format += "method";
            break;
        case cebu::token_type::trait:
            format += "trait";
            break;
        case cebu::token_type::type:
            format += "type";
            break;
        case cebu::token_type::static_:
            format += "static";
            break;
        case cebu::token_type::let:
            format += "let";
            break;
        case cebu::token_type::if_:
            format += "if";
            break;
        case cebu::token_type::else_:
            format += "else";
            break;
        case cebu::token_type::elif:
            format += "elif";
            break;
        case cebu::token_type::return_:
            format += "return";
            break;
        }
        return formatter<string>::format(std::format(
            "{}",
            format
        ), ctx);
    }
};

template<>
struct formatter<cebu::token>
    : formatter<string>
{
    auto format(cebu::token const& self, format_context& ctx) const
    {
        string format{std::format("token:\n\t{}", self.type)};
        if (self.is_valuable()) {
            format += "\n\tvalue: ";
            switch (self.type) {
            case cebu::token_type::string:
                format += std::format("\"{}\"", self.value.string);
                break;
            case cebu::token_type::name:
                format += std::format("{}", self.value.string);
                break;
            case cebu::token_type::character:
                format += std::format("'{}'", self.value.character);
                break;
            case cebu::token_type::number:
                format += std::format("{}", self.value.number);
                break;
            }
        }
        return formatter<string>::format(format, ctx);
    }
};

}
