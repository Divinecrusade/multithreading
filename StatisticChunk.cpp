#include "StatisticChunk.hpp"

#include "config.hpp"

#include <filesystem>
#include <fstream>
#include <format>
#include <numeric>


void StatisticChunk::save_as_csv(std::span<StatisticChunk const> stats, std::filesystem::path uri)
{
    std::ofstream csv{ uri };
    for (std::size_t i{ 0ULL }; i < config::SLAVES_COUNT; ++i)
    {
        csv << std::format("work_{0:};idle_{0:};heavies_{0:};", i);
    }
    csv << "total_time;total_heavies;total_idle" << std::endl;

    for (auto const& chunk : stats)
    {
        long long total_idle{ 0LL };
        for (std::size_t i{ 0ULL }; i < config::SLAVES_COUNT; ++i)
        {
            auto const cur_idle{ chunk.total_timing - chunk.timing_per_thread[i] };
            csv << std::format("{};{};{};", chunk.timing_per_thread[i], cur_idle, chunk.number_of_heavy_jobs_per_thread[i]);
            total_idle += cur_idle;
        }
        csv << std::format("{};{};{}", chunk.total_timing, std::accumulate(chunk.number_of_heavy_jobs_per_thread.begin(), chunk.number_of_heavy_jobs_per_thread.end(), 0ULL), total_idle) << std::endl;
    }
}
