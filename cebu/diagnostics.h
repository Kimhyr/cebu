#pragma once
#define CEBU_INCLUDED_DIAGNOSTICS_H

#include <format>
#include <functional>
#include <iostream>
#include <string_view>

namespace cebu
{

struct position
{
    std::size_t row;
    std::size_t column;
};

}

template<>
struct std::formatter<cebu::position>
    : formatter<string>
{
    auto format(cebu::position const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "{}:{}",
            self.row, self.column
        ), ctx);
    }
};

namespace cebu
{

struct location
{
    std::string_view file_path;
    position         position;
};

}

template<>
struct std::formatter<cebu::location>
    : formatter<string>
{
    auto format(cebu::location const& self, format_context& ctx) const
    {
        return formatter<string>::format(std::format(
            "{}:{}",
            self.file_path, self.position
        ), ctx);
    }
};

namespace cebu
{

enum class error_type
{
    parsing
};

}

template<>
struct std::formatter<cebu::error_type>
    : formatter<string>
{
    auto format(cebu::error_type const& self, format_context& ctx) const
    {
        std::string_view str;
        switch (self) {
        case cebu::error_type::parsing:
            str = "parsing error";
            break;
        }
        return formatter<string>::format(std::format(
            "{}", str
        ), ctx);
    }
};

namespace cebu
{

class error
{
public:
    template<typename ...Args>
    error(
        error_type                   type,
        location                     location,
        std::string                  message
    ) noexcept
    {
        std::string format{std::format(
            "[{}@{}] {}",
            type, location, message
        )};
        std::cerr << format << std::endl;
    }
};

}
