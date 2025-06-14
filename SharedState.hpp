#ifndef SHARED_STATE_HPP
#define SHARED_STATE_HPP

#include <optional>
#include <semaphore>
#include <variant>

namespace multithreading::futurama {
template <class T>
class SharedState {
 public:
  template <class R>
  void Set(R&& new_val) noexcept {
    val_ = std::forward<R>(new_val);
    sem_.release();
  }

  T Get() {
    sem_.acquire();
    if (auto const e{std::get_if<std::exception_ptr>(&val_)}) {
      std::rethrow_exception(*e);
    } else {
      return std::get<T>(val_);
    }
  }

 private:
  std::binary_semaphore sem_{0};
  std::variant<std::monostate, T, std::exception_ptr> val_{};
};

template <>
class SharedState<void> {
 public:
  void Set() noexcept {
    sem_.release();
  }
  void Set(std::exception_ptr e) noexcept {
    e_ = e;
    sem_.release();
  }

  void Get() {
    sem_.acquire();
    if (e_) {
      std::rethrow_exception(e_);
    }
  }

 private:
  std::binary_semaphore sem_{0};
  std::exception_ptr e_{nullptr};
};
}  // namespace multithreading::futurama

#endif  // !SHARED_STATE_HPP