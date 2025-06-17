#ifndef MULTITHREADING_POOL_ASYNC_HPP
#define MULTITHREADING_POOL_ASYNC_HPP

#include "multithreading_pool_generic.hpp"

namespace multithreading::pool::async {
class TaskExecuter {
 private:
  TaskExecuter(std::size_t logical_cores_count);

 public:
  template <class F, typename... Args>
  static auto RunAsyncTask(F&& functor, Args&&... params) {
    return GetEntity().async_queue.Run(std::forward<F>(functor),
                                       std::forward<Args>(params)...);
  }

  template <class F, typename... Args>
  static auto RunProcessTask(F&& functor, Args&&... params) {
    return GetEntity().process_queue.Run(std::forward<F>(functor),
                                         std::forward<Args>(params)...);
  }

  template<class F1, class F2, typename... Args>
  static auto RunCombinedTask(F1&& functor_async, 
                              F2&& functor_process, 
                              Args&&... params) {
    return RunAsyncTask([functor_async = std::forward<F1>(functor_async),
                         functor_process = std::forward<F2>(functor_process),
                         ...params = std::forward<Args>(params)] {
      auto resource{RunAsyncTask(std::move(functor_async), std::move(params)...)};
      auto result{RunProcessTask(std::move(functor_process), resource.get())};
      return result.get();
    });
  }

 private:
  static TaskExecuter& GetEntity();

 private:
  generic::Master async_queue;
  generic::Master process_queue;
};
}

#endif  // !MULTITHREADING_POOL_ASYNC_HPP
