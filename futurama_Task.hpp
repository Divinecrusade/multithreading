#ifndef FUTURAMA_TASK_HPP
#define FUTURAMA_TASK_HPP

#include "Promise.hpp"
#include <functional>

namespace multithreading::futurama {
class Task {
 public:
  Task() = default;

  operator bool() const {
    return static_cast<bool>(execute_);
  }

  template <class F, typename... Args>
  static auto make(F&& functor, Args&&... params) {
    Promise<std::invoke_result_t<F, Args...>> promise{};
    auto future{promise.GetFuture()};
    return std::make_pair(
      Task{std::forward<F>(functor), std::move(promise), std::forward<Args>(params)...},
      std::move(future)
    );
  }
  void operator()() const { execute_(); }

 private:
  template <class F, class P, typename... Args>
  Task(F&& functor, P&& promise, Args&&... params) 
  :
  execute_{ [
    functor = std::forward<F>(functor),
    promise = std::forward<P>(promise),
    ...params = std::forward<Args>(params)
  ]() mutable {
    promise.SetResult(functor(params...));
  }}
  {}

 private:
  std::function<void()> execute_{};
};
} // multithreading::futurama

#endif  // !FUTURAMA_TASK_HPP
