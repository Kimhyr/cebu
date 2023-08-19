#pragma once
#define CEBU_INCLUDED_SYNTAX_H

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string_view>
#include <type_traits>

#include <cebu/utilities/vector.h>
#include <cebu/diagnostics.h>

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
    b8 = 0, // 'b8',
    b16,    // 'b16',
    b32,    // 'b32',
    b64,    // 'b64',
    b128,   // 'b128'
    i8,     // 'i8',
    i16,    // 'i16',
    i32,    // 'i32',
    i64,    // 'i64',
    i128,   // 'i128'
    f16,    // 'f16',
    f32,    // 'f32',
    f64,    // 'f64',
    f128,   // 'f128'
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
    trait,
    type,
    static_,
    let = 1100,    // 'let'
    if_,           // 'if'
    else_,         // 'else'
    elif,          // 'elif'
    return_        // 'return'
};

class token
{
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

    operator token_type() const noexcept
    {
        return type();
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
        return *this >= token_type::name && *this <= token_type::string;
    }

    auto is_delimiter() const noexcept -> bool
    {
        switch (*this) {
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
        return !is_delimiter() && static_cast<int>(type()) >= 33 && static_cast<int>(type()) <= 1000;
    }

    auto is_primitive_type() const noexcept -> bool
    {
        return *this >= token_type::b8 && *this <= token_type::f128;
    }

    auto is_determiner() const noexcept -> bool
    {
        return *this >= token_type::method && *this <= token_type::method;
    }

    template<int Size>
    friend auto operator==(
        token const& left,
        std::array<token_type, Size> const& right
    ) noexcept -> token_type
    {
        if (auto r{std::find(right.begin(), right.end(), left)}; r == left)
            return r;
        else return token_type::none;
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

template<token_type Type, typename Value>
class basic_token
{
public:
    using value_type = Value;

    basic_token(value_type value) noexcept
        : m_value{value}
    {}

    [[nodiscard]]
    auto value() const noexcept -> value_type const&
    {
        return m_value;    
    }

    [[nodiscard]]
    auto type() const noexcept -> token_type const&
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
    : basic_token<token_type::name, std::string_view>
{
public:
    using base_type = basic_token<token_type::name, std::string_view>;
    using value_type = std::string_view;
        
    explicit name(value_type&& value) noexcept
        : base_type{std::move(value)}
    {}
};

class character
    : basic_token<token_type::character, char>
{
public:
    using base_type = basic_token<token_type::character, char>;
    using value_type = base_type::value_type;
        
    explicit character(value_type value) noexcept
        : base_type{value}
    {}

private:
    std::byte m_padding[[maybe_unused]][3];
};

class number
    : basic_token<token_type::number, std::size_t>
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
    : basic_token<token_type::string, std::string_view>
{
public:
    using base_type = basic_token<token_type::string, std::string_view>;
    using value_type = base_type::value_type;
        
    explicit string(value_type&& value) noexcept
        : base_type{std::move(value)}
    {}
};

enum class primitive_type
{
    b8 = 0, // 'b8',
    b16,    // 'b16',
    b32,    // 'b32',
    b64,    // 'b64',
    b128,   // 'b128'
    i8,     // 'i8',
    i16,    // 'i16',
    i32,    // 'i32',
    i64,    // 'i64',
    i128,   // 'i128'
    f16,    // 'f16',
    f32,    // 'f32',
    f64,    // 'f64',
    f128    // 'f128'
};

class parser;

struct basic_option {};

template<typename T>
concept option = std::derived_from<T, basic_option>;

struct identifier
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
    name name{nullptr};
#pragma GCC diagnostic pop

    template<option...>
    static void parse(parser&, identifier&);
};

struct type
{
    union {
        primitive_type primitive;
    } value;
    enum : int {
        primitive
    } variant;

    using type_t = unsigned char;

    template<option...>
    static void parse(parser&, type&);

    constexpr type(primitive_type primitive) noexcept
        : value{.primitive = primitive}
        , variant{type::primitive}
    {}
};

constexpr type b8_type{primitive_type::b8};
constexpr type b16_type{primitive_type::b16};
constexpr type b32_type{primitive_type::b32};
constexpr type b64_type{primitive_type::b64};
constexpr type i8_type{primitive_type::i8};
constexpr type i16_type{primitive_type::i16};
constexpr type i32_type{primitive_type::i32};
constexpr type i64_type{primitive_type::i64};
constexpr type f16_type{primitive_type::f16};
constexpr type f32_type{primitive_type::f32};
constexpr type f64_type{primitive_type::f64};

template<typename Syntax>
struct path
{
    template<option...>
    static void parse(parser&, path&);
};

struct body
{
    template<option...>
    static void parse(parser&, body&);
};

struct value_definition
{
    identifier identifier;
    path<type> type;
    std::optional<body> body;

    template<option...>
    static void parse(parser&, value_definition&);
};

struct parameters
{
    std::vector<value_definition> definitions;

    template<option...>
    static void parse(parser&, parameters&);
};

struct method_definition
{
    identifier identifier;
    std::optional<parameters> parameters;
    std::optional<path<type>> type;
    std::optional<body> body;

    template<option...>
    static void parse(parser&, method_definition&);
};

}
