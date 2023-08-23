#include <cctype>
#include <string>

#include "lexer.h"

namespace cebu
{

result lexer::lex(token& out) noexcept
{
    if (flags().reverted) [[unlikely]] {
        m_marker = m_prior_marker;
        m_flags.reverted = false;
    }
    while (std::isspace(current()))
        consume();
    struct position start_position{position()};
    m_prior_marker = m_marker;
    switch (current()) {
    case '"': {
        std::vector<char> buffer;
        for (;;) {
            consume();
            character_result result{lex_escaped_character()};
            if (result == character_result::failure) [[unlikely]] {
                report<lexing_error::unknown_escaped_character>(start_position);
                return result::failure;
            }

            if (result == character_result::regular && current() == '"') [[unlikely]]
                break;
            buffer.push_back(current());
        }
        consume();

        out.type = token_type::string;
        out.value.string = std::string_view{
            new char[buffer.size()],
            buffer.size()
        };
        std::copy(
            buffer.begin(),
            buffer.end(),
            const_cast<char*>(out.value.string.begin())
        );
    } break;
    case '\'': {
        consume();
        character_result result{lex_escaped_character()};
        if (result == character_result::failure) [[unlikely]] {
            report<lexing_error::unknown_escaped_character>(start_position);
            return result::failure;
        }

        if (result == character_result::regular && current() == '\'') [[unlikely]]
            report<lexing_error::incomplete_character>(start_position);
        out.value.character = current();
        consume();

        if (current() != '\'') [[unlikely]]
            report<lexing_error::incomplete_character>(start_position);
        consume();
        out.type = token_type::character;
    } break;
    case '=':
        switch (peek()) {
        case '=':
        case '>': goto double_character;
        }
    case '+':
        switch (peek()) {
        case '+': goto double_character;
        }
    case '-':
        switch (peek()) {
        case '-':
        case '>': goto double_character;
        }
    case '|':
        switch (peek()) {
        case '|': goto double_character;
        }
    case '\0':
    case '@':
    case ',':
    case ':':
    case ';':
    case '(':
    case ')':
    case '[':
    case ']':
    case '<':
    case '>':
    case '*':
    case '/':
    case '%':
        out.type = static_cast<token_type>(current());
        goto single_character;
    double_character:
        out.type = static_cast<token_type>(current() + peek() + '\x7f');
        consume();
    single_character:
        consume();
        break;
    default:
        if (std::isalpha(current()) || current() == '_') {
            std::vector<char> buffer;
            do {
                buffer.push_back(current());
                consume();
            } while (std::isalpha(current())
                || current() == '_'
                || std::isdigit(current()));
            auto view{std::string_view{buffer.data(), buffer.size()}};

            try {
                out.type = get_keywords().at(view);
            } catch (std::out_of_range const&) {
                out.type = token_type::name;
                out.value.string = std::string_view{
                    new char[buffer.size()],
                    buffer.size()
                };
                std::copy(
                    buffer.begin(),
                    buffer.end(),
                    const_cast<char*>(out.value.string.begin())
                );
            }
        } else if (std::isdigit(current())) {
            out.type = token_type::number;
            std::vector<char> buffer;
            do {
                buffer.push_back(current());
                consume();
                if (current() == '.') [[unlikely]] {
                    if (out == token_type::decimal) [[unlikely]] {
                        report<lexing_error::multiple_decimal_points>(start_position);
                        return result::failure;
                    }
                    out.type = token_type::decimal;
                }
            } while (std::isdigit(current()));
            buffer.push_back('\0');

            try {
                if (out == token_type::decimal) [[unlikely]]
                    out.value.decimal = std::stod(buffer.data());
                else out.value.number = std::stoul(buffer.data());
            } catch (std::out_of_range const&) {
                report<lexing_error::number_overflow>(start_position, buffer);
                return result::failure;
            } catch (std::invalid_argument const&) {
                throw std::runtime_error{"unreachable"};
            }
        } else {
            report<lexing_error::unknown_character>(start_position);
            return result::failure;
        }
        break;
    }
    return result::success;
}

auto lexer::lex_escaped_character() noexcept -> lexer::character_result
{
    if (current() != '\\')
        return character_result::regular;
    consume();
    switch (current()) {
    case '\\':
    case '"':
    case '\'':
        return character_result::escaped;
    default:
        return character_result::failure;
    }
}

template<lexing_error Error, typename ...Args>
void lexer::report(struct position const& position, Args&&... args)
{
    struct location location{file_path(), position};
    std::string format{std::format("[{}] lexing error: ", location)};
    if constexpr(Error == lexing_error::incomplete_character)
        format += std::format("incomplete character token");
    else if constexpr(Error == lexing_error::unknown_character)
        format += std::format("unknown character: '{}'", current());
    else if constexpr(Error == lexing_error::number_overflow)
        format += [&](std::vector<char> const& buffer) -> std::string {
            return std::format("number overflow: {}", buffer.data());
        }(std::forward<Args>(args)...);
    else if constexpr(Error == lexing_error::unknown_escaped_character)
        format += std::format("unknown escaped character: '{}'", current());
    else if constexpr(Error == lexing_error::multiple_decimal_points)
        format += "more than one decimal point in decimal token";
    std::cerr << format << std::endl;
}

}
