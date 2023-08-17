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
            "{}",
            str
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
        std::string                  message,
        std::function<void(Args...)> recover,
        Args&&...                    args
    ) noexcept
    {
        std::string format{std::format(
            "[{}@{}] {}",
            type, location, message
        )};
        std::cerr << format << std::endl;
        recover(std::forward<Args>(args)...);
    }
};

class source_file
{
public:
    source_file(std::string_view path);

    ~source_file();

    [[nodiscard]]
    char const* map() const noexcept
    { return m_map; }

    [[nodiscard]]
    std::string_view const& path() const noexcept
    { return m_path; }

    [[nodiscard]]
    std::size_t size() const noexcept
    { return m_size; }

private:
    char const*      m_map;
    std::string_view m_path;
    std::size_t      m_size;
};

}
