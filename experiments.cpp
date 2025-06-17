#include "experiments.hpp"
#include "multithreading.hpp"
#include "multithreading_queue.hpp"
#include "multithreading_pool_generic.hpp"
#include "multithreading_pool_async.hpp"

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

void experiments::multithread::process_data_with_pool(config::DUMMY_DATA&& data) {
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

void experiments::multithread::process_data_with_pool(
    std::vector<Job>&& data, 
    std::size_t async_threads_count,
    std::size_t compute_threads_count) {
  static constexpr auto pool_adapter{
      [](Job const& task) { return task.task->do_stuff(); }};

  Task::DUMMY_OUTPUT result{0ULL};
  using namespace multithreading::pool::generic;
  Master task_manager{compute_threads_count};
  auto futures{data |
               std::views::transform([&task_manager](auto&& task) {
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
                                                                     start_time)
                   .count()
            << "ms - multithread pool\n";
}

void experiments::multithread::test_combined_data() {
  using namespace multithreading::pool::async;
  using namespace std::chrono_literals;

  auto const get_thread_id{[](int miliseconds_count) {
    std::clog << "starting process...\n";
    auto const thread_id{std::this_thread::get_id()};
    std::this_thread::sleep_for(1ms * (miliseconds_count));
    return std::format("<< {} >>\n", thread_id);
  }};

  auto const get_miliseconds{[](int miliseconds_count) {
    std::clog << "starting async...\n";
    std::this_thread::sleep_for(2s + 50ms * (miliseconds_count % 50));
    return miliseconds_count * (100 + miliseconds_count % 20);
  }};
  
  auto futures{std::views::iota(1, 101) |
               std::views::transform([&](auto const& i) {
                  return TaskExecuter::RunCombinedTask(get_miliseconds, get_thread_id, i);
               }) |
               std::ranges::to<std::vector>()};
  for (auto& futa : futures) {
    try {
      std::cout << futa.get();
    } catch (std::runtime_error) {
      std::cerr << "exception from thread\n";
    }
  }
}

void experiments::multithread::test_combined_data(std::vector<Job>&& data) {
  using namespace multithreading::pool::async;
  using namespace std::chrono_literals;

  static constexpr auto task_async_delay { 
    [] { std::this_thread::sleep_for(400ms); }
  };
  static constexpr auto pool_adapter{[](Job&& task) {
    return task.task->do_stuff(); 
  }};
  
  Task::DUMMY_OUTPUT result{0ULL};
  auto futures{data | std::views::transform([&](auto&& task) {
                 return TaskExecuter::RunAsyncTask([&] { 
                  task_async_delay();
                  auto f{TaskExecuter::RunProcessTask(pool_adapter, std::move(task))};
                  return f.get();
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
            << "ms - multithread pool async\n";
}
