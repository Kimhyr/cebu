#include <cctype>

#include "lexer.h"

namespace cebu
{

result lexer::lex(token& out) noexcept
{
    while (std::isspace(current()))
        consume();

    struct position start_position{position()};

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

        out.m_type = token_type::string;
        out.m_value.string = std::string_view{new char[buffer.size()], buffer.size()};
        std::copy(buffer.begin(), buffer.end(), const_cast<char*>(out.m_value.string.begin()));
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
        out.m_value.character = current();
        consume();

        if (current() != '\'') [[unlikely]]
            report<lexing_error::incomplete_character>(start_position);
        consume();
        out.m_type = token_type::character;
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
        out.m_type = static_cast<token_type>(current());
        goto single_character;
    double_character:
        out.m_type = static_cast<token_type>(current() + peek() + '\x7f');
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
                out.m_type = get_keywords().at(view);
            } catch (std::out_of_range const&) {
                out.m_type = token_type::name;
                out.m_value.string = std::string_view{new char[buffer.size()], buffer.size()};
                std::copy(buffer.begin(), buffer.end(), const_cast<char*>(out.m_value.string.begin()));
            }
        } else if (std::isdigit(current())) {
            std::vector<char> buffer;
            do {
                buffer.push_back(current());
                consume();
            } while (std::isdigit(current()));
            buffer.push_back('\0');

            try {
                out.m_value.number = std::stoul(buffer.data());
            } catch (std::out_of_range const&) {
                report<lexing_error::number_overflow>(start_position, buffer);
                return result::failure;
            } catch (std::invalid_argument const&) {
                throw std::runtime_error{"unreachable"};
            }
            out.m_type = token_type::number;
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
        format += std::format("unknown character: {}", current());
    else if constexpr(Error == lexing_error::number_overflow)
        format += [&](std::vector<char> const& buffer) -> std::string {
            return std::format("number overflow: {}", buffer.data());
        }(std::forward<Args>(args)...);
    else if constexpr(Error == lexing_error::unknown_escaped_character)
        format += std::format("unknown escaped character: {}", current());
    std::cerr << format << std::endl;
}

}
