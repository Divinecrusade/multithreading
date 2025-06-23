#ifndef MULTITHREADING_POOL_FUN_HPP
#define MULTITHREADING_POOL_FUN_HPP

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>
#include <gsl/gsl>

namespace multithreading::pool::fun {
struct TaskExecuter;

TaskExecuter& ThisExecuter(TaskExecuter* init_executer = nullptr) {
  thread_local TaskExecuter* cur_executer{init_executer};
  return *cur_executer;
}

class Master {
 private:
  class Slave {
   public:
    Slave(Master& tasks_pool, TaskExecuter* cur_executer) 
    : tasks_pool_{tasks_pool},
      cur_thread_{std::bind_front(&Slave::KernelRoutine, this, cur_executer)} {
    }

    void Kill() noexcept { cur_thread_.request_stop(); }

   private:
    void KernelRoutine(TaskExecuter* cur_executer, 
                       std::stop_token const& st) const noexcept {
      ThisExecuter(cur_executer);
      while (auto cur_task{tasks_pool_.GetTask(st)}) {
        cur_task();
      }
    }

   private:
    Master& tasks_pool_;
    std::jthread cur_thread_;
  };

 public:
  Master(std::size_t slaves_count, gsl::not_null<TaskExecuter*> cur_executer) noexcept {
    slaves_.reserve(slaves_count);
    for (auto i : std::views::iota(0ULL, slaves_count)) {
      slaves_.emplace_back(*this, cur_executer);
    }
  }

  ~Master() noexcept {
    for (auto& slave : slaves_) {
      slave.Kill();
    }
  }

  template <class F, typename... Args>
  auto Dispatch(F&& functor, Args&&... params) noexcept {
    using functor_return_t = std::invoke_result_t<F, Args...>;
    std::packaged_task<functor_return_t(Args...)> pack{functor};
    auto future{pack.get_future()};
    auto task{[... params = std::forward<Args>(params),
               pack = std::move(pack)] mutable { pack(std::move(params)...); }};
    std::ignore = std::lock_guard{queue_mtx_},
    remaining_tasks_.push(std::move(task));

    queue_cv_.notify_one();

    return future;
  }

 private:
  std::move_only_function<void()> GetTask(std::stop_token const& st) {
    std::unique_lock lk{queue_mtx_};
    queue_cv_.wait(lk, st,
                   [&tasks = remaining_tasks_]() { return !tasks.empty(); });
    if (!st.stop_requested()) {
      auto cur_task{std::move(remaining_tasks_.front())};
      remaining_tasks_.pop();
      return std::move(cur_task);
    } else {
      return {};
    }
  }

 private:
  std::queue<std::move_only_function<void()>> remaining_tasks_{};

  std::mutex queue_mtx_{};
  std::condition_variable_any queue_cv_{};
  std::vector<Slave> slaves_{};
};

struct TaskExecuter {
  TaskExecuter(std::size_t async_cores_count, std::size_t process_cores_count)
      : async_queue{async_cores_count, this},
        process_queue{process_cores_count, this}
  { }

  Master async_queue;
  Master process_queue;
};

template <class F, typename... Args>
inline auto RunAsyncTask(F&& functor, Args&&... params) {
  return ThisExecuter().async_queue.Dispatch(std::forward<F>(functor),
                                             std::forward<Args>(params)...);
}

template <class F, typename... Args>
inline auto RunProcessTask(F&& functor, Args&&... params) {
  return ThisExecuter().process_queue.Dispatch(std::forward<F>(functor),
                                               std::forward<Args>(params)...);
}
}  // namespace multithreading::pool::fun

#endif  // !MULTITHREADING_POOL_FUN_HPP
