#include "experiments.hpp"
#include "multithreading.hpp"
#include "multithreading_queue.hpp"
#include "multithreading_pool_generic.hpp"
#include "multithreading_pool_stealing.hpp"

#include <ranges>
#include <iostream>

void experiments::singlethread::process_data(config::DUMMY_DATA const& data) {
  Task::DUMMY_OUTPUT result{0ULL};
  long long total_time{0LL};

  for (auto const& [i, chunk] : std::views::enumerate(data)) {
    auto const start_time_task{std::chrono::steady_clock::now()};
    for (auto const& dummy_process : chunk) {
      result += dummy_process.task->do_stuff();
    }
    auto const end_time_task{std::chrono::steady_clock::now()};

    total_time += std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_time_task - start_time_task)
                      .count();
  }
  std::clog << "Result: " << result << " | Done in " << total_time
            << "ms - singlethread\n";
}

std::vector<StatisticChunk> 
experiments::multithread::process_data_without_queue(config::DUMMY_DATA const& data) {
  using namespace multithreading;

  Master control_block{};
  std::vector<Slave> slaves{};
  std::generate_n(std::back_inserter(slaves), config::SLAVES_COUNT,
                  [&control_block] { return Slave{control_block}; });

  std::vector<StatisticChunk> results{};
  results.reserve(config::CHUNKS_COUNT);

  Task::DUMMY_OUTPUT result{0ULL};
  long long total_time{0LL};
  for (auto const& chunk : data) {
    constexpr auto SLAVE_LOADOUT{config::CHUNK_SIZE / config::SLAVES_COUNT};
    if (chunk.size() < SLAVE_LOADOUT) continue;

    auto const start_time_chunk{std::chrono::steady_clock::now()};
    for (auto const& [j, slave] : std::views::enumerate(slaves)) {
      slave.set_job(
          config::SLAVE_JOB{chunk.begin() + j * SLAVE_LOADOUT, SLAVE_LOADOUT});
    }
    control_block.wait_for_slaves();
    for (auto const& slave : slaves) {
      result += slave.get_result();
    }
    auto const end_time_chunk{std::chrono::steady_clock::now()};

    results.emplace_back();
    for (auto const& [j, slave] : std::views::enumerate(slaves)) {
      results.back().timing_per_thread[j] = slave.get_work_time_elapsed();
      results.back().number_of_heavy_jobs_per_thread[j] =
          slave.get_heavy_jobs_count();
    }
    auto const chunk_time{std::chrono::duration_cast<std::chrono::milliseconds>(
                              end_time_chunk - start_time_chunk)
                              .count()};
    total_time += chunk_time;
    results.back().total_timing = chunk_time;
  }
  std::clog << "Result: " << result << " | Done in " << total_time
            << "ms - multithread\n";

  return results;
}

std::vector<StatisticChunk>
experiments::multithread::process_data_with_queue(config::DUMMY_DATA const& data) {
  using namespace multithreading::queue;

  Master control_block{};
  std::vector<Slave> slaves{};
  std::generate_n(std::back_inserter(slaves), config::SLAVES_COUNT,
                  [&control_block] { return Slave{control_block}; });

  std::vector<StatisticChunk> results{};
  results.reserve(config::CHUNKS_COUNT);

  Task::DUMMY_OUTPUT result{0ULL};
  long long total_time{0LL};
  for (auto const& chunk : data) {
    auto const start_time_chunk{std::chrono::steady_clock::now()};
    control_block.add_workload(chunk);
    for (auto& slave : slaves) {
      slave.chunk_load();
    }
    control_block.wait_for_slaves();
    for (auto const& slave : slaves) {
      result += slave.get_result();
    }
    auto const end_time_chunk{std::chrono::steady_clock::now()};

    results.emplace_back();
    for (auto const& [j, slave] : std::views::enumerate(slaves)) {
      results.back().timing_per_thread[j] = slave.get_work_time_elapsed();
      results.back().number_of_heavy_jobs_per_thread[j] =
          slave.get_heavy_jobs_count();
    }
    auto const chunk_time{std::chrono::duration_cast<std::chrono::milliseconds>(
                              end_time_chunk - start_time_chunk)
                              .count()};
    total_time += chunk_time;
    results.back().total_timing = chunk_time;
  }
  std::clog << "Result: " << result << " | Done in " << total_time
            << "ms - multithread queued\n";

  return results;
}

void experiments::multithread::process_data_with_pool(
    config::DUMMY_DATA const& data) {
  static constexpr auto pool_adapter{
      [](Job const& task) { return task.task->do_stuff(); }};

  Task::DUMMY_OUTPUT result{0ULL};
  using namespace multithreading::pool::generic;
  Master task_manager{config::SLAVES_COUNT};
  auto futures{data | std::views::join | std::views::transform([&task_manager](auto&& task) {
                 return task_manager.Run(pool_adapter, std::move(task));
               }) |
               std::ranges::to<std::vector>()};

  auto const start_time{std::chrono::steady_clock::now()};
  for (auto& futa : futures) {
    result += futa.get();
  }
  auto const end_time{std::chrono::steady_clock::now()};

  std::clog << "Result: " << result << " | Done in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                     start_time).count()
            << "ms - multithread pool\n";
}

void experiments::multithread::process_data_with_pool_dynamic(
    std::vector<Job> const& data, 
    std::size_t async_threads_count,
    std::size_t compute_threads_count) {
  static constexpr auto pool_adapter{
      [](Job const& task) { return task.task->do_stuff(); }};

  Task::DUMMY_OUTPUT result{0ULL};
  using namespace multithreading::pool::generic;
  Master task_manager{compute_threads_count};
  auto futures{data |
               std::views::transform([&task_manager](auto const& task) {
                 return task_manager.Run(pool_adapter, task);
               }) |
               std::ranges::to<std::vector>()};

  auto const start_time{std::chrono::steady_clock::now()};
  for (auto& futa : futures) {
    result += futa.get();
  }
  auto const end_time{std::chrono::steady_clock::now()};

  std::clog << "Result: " << result << " | Done in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                     start_time)
                   .count()
            << "ms - multithread pool\n";
}

void experiments::multithread::process_data_with_pool_stealing(
    std::vector<Job> const& data) {
  using namespace multithreading::pool::stealing;
  using namespace std::chrono_literals;

  static constexpr auto task_async_delay{
      [] { std::this_thread::sleep_for(40ms); return 20.; }};
  static constexpr auto pool_adapter{
      [](Job const& task) { return task.task->do_stuff(); }};

  auto const logical_cores_number{std::thread::hardware_concurrency() * 2};
  TaskExecuter cur_exec{logical_cores_number * 10, logical_cores_number};
  Task::DUMMY_OUTPUT result{0ULL};
  auto futures{
      data | std::views::transform([&](auto const& task) {
        return cur_exec.async_queue.Dispatch([&task] {
          auto tmp{task_async_delay()};
          return RunProcessTask(pool_adapter, task).get() / tmp;
        });
      }) |
      std::ranges::to<std::vector>()};

  auto const start_time{std::chrono::steady_clock::now()};
  for (auto& futa : futures) {
    result += futa.get();
  }
  auto const end_time{std::chrono::steady_clock::now()};

  std::clog << "Result: " << result << " | Done in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                     start_time)
                   .count()
            << "ms - multithread pool stealing\n";
}
