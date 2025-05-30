#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Job.hpp"

#include <span>
#include <array>

namespace config
{
    constexpr std::size_t CHUNKS_COUNT{ 100ull };
    constexpr std::size_t CHUNK_SIZE{ 10'000ull };
    constexpr std::size_t SLAVES_COUNT{ 4ull };
    static_assert(CHUNK_SIZE >= SLAVES_COUNT);
    static_assert(CHUNK_SIZE% SLAVES_COUNT == 0ull);

    using CHUNK = std::vector<Job>;
    using DUMMY_DATA = std::array<CHUNK, CHUNKS_COUNT>;
}
#endif // !CONFIG_HPP