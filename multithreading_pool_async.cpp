#include "multithreading_pool_async.hpp"

namespace multithreading::pool::async {
TaskExecuter::TaskExecuter(std::size_t logical_cores_count)
:
async_queue{logical_cores_count * 10ULL},
process_queue{logical_cores_count}
{}

TaskExecuter& TaskExecuter::GetEntity() {
  static TaskExecuter entity{std::thread::hardware_concurrency() * 2};
  return entity;
}
}
