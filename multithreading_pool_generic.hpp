#ifndef MULTITHREADING_POOL_GENERIC_HPP
#define MULTITHREADING_POOL_GENERIC_HPP

#include <functional>
#include <condition_variable>
#include <thread>
#include <optional>
#include <cassert>
#include <utility>
#include <algorithm>
#include <vector>
#include <memory>

namespace multithreading::pool::generic {

using Task = std::function<void()>;

class Slave {
 public:

  [[nodiscard]] bool IsRunning() const noexcept { return cur_task_.has_value(); }
  void SetTask(Task new_task) noexcept { 
    assert(!IsRunning());
    cur_task_ = std::move(new_task);
  }

 private:

  void KernelRoutine() {
    std::unique_lock lck{mtx_};
    auto const stop_tkn{cur_thread_.get_stop_token()};

    while (cv_.wait(lck, stop_tkn, [this] { return IsRunning(); })) {
      (*cur_task_)();
      cur_task_.reset();
    }
  }

 private:

  std::mutex mtx_{};
  std::condition_variable_any cv_{};
  std::optional<Task> cur_task_{};
  std::jthread cur_thread_{&Slave::KernelRoutine, this};
};

class Master {
 public:
  
  void Run(Task task) noexcept { 
    if (auto const free_slave{std::ranges::find_if(
            slaves_, [](auto const& slave) { 
          return !slave->IsRunning();
        })};
        free_slave != slaves_.end()) {
      (*free_slave)->SetTask(std::move(task));
    } else {
      slaves_.emplace_back(std::make_unique<Slave>());
      slaves_.back()->SetTask(std::move(task));
    }
  }

 private:

  std::vector<std::unique_ptr<Slave>> slaves_{};
};

  void do_experiment();
}

#endif // !MULTITHREADING_GENERIC_POOL_HPP
