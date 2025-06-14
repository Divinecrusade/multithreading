#ifndef FUTURAMA_TASK_HPP
#define FUTURAMA_TASK_HPP

#include <functional>
#include <future>

namespace multithreading::futurama {
class Task {
 public:
  Task() = default;
  Task(Task&&) = default;

  Task& operator=(Task&&) = default;

  operator bool() const {
    return static_cast<bool>(execute_);
  }

  template <class F, typename... Args>
  static auto make(F&& functor, Args&&... params) {
    std::promise<std::invoke_result_t<F, Args...>> promise{};
    auto future{promise.get_future()};
    return std::make_pair(
      Task{std::forward<F>(functor), std::move(promise), std::forward<Args>(params)...},
      std::move(future)
    );
  }
  void operator()() { execute_(); }

 private:
  template <class F, class P, typename... Args>
  Task(F&& functor, P&& promise, Args&&... params) 
  :
  execute_{ [
    functor = std::forward<F>(functor),
    promise = std::forward<P>(promise),
    ...params = std::forward<Args>(params)
  ]() mutable {
    try {
      if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
        functor(std::forward<Args>(params)...);
        promise.set_value();
      } else {
        promise.set_value(functor(std::forward<Args>(params)...));
      }
    } catch(...) {
      promise.set_exception(std::current_exception());
    }
  }}
  {}

 private:
  std::move_only_function<void()> execute_{};
};
} // multithreading::futurama

#endif  // !FUTURAMA_TASK_HPP
