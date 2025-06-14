#ifndef FUTURE_HPP
#define FUTURE_HPP

#include "SharedState.hpp"

#include <memory>

namespace multithreading::futurama {
template<class T>
class Future {
 public:
  Future(std::shared_ptr<SharedState<T>> state) noexcept : state_{state} {}

  T GetResult() const noexcept {
    return state_->Get();
  }

 private:
  std::shared_ptr<SharedState<T>> state_;
};
}  // namespace multithreading::futurama

#endif  // !FUTURE_HPP
