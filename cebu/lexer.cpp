#include <cctype>
#include <string>

#include "lexer.h"

namespace cebu
{

result lexer::lex(token& token) noexcept
{
    // Skip any whitespace.
    while (std::isspace(current()))
        consume();

    // Save the current position for diagnostics and save the state of the
    // cursor.
    struct position start_position{position()};
    m_prior_cursor = m_cursor;

    // Match the current character with the start symbol of each token.
    switch (current()) {
    case '"': {
        // Lex a string token.

        std::vector<char> buffer;
        for (;;) {
            consume();
            character_result result{lex_escaped_character()};
            if (result == character_result::failure) [[unlikely]] {
                report<error::unknown_escaped_character>(start_position);
                return result::failure;
            }

            // Check if the string should be terminated.
            if (result == character_result::regular && current() == '"') [[unlikely]]
                break;
            buffer.push_back(current());
        }
        consume();  // Consume the terminator.

        token.type = token_type::string;

        // Allocate a tight space for the string then copy the string into the
        // space.
        token.value.string = std::string_view{
            new char[buffer.size()],
            buffer.size()
        };
        std::copy(buffer.begin(), buffer.end(),
                  const_cast<char*>(token.value.string.begin()));
    } break;
    case '\'': {
        // Lex a character token.

        consume();
        character_result result{lex_escaped_character()};
        if (result == character_result::failure) [[unlikely]] {
            report<error::unknown_escaped_character>(start_position);
            return result::failure;
        }

        if (result == character_result::regular && current() == '\'') [[unlikely]]
            token.value.character = '\0';
        else {
            token.value.character = current();
            consume();  // Consume the value.

            if (current() != '\'') [[unlikely]]
                report<error::incomplete_character>(start_position);
        }
        consume();  // Consume the terminator.
        token.type = token_type::character;
    } break;

    // The following are symbolic tokens.
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
        // For single-characters, we can simply cast the current character as
        // a token type since symbolic token types have the value of the
        // symbol that they represent.
        token.type = static_cast<token_type>(current());
        goto single_character;
    double_character:
        // Similarly to single-characters, but we also add the peeked character
        // and the special character `\x7f` so that the value of the token type
        // doesn't conflict with single-character tokens.
        token.type = static_cast<token_type>(current() + peek() + '\x7f');
        consume();
    single_character:
        consume();
        break;
    default:
        // Instead of adding more cases for digits and letters, implications
        // are used.

        // Check for the start symbol of an identifier or keyword token.
        if (std::isalpha(current()) || current() == '_') {
            std::vector<char> buffer;
            do {
                buffer.push_back(current());
                consume();
            } while (std::isalpha(current())
                || current() == '_'
                || std::isdigit(current()));
            auto view{std::string_view{buffer.data(), buffer.size()}};

            // First try to create a keyword token.  If this fails, create an
            // identifier token.
            try {
                token.type = get_keywords().at(view);
            } catch (std::out_of_range const&) {
                token.type = token_type::name;

                // Allocate a tight space for the identifier and copy the
                // identifier into the space.
                token.value.string = std::string_view{
                    new char[buffer.size()],
                    buffer.size()
                };
                std::copy(buffer.begin(), buffer.end(),
                          const_cast<char*>(token.value.string.begin()));
            }
        }

        // Check for the start symbol of a number token.
        else if (std::isdigit(current())) {
            token.type = token_type::number;
            std::vector<char> buffer;
            do {
                buffer.push_back(current());
                consume();
                if (current() == '.') [[unlikely]] {
                    if (token == token_type::decimal) [[unlikely]]
                        break;
                    token.type = token_type::decimal;
                }
            } while (std::isdigit(current()));
            buffer.push_back('\0');

            // Try to parse the value as the token type.
            try {
                if (token == token_type::decimal) [[unlikely]]
                    token.value.decimal = std::stod(buffer.data());
                else token.value.number = std::stoul(buffer.data());
            } catch (std::out_of_range const&) {
                report<error::number_overflow>(start_position, buffer);
                return result::failure;
            } catch (std::invalid_argument const&) {
                throw std::runtime_error{"unreachable"};
            }
        }

        // The character is uknown.
        else {
            report<error::unknown_character>(start_position);
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

template<lexer::error Error, typename ...Args>
void lexer::report(struct position const& position, Args&&... args)
{
    struct location location{file_path(), position};
    std::string format{std::format("[{}] lexing error: ", location)};
    if constexpr(Error == error::incomplete_character)
        format += std::format("incomplete character token");
    else if constexpr(Error == error::unknown_character)
        format += std::format("unknown character: '{}'", current());
    else if constexpr(Error == error::number_overflow)
        format += [&](std::vector<char> const& buffer) -> std::string {
            return std::format("number overflow: {}", buffer.data());
        }(std::forward<Args>(args)...);
    else if constexpr(Error == error::unknown_escaped_character)
        format += std::format("unknown escaped character: '{}'", current());
    else if constexpr(Error == error::multiple_decimal_points)
        format += "more than one decimal point in decimal token";
    std::cerr << format << std::endl;
}

}
