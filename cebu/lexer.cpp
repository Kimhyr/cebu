#include <cctype>
#include <stdexcept>
#include <vector>

#include "lexer.h"

namespace cebu
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
std::unordered_map<std::string_view, token_type> lexer::word_map{
    { "fn"  , token_type::fn    },
    { "let" , token_type::let   },
    { "if"  , token_type::if_   },
    { "else", token_type::else_ },
    { "elif", token_type::elif  },
    { "ret" , token_type::ret   },
    { "b8"  , token_type::b8    },
    { "b16" , token_type::b16   },
    { "b32" , token_type::b32   },
    { "b64" , token_type::b64   },
    { "b128", token_type::b128  },
    { "i8"  , token_type::i8    },
    { "i16" , token_type::i16   },
    { "i32" , token_type::i32   },
    { "i64" , token_type::i64   },
    { "i128", token_type::i128  },
    { "f16" , token_type::f16   },
    { "f32" , token_type::f32   },
    { "f64" , token_type::f64   },
    { "f128", token_type::f128  },
};
#pragma GCC diagnostic pop

auto lexing_error::what() const noexcept -> char const*
{
    return "lexing_error";
}

auto number_overflow_error::what() const noexcept -> char const*
{
    return "number_overflow_error";
}

auto unknown_character_error::what() const noexcept -> char const*
{
    return "unknown_character_error";
}

auto invalid_escaped_character_error::what() const noexcept -> char const*
{
    return "invalid_escaped_character_error";
}

auto incomplete_character_error::what() const noexcept -> char const*
{
    return "incomplete_character_error";
}

void lexer::next(any_token& token)
{
    while (std::isspace(current()))
        consume();
    token.m_position = position();
    token_type type{static_cast<token_type>(*m_pointer)};
    std::vector<char> buffer;
    switch (type) {
    case token_type::quotation_mark:
        for (;;) {
            consume();
            if (!next_escaped_character() && current() == '"')
                break;
            buffer.push_back(current());
        }
        token.m_type         = token_type::string;
        token.m_value.string = std::string_view{new char[buffer.size()], buffer.size()};
        std::copy(buffer.begin(), buffer.end(), const_cast<char*>(token.m_value.string.begin()));
        consume();
        return;
    case token_type::apostrophe:
        consume();
        if (!next_escaped_character() && current() == '\'')
            throw incomplete_character_error{position()};
        token.m_value.character = current();
        consume();
        if (current() != '\'') [[unlikely]]
            throw incomplete_character_error{position()};
        consume();
        token.m_type = token_type::character;
        return;
    case token_type::equals_sign:
        switch (peek()) {
        case '=':
        case '>': goto double_character;
        }
    case token_type::plus_sign:
        switch (peek()) {
        case '+': goto double_character;
        }
    case token_type::minus_sign:
        switch (peek()) {
        case '-':
        case '>': goto double_character;
        }
    case token_type::vertical_line:
        switch (peek()) {
        case '|': goto double_character;
        }
    case token_type::end:
    case token_type::commercial_at:
    case token_type::colon:
    case token_type::semicolon:
    case token_type::left_parenthesis:
    case token_type::right_parenthesis:
    case token_type::left_angle_bracket:
    case token_type::right_angle_bracket:
    case token_type::left_curly_bracket:
    case token_type::right_curly_bracket:
    case token_type::asterisk:
    case token_type::slash:
    case token_type::percent_sign:
        consume();
        token.m_type = type;
        return;
    default:
        if (std::isalpha(current()) || current() == '_') {
            do {
                buffer.push_back(current());
                consume();
            } while (std::isalpha(current())
                    || current() == '_'
                    || std::isdigit(current()));
            auto view{std::string_view{buffer.data(), buffer.size()}};
            try {
                token.m_type = word_map.at(view);
            } catch (std::out_of_range const&) {
                token.m_type = token_type::identifier;
                token.m_value.string = std::string_view{new char[buffer.size()], buffer.size()};
                std::copy(buffer.begin(), buffer.end(), const_cast<char*>(token.m_value.string.begin()));
            }
            return;
        } else if (std::isdigit(current())) {
            do {
                buffer.push_back(current());
                consume();
            } while (std::isdigit(current()));
            buffer.push_back(0);
            try {
                token.m_value.number = std::stoul(buffer.data(), nullptr, 10);
            } catch (std::out_of_range const&) {
                throw number_overflow_error{position(), std::move(buffer)};
            } catch (std::invalid_argument const&) {
                throw std::runtime_error{"unreachable"};
            }
            token.m_type = token_type::number;
            return;
        }
        throw unknown_character_error{position(), *m_pointer};
    }

double_character:
    token.m_type = static_cast<token_type>(current() + peek() + '\x7f');
    consume();
    consume();
}

bool lexer::next_escaped_character()
{
    if (current() != '\\') {
        return false;
    }
    consume();
    switch (current()) {
    case '\\':
    case '\'':
    case '"': return true;
    default:
        throw invalid_escaped_character_error{position(), current()};
    }
}

}

auto operator<<(std::ostream& os, cebu::token_type const& token) -> std::ostream&
{
    return os << std::format("{}", token);
}

auto operator<<(std::ostream& os, cebu::any_token const& token) -> std::ostream&
{
    return os << std::format("{}", token);
}
