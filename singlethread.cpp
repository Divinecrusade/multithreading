#include "singlethread.hpp"

namespace singlethread
{
    void do_experiment(config::DUMMY_DATA const& data)
    {
        Task::DUMMY_OUTPUT result{ 0ULL };
        long long total_time{ 0LL };
        for (auto const& [i, chunk] : std::views::enumerate(data))
        {
            auto const start_time_task{ std::chrono::steady_clock::now() };
            for (auto const& dummy_process : chunk)
            {
                result += dummy_process.task->do_stuff();
            }
            auto const end_time_task{ std::chrono::steady_clock::now() };
            total_time += std::chrono::duration_cast<std::chrono::milliseconds>(end_time_task - start_time_task).count();
        }
        std::clog << "Result: " << result << " | Done in " << total_time << "ms - singlethread\n";
    }
}