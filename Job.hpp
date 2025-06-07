#ifndef JOB_HPP
#define JOB_HPP

#include "HeavyTask.hpp"
#include "LightTask.hpp"

class Job {
 public:
  static constexpr auto CHANCE_OF_HEAVY_JOB_APPEARING{0.02};

  static std::unique_ptr<Task> generate_task() noexcept {
    static std::mt19937 rng{};
    static std::bernoulli_distribution is_heavy_job{
        CHANCE_OF_HEAVY_JOB_APPEARING};

    if (!is_heavy_job(rng))
      return std::make_unique<LightTask>();
    else
      return std::make_unique<HeavyTask>();
  }

 public:
  Job() = default;
  Job(std::unique_ptr<Task> task_init) : task{std::move(task_init)} {}

  std::unique_ptr<Task> task{generate_task()};
};

#endif  // !JOB_HPP
