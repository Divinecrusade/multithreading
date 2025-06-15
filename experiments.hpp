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
void test_pool_generic();
}
}

#endif // !EXPERIMENTS_HPP