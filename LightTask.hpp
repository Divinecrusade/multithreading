#ifndef LIGHT_TASK_HPP
#define LIGHT_TASK_HPP

#include "Task.hpp"

class LightTask : public Task {
 public:
  static constexpr std::size_t ITERATIONS_COUNT{25ULL};
  DUMMY_OUTPUT do_stuff() const override {
    auto result{val};
    for (std::size_t i{0ULL}; i < ITERATIONS_COUNT; ++i) {
      result = std::sin(std::cos(result) * 10'000.);
      result = std::pow(val, result);
      result = std::exp(result);
    }
    return static_cast<DUMMY_OUTPUT>(std::round(result)) % 100;
  }
};
#endif  // !LIGHT_TASK_HPP