#pragma once
#define CEBU_INCLUDED_DIAGNOSTICS_H

#include <utility>
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

class result
{
public:
    enum variant
    {
        failure = 0,
        success = 1
    };

    constexpr result(variant value) noexcept
        : m_value{value}
    {}

    operator bool() const noexcept
    { return m_value; }

    constexpr variant value() const noexcept
    { return m_value; }

private:
    variant m_value;
};

}
