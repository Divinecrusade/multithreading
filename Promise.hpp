#ifndef PROMISE_HPP
#define PROMISE_HPP

#include "Future.hpp"

namespace multithreading::futurama{ 
template<class T>
class Promise {
 public:
  template<class R>
  void SetResult(R&& result) {
    state_->Set(std::forward<R>(result));
  }

  Future<T> GetFuture() const {
    return {state_};
  }

 private:
  std::shared_ptr<SharedState<T>> state_{std::make_shared<SharedState<T>>()};
};
}  // namespace multithreading::futurama

#endif  // !PROMISE_HPP
