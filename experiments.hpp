#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP

#include "StatisticChunk.hpp"

namespace experiments {
namespace singlethread {
void process_data(config::DUMMY_DATA const& data);
}
namespace multithread {
std::vector<StatisticChunk> process_data_without_queue(config::DUMMY_DATA const& data);
std::vector<StatisticChunk> process_data_with_queue(config::DUMMY_DATA const& data);
void process_data_with_pool(config::DUMMY_DATA&& data);
void process_data_with_pool(std::vector<Job>&& data,
                            std::size_t async_threads_count,
                            std::size_t compute_threads_count);
void test_combined_data();
void test_combined_data_with_singleton(std::vector<Job>&& data);
void test_combined_data_with_functions(std::vector<Job>&& data);
}
}

#endif // !EXPERIMENTS_HPP