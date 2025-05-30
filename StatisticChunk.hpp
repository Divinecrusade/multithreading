#ifndef STATISTIC_CHUNK_HPP
#define STATISTIC_CHUNK_HPP

#include "config.hpp"

#include <array>
#include <filesystem>

struct StatisticChunk
{
    std::array<long long, config::SLAVES_COUNT> timing_per_thread{ };
    std::array<std::size_t, config::SLAVES_COUNT> number_of_heavy_jobs_per_thread{ };
    long long total_timing{ };

    static void save_as_csv(std::span<StatisticChunk const> stats, std::filesystem::path uri);
};


#endif // !STATISTIC_CHUNK_HPP