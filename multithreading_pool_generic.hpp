#ifndef MULTITHREADING_POOL_GENERIC_HPP
#define MULTITHREADING_POOL_GENERIC_HPP

#include "futurama_Task.hpp"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

namespace multithreading::pool::generic {

class Master {
 private:
  class Slave {
   public:
    Slave(Master& tasks_pool) : tasks_pool_{tasks_pool} {}

    void Kill() noexcept { cur_thread_.request_stop(); }

   private:
    void KernelRoutine(std::stop_token const& st) const noexcept {
      while (auto const cur_task{tasks_pool_.GetTask(st)}) {
        cur_task();
      }
    }

   private:
    Master& tasks_pool_;

    std::jthread cur_thread_{std::bind_front(&Slave::KernelRoutine, this)};
  };

 public:
  Master(std::size_t slaves_count) noexcept {
    slaves_.reserve(slaves_count);
    for (auto i : std::views::iota(1ULL, slaves_count)) {
      slaves_.emplace_back(*this);
    }
  }

  ~Master() noexcept {
    for (auto& slave : slaves_) {
      slave.Kill();
    }
  }

  template <class F, typename... Args>
  auto Run(F&& functor, Args&&... params) noexcept {
    auto [task, future] {multithreading::futurama::Task::make(std::forward<F>(functor), std::forward<Args>(params)...)};
    std::ignore = std::lock_guard{queue_mtx_},
    remaining_tasks_.push(std::move(task));

    queue_cv_.notify_one();

    return future;
  }

  void WaitForAll() {
    std::unique_lock lk{queue_mtx_};
    wait_cv_.wait(lk, [&tasks = remaining_tasks_]() { return tasks.empty(); });
  }

 private:
  multithreading::futurama::Task GetTask(std::stop_token const& st) noexcept {
    std::unique_lock lk{queue_mtx_};
    queue_cv_.wait(lk, st,
                   [&tasks = remaining_tasks_]() { return !tasks.empty(); });
    if (!st.stop_requested()) {
      auto const cur_task{std::move(remaining_tasks_.front())};
      remaining_tasks_.pop();
      if (remaining_tasks_.empty()) {
        wait_cv_.notify_all();
      }
      return cur_task;
    } else {
      return {};
    }
  }

 private:
  std::queue<multithreading::futurama::Task> remaining_tasks_{};

  std::condition_variable wait_cv_{};
  std::mutex queue_mtx_{};
  std::condition_variable_any queue_cv_{};
  std::vector<Slave> slaves_{};
};
}  // namespace multithreading::pool::generic

#endif  // !MULTITHREADING_GENERIC_POOL_HPP
