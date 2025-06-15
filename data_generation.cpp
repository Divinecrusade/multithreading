#include "data_generation.hpp"
#include <cassert>

namespace data_generation {
config::DUMMY_DATA get_evened() {
  config::DUMMY_DATA dataset{};
  for (auto& chunk : dataset) {
    std::generate_n(std::back_inserter(chunk), config::CHUNK_SIZE,
                    [counter_max = static_cast<std::size_t>(
                         std::round(1. / Job::CHANCE_OF_HEAVY_JOB_APPEARING)),
                     counter = 0ULL]() mutable {
                      ++counter;
                      counter %= counter_max;
                      if (counter == 0ULL)
                        return Job{std::make_unique<HeavyTask>()};
                      else
                        return Job{std::make_unique<LightTask>()};
                    });
  }
  return dataset;
}

config::DUMMY_DATA get_stacked() {
  config::DUMMY_DATA dataset{get_evened()};
  for (auto& chunk : dataset) {
    std::ranges::partition(chunk, [](auto const& dummy_process) {
      return typeid(*dummy_process.task.get()) == typeid(HeavyTask const&);
    });
  }
  return dataset;
}
std::vector<Job> get_dynamic(
  std::size_t all_tasks_count,
  std::size_t heavy_tasks_count) {
  assert(all_tasks_count >= heavy_tasks_count);

  std::vector<Job> dataset{};
  dataset.reserve(all_tasks_count);

  std::generate_n(std::back_inserter(dataset), heavy_tasks_count,
                  [] { return Job{std::make_unique<HeavyTask>()}; });
  std::generate_n(std::back_inserter(dataset), all_tasks_count - heavy_tasks_count,
                  [] { return Job{std::make_unique<LightTask>()}; });

  return dataset;
}
}