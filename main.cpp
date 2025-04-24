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

static constexpr auto DUMMY_DATA_SIZE{ std::numeric_limits<int>::max() / 32 };
using DUMMY_DATA = std::array<std::byte, DUMMY_DATA_SIZE>;

static void dummy_proccess(DUMMY_DATA& data, long long& acc)
{
    for (auto const& el : data)
    {
        double const x{ std::to_integer<int>(el) / 255. };
        double const y{ std::sin(std::cos(x)) };

        acc += static_cast<int>(std::round(y)) % 2;
    }
}

int main()
{
    std::vector<DUMMY_DATA> data{ 4 };
    std::vector<std::thread> workers{ };
    long long acc{ 0ll };
    
    for (auto& dummy : data)
    {
        std::ranges::generate(dummy, [rng = std::minstd_rand{ }]() mutable { return std::byte{ static_cast<unsigned char>(rng() % 256) }; });
    }

    {
        auto const start_time{ std::chrono::steady_clock::now() };
        for (auto& dummy : data)
        {
            dummy_proccess(dummy, acc);
        }
        auto const end_time{ std::chrono::steady_clock::now() };
        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (1 thread)\n";
        std::cout << "Result: " << acc;
    }
    std::puts("");
    constexpr int ITERATIONS_COUNT{ 5 };
    for (int i{ 1 }; i <= ITERATIONS_COUNT; ++i)
    {
        auto const start_time{ std::chrono::steady_clock::now() };
        acc = 0ll;
        for (auto& dummy : data)
        {
            workers.push_back(std::thread{ dummy_proccess, std::ref(dummy), std::ref(acc) });
        }
        for (auto& worker : workers)
        {
            worker.join();
        }
        workers.clear();
        auto const end_time{ std::chrono::steady_clock::now() };
        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms (4 threads)\n";
        std::cout << "Result: " << acc;
        std::puts("");
    }

    return EXIT_SUCCESS;
}