#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Job.hpp"

#include <array>
#include <span>

namespace config {
constexpr std::size_t CHUNKS_COUNT{100ULL};
constexpr std::size_t CHUNK_SIZE{10'000ULL};
constexpr std::size_t SLAVES_COUNT{4ULL};
static_assert(CHUNK_SIZE >= SLAVES_COUNT);
static_assert(CHUNK_SIZE % SLAVES_COUNT == 0ULL);

using CHUNK = std::vector<Job>;
using DUMMY_DATA = std::array<CHUNK, CHUNKS_COUNT>;
}  // namespace config
#endif  // !CONFIG_HPP