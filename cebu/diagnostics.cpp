#include <stdexcept>

#include "diagnostics.h"

namespace cebu
{

source_file::source_file(std::string_view file_path)
{
    // Open the file.
    int descriptor{open(file_path.cbegin(), O_RDWR)};
    if (descriptor == -1) [[unlikely]]
        throw std::invalid_argument{"open"};

    // Get the size of the file.
    struct stat file_stats;
    if (fstat(descriptor, &file_stats) == -1) [[unlikely]]
        throw std::invalid_argument{"fstat"};
    m_size = static_cast<std::size_t>(file_stats.st_size);

    // Memory map the file.
    m_map = reinterpret_cast<char const*>(
        mmap(nullptr, m_size,
             PROT_READ | PROT_WRITE,
             MAP_PRIVATE, descriptor,
             0));
    if (m_map == MAP_FAILED) [[unlikely]]
        throw std::invalid_argument{"mmap"};

    // Close the file.
    if (close(descriptor) == -1) [[unlikely]]
        throw std::invalid_argument{"close"};

    // Finalize.
    m_path = file_path;
}

source_file::~source_file()
{
    munmap(const_cast<char*>(m_map), m_size);
}

}
