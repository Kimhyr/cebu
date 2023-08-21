#include <fstream>
#include <sstream>

#include "parser.h"

namespace cebu
{

char const* end_of_file_error::what() const noexcept
{
    return "end of file";
}

void parser::unsafely_load_file(std::string_view const& file_path)
{
    std::ifstream file{file_path.begin()};
    std::ostringstream sstr;
    sstr << file.rdbuf();
    m_source = sstr.str();
    m_lexer.load(file_path, m_source.begin());
}

}
