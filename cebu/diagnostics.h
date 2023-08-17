#pragma once
#define CEBU_INCLUDED_DIAGNOSTICS_H

#include <string_view>
#include <format>

namespace cebu
{

struct position
{
    std::size_t row;
    std::size_t column;
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

struct location
{
    source_file const& file;
    position           position;
};

}

namespace std
{

using namespace cebu;
    
template<>
struct formatter<position>
    : formatter<string>
{
    auto format(position const& self, format_context& ctx) const
    {

        return formatter<string>::format(std::format(
            "position {{ row: {}, column: {} }}",
            self.row, self.column
        ), ctx);
    }
};

}
