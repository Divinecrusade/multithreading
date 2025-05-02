#include <iostream>
#include <array>
#include <vector>
#include <limits>
#include <chrono>
#include <ranges>
#include <random>
#include <algorithm>
#include <thread>
#include <cmath>
#include <mutex>
#include <functional>
#include <numeric>
#include <span>

static constexpr auto DUMMY_DATA_SIZE{ 1'000'000 };
using DUMMY_DATA = std::array<std::byte, DUMMY_DATA_SIZE>;
static constexpr auto BLOCKS_DATA_COUNT{ 4 };

using ACC = long long;
static constexpr auto ACC_SIZE{ sizeof(ACC) };
static constexpr decltype(ACC_SIZE) CACHE_DDR_BAND_WIDTH{ 64 };
static constexpr auto CACHE_DDR_PADDING{ CACHE_DDR_BAND_WIDTH - ACC_SIZE };
using PADDING = std::array<std::byte, CACHE_DDR_PADDING>;
using ACC_PADDED = std::pair<ACC, PADDING>;

static constexpr int ITERATIONS_COUNT{ 5 };


static void dummy_process_multi(std::span<std::byte const> data, ACC& acc) noexcept
{
    for (auto const& el : data)
    {
        double const x{ std::to_integer<int>(el) / 255. };
        double const y{ std::sin(std::cos(x)) };

        acc += static_cast<int>(std::round(y)) % 2;
    }
}

static void dummy_process(std::span<std::byte const> data, ACC& acc) noexcept
{
    for (auto const& el : data)
    {
        double const x{ std::to_integer<int>(el) / 255. };
        double const y{ std::sin(std::cos(x)) };

        acc += static_cast<int>(std::round(y)) % 2;
    }
}

static void process_data_in_one_shot(std::vector<DUMMY_DATA> const& data)
{
    std::puts("=========================");
    std::puts("Process data in one shot");
    {
        ACC acc{ 0ll };
        auto const start_time{ std::chrono::steady_clock::now() };
        for (auto const& dummy : data)
        {
            dummy_process(dummy, acc);
        }
        auto const end_time{ std::chrono::steady_clock::now() };
        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (1 thread)\n";
        std::cout << "Result: " << acc;
    }
    std::puts("");
    
    for (int i{ 1 }; i <= ITERATIONS_COUNT; ++i)
    {
        std::vector<std::thread> workers{ };
        ACC acc{ 0ll };

        auto const start_time{ std::chrono::steady_clock::now() };

        for (auto const& [i, block] : std::views::enumerate(data))
        {
            workers.push_back(std::thread{ dummy_process_multi, std::span{ block }, std::ref(acc) });
        }
        for (auto& worker : workers)
        {
            worker.join();
        }

        auto const end_time{ std::chrono::steady_clock::now() };

        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (4 threads write in one field)\n";
        std::cout << "Result: " << acc;
        std::puts("");
    }

    for (int i{ 1 }; i <= ITERATIONS_COUNT; ++i)
    {
        std::vector<std::thread> workers{ };

        auto const start_time{ std::chrono::steady_clock::now() };

        for (auto const& [i, block] : std::views::enumerate(data))
        {
            workers.push_back(std::thread{ [dummy = ACC(0)](DUMMY_DATA const& block) mutable { dummy_process_multi(std::span{ block }, dummy); }, std::ref(block)});
        }
        for (auto& worker : workers)
        {
            worker.join();
        }

        auto const end_time{ std::chrono::steady_clock::now() };

        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (4 threads write in lamda field)";
        std::puts("");
    }

    for (int i{ 1 }; i <= ITERATIONS_COUNT; ++i)
    {
        std::vector<std::thread> workers{ };
        std::array<ACC, BLOCKS_DATA_COUNT> accs{ };

        auto const start_time{ std::chrono::steady_clock::now() };

        for (auto const& [i, block] : std::views::enumerate(data))
        {
            workers.push_back(std::thread{ dummy_process_multi, std::span{ block }, std::ref(accs[i])});
        }
        for (auto& worker : workers)
        {
            worker.join();
        }

        auto const end_time{ std::chrono::steady_clock::now() };

        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (4 threads without padding)\n";
        std::cout << "Result: " << std::accumulate(accs.cbegin(), accs.cend(), static_cast<ACC>(0));
        std::puts("");
    }

    for (int i{ 1 }; i <= ITERATIONS_COUNT; ++i)
    {
        std::vector<std::thread> workers{ };
        std::array<ACC_PADDED, BLOCKS_DATA_COUNT> accs{ };
        
        auto const start_time{ std::chrono::steady_clock::now() };
        
        for (auto const& [i, block] : std::views::enumerate(data))
        {
            workers.push_back(std::thread{ dummy_process_multi, std::span{ block }, std::ref(accs[i].first) });
        }
        for (auto& worker : workers)
        {
            worker.join();
        }

        auto const end_time{ std::chrono::steady_clock::now() };

        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (4 threads with padding)\n";
        std::cout << "Result: " << std::accumulate(accs.cbegin(), accs.cend(), static_cast<ACC>(0), [](auto const& lhs, auto const& rhs) -> ACC { return lhs + rhs.first; });
        std::puts("");
    }
    std::puts("=========================");
}

static void process_data_in_multi_shots(std::vector<DUMMY_DATA> const& data)
{
    std::puts("=========================");

    for (int i{ 1 }; i <= ITERATIONS_COUNT; ++i)
    {
        std::array<ACC_PADDED, BLOCKS_DATA_COUNT> accs{ };

        auto const start_time{ std::chrono::steady_clock::now() };

        constexpr std::size_t CHUNK_SIZE{ DUMMY_DATA_SIZE / 1'000ull };
        for (std::size_t j{ 0ull }; j != DUMMY_DATA_SIZE; j += CHUNK_SIZE)
        {
            for (std::vector<std::jthread> workers{ }; auto const& [i, block] : std::views::enumerate(data))
            {
                workers.push_back(std::jthread{ dummy_process_multi, std::span{ block.begin() + j, CHUNK_SIZE }, std::ref(accs[i].first)});
            }
        }

        auto const end_time{ std::chrono::steady_clock::now() };

        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (4 threads with padding)\n";
        std::cout << "Result: " << std::accumulate(accs.cbegin(), accs.cend(), static_cast<ACC>(0), [](auto const& lhs, auto const& rhs) -> ACC { return lhs + rhs.first; });
        std::puts("");
    }
    std::puts("=========================");
}


int main(int argc, char const* (argv)[])
{
    std::vector<DUMMY_DATA> data{ BLOCKS_DATA_COUNT };

    for (auto& dummy : data)
    {
        std::ranges::generate(dummy, [rng = std::minstd_rand{ }]() mutable { return std::byte{ static_cast<unsigned char>(rng() % 256) }; });
    }
    
    process_data_in_one_shot(data);
    process_data_in_multi_shots(data);

    return EXIT_SUCCESS;
}