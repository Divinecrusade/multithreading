#ifndef SHARED_STATE_HPP
#define SHARED_STATE_HPP

#include <optional>
#include <concepts>
#include <semaphore>

namespace multithreading::futurama {
template <class T>
class SharedState {
 public:
  template <class R>
  requires std::constructible_from<T, R>
  void Set(R&& new_val) noexcept {
    sem.release();
    val_ = std::forward<R>(new_val);
    // TODO: compile restrict, val should be initilised only once
  }

  T Get() noexcept {
    sem.acquire();
    return std::move(val_.value_or(T{}));
    // TODO: compile restrict, val should be getted only once
  }

 private:
  std::binary_semaphore sem{0};
  std::optional<T> val_{std::nullopt};
};

template <>
class SharedState<void> {
 public:
  void Set() noexcept {
    sem.release();
  }

  void Get() noexcept {
    sem.acquire();
  }

 private:
  std::binary_semaphore sem{0};
};
}  // namespace multithreading::futurama

#endif  // !SHARED_STATE_HPP