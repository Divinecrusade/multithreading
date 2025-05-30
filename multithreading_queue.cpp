#include "multithreading_queue.hpp"

#include <ranges>
#include <iostream>

namespace multithreading::queue
{
    std::vector<StatisticChunk> do_experiment(config::DUMMY_DATA const& data)
    {
        Master control_block{ };
        std::vector<Slave> slaves{ };
        std::generate_n(std::back_inserter(slaves), config::SLAVES_COUNT, [&control_block] { return Slave{ control_block }; });

        std::vector<StatisticChunk> results{ };
        results.reserve(config::CHUNKS_COUNT);

        Task::DUMMY_OUTPUT result{ 0ULL };
        long long total_time{ 0LL };
        for (auto const& chunk : data)
        {
            auto const start_time_chunk{ std::chrono::steady_clock::now() };
            control_block.add_workload(chunk);
            for (auto& slave : slaves)
            {
                slave.chunk_load();
            }
            control_block.wait_for_slaves();
            for (auto const& slave : slaves)
            {
                result += slave.get_result();
            }
            auto const end_time_chunk{ std::chrono::steady_clock::now() };

            results.emplace_back();
            for (auto const& [j, slave] : std::views::enumerate(slaves))
            {
                results.back().timing_per_thread[j] = slave.get_work_time_elapsed();
                results.back().number_of_heavy_jobs_per_thread[j] = slave.get_heavy_jobs_count();
            }
            auto const chunk_time{ std::chrono::duration_cast<std::chrono::milliseconds>(end_time_chunk - start_time_chunk).count() };
            total_time += chunk_time;
            results.back().total_timing = chunk_time;
        }
        std::clog << "Result: " << result << " | Done in " << total_time << "ms - multithread queued\n";

        return results;
    }
}