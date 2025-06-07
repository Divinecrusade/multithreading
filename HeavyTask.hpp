#ifndef HEAVY_TASK_HPP
#define HEAVY_TASK_HPP

#include "Task.hpp"

class HeavyTask : public Task {
 public:
  static constexpr std::size_t ITERATIONS_COUNT{5'000ULL};

  DUMMY_OUTPUT do_stuff() const override {
    auto result{val};
    for (std::size_t i{0ULL}; i < ITERATIONS_COUNT; ++i) {
      result = std::sin(std::cos(result) * 10'000.);
      result = std::pow(val, result);
      result = std::exp(result);
      result = std::sqrt(result);
      result = std::pow(val, result);
      result = std::cos(std::sin(result) * 10'000.);
      result = std::pow(val, result);
      result = std::exp(result);
    }
    return static_cast<DUMMY_OUTPUT>(std::round(result)) % 100;
  }
};

#endif  // !HEAVY_TASK_HPP