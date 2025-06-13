#include "experiments.hpp"
#include "multithreading.hpp"
#include "multithreading_queue.hpp"
#include "multithreading_pool_generic.hpp"
#include "Promise.hpp"

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

void experiments::multithread::test_pool_generic() {
  using namespace multithreading::pool::generic;
  using namespace multithreading::futurama;
  using namespace std::chrono_literals;

  //Master task_manager{std::thread::hardware_concurrency() * 2};
  //auto const get_thread_id{[] {
  //  auto const thread_id{std::this_thread::get_id()};
  //  std::this_thread::sleep_for(100ms);
  //  std::clog << std::format("<< {} >>\n", thread_id) << std::flush;
  //}};

  //std::this_thread::sleep_for(200ms);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.Run(get_thread_id);
  //task_manager.WaitForAll();
  
  Promise<int> ticket{};
  auto fut{ticket.GetFuture()};
  std::thread{ [](Promise<int> state){
    std::this_thread::sleep_for(3s);
    state.SetResult(69);
  }, std::move(ticket) }.detach();
  std::clog << fut.GetResult();
}
