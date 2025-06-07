#ifndef TASK_HPP
#define TASK_HPP

#include <numbers>
#include <random>

class Task {
 public:
  using DUMMY_INPUT = double;
  using DUMMY_OUTPUT = int;
  static constexpr DUMMY_INPUT DUMMY_INPUT_MIN{0};
  static constexpr DUMMY_INPUT DUMMY_INPUT_MAX{std::numbers::pi};

  static DUMMY_INPUT generate_val() noexcept {
    static std::minstd_rand rng{};
    static std::uniform_real_distribution<DUMMY_INPUT> val_range{
        DUMMY_INPUT_MIN, DUMMY_INPUT_MAX};

    return val_range(rng);
  }

  virtual DUMMY_OUTPUT do_stuff() const = 0;

 protected:
  DUMMY_INPUT val{generate_val()};
};

#endif  // !TASK_HPP