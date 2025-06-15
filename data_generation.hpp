#ifndef DATA_GENERATION_HPP
#define DATA_GENERATION_HPP

#include "config.hpp"

namespace data_generation {
config::DUMMY_DATA get_evened();
config::DUMMY_DATA get_stacked();
std::vector<Job> get_dynamic(std::size_t all_tasks_count, std::size_t heavy_tasks_count);
}  // namespace data_generation

#endif  // !DATA_GENERATION_HPP
