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
#include <cassert>
#include <numbers>
#include <format>
#include <fstream>


struct Task
{
    using DUMMY_INPUT = double;
    using DUMMY_OUTPUT = int;
    static constexpr DUMMY_INPUT DUMMY_INPUT_MIN{ 0 };
    static constexpr DUMMY_INPUT DUMMY_INPUT_MAX{ std::numbers::pi };

    static DUMMY_INPUT generate_val() noexcept
    {
        static std::minstd_rand rng{ };
        static std::uniform_real_distribution<DUMMY_INPUT> val_range{ DUMMY_INPUT_MIN, DUMMY_INPUT_MAX };

        return val_range(rng);
    }

    virtual DUMMY_OUTPUT do_stuff() = 0;

protected:

    DUMMY_INPUT val{ generate_val() };
};

struct LightJob : public Task
{
    static constexpr std::size_t ITERATIONS_COUNT{ 25ULL };
    DUMMY_OUTPUT do_stuff() override
    {
        auto result{ val };
        for (std::size_t i{ 0ULL }; i < ITERATIONS_COUNT; ++i)
        {
            result = std::sin(std::cos(result) * 10'000.);
            result = std::pow(val, result);
            result = std::exp(result);
        }
        return static_cast<DUMMY_OUTPUT>(std::round(result)) % 100;
    }
};

struct HeavyJob : public Task
{
    static constexpr std::size_t ITERATIONS_COUNT{ LightJob::ITERATIONS_COUNT * 200ULL };
    DUMMY_OUTPUT do_stuff() override
    {
        auto result{ val };
        for (std::size_t i{ 0ULL }; i < ITERATIONS_COUNT; ++i)
        {
            result = std::sin(std::cos(result) * 10'000.);
            result = std::pow(val, result);
            result = std::exp(result);
            result = std::sqrt(result);
            result = std::pow(val, result);
            result = std::cos(std::sin(result) * 10'000);
            result = std::pow(val, result);
            result = std::exp(result);
        }
        return static_cast<DUMMY_OUTPUT>(std::round(result)) % 100;
    }
};

struct Job
{
    static constexpr auto CHANCE_OF_HEAVY_JOP_APPEARING{ 0.02 };

    static std::unique_ptr<Task> generate_task() noexcept
    {
        static std::mt19937 rng{ };
        static std::bernoulli_distribution is_heavy_job{ CHANCE_OF_HEAVY_JOP_APPEARING };

        if (!is_heavy_job(rng)) return std::make_unique<LightJob>();
        else return std::make_unique<HeavyJob>();
    }

    Job() = default;
    Job(std::unique_ptr<Task> task_init)
    :
    task{ std::move(task_init) }
    {  }

    std::unique_ptr<Task> task{ generate_task() };
};


namespace
{
    constexpr std::size_t CHUNKS_COUNT{ 100ull };
    constexpr std::size_t CHUNK_SIZE{ 10'000ull };
    constexpr std::size_t SLAVES_COUNT{ 4ull };
    static_assert(CHUNK_SIZE >= SLAVES_COUNT);
    static_assert(CHUNK_SIZE % SLAVES_COUNT == 0ull);

    using CHUNK = std::vector<Job>;
    using DUMMY_DATA = std::array<CHUNK, CHUNKS_COUNT>;

    using SLAVE_JOB = std::span<Job const>;
}


class Master
{
public:

    void job_is_done()
    {
        bool notification_needed{ false };
        {
            std::lock_guard lock{ mtx };
            ++slaves_finished_job_count;
            notification_needed = slaves_finished_job_count == SLAVES_COUNT;
        }
        if (notification_needed)
        {
            cv.notify_one();
        }
    }

    void wait_for_slaves()
    {
        cv.wait(lock, [this] { return slaves_finished_job_count == SLAVES_COUNT; });
        slaves_finished_job_count = 0ull;
    }

private:

    std::condition_variable cv{ };
    std::mutex mtx{ };
    std::unique_lock<std::mutex> lock{ mtx };

    std::size_t slaves_finished_job_count{ 0ull };
};

class Slave
{
public:

    Slave(Master& control_block_init)
        :
        control_block{ control_block_init },
        process{ &Slave::run, this }
    {
    }

    Slave(Slave&& slave_tmp) noexcept
    :
    Slave{ slave_tmp.control_block }
    { }

    ~Slave()
    {
        kill();
    }

    void set_job(SLAVE_JOB data_to_process)
    {
        std::ignore = std::lock_guard{ mtx }, data = data_to_process, dying = false;
        cv.notify_one();
    }

    void kill()
    {
        std::ignore = std::lock_guard{ mtx }, dying = true;
        cv.notify_one();
    }

    Task::DUMMY_OUTPUT get_result() const
    {
        return output;
    }

    long long get_work_time_elapsed() const
    {
        return work_time_elapsed;
    }

    std::size_t get_heavy_jobs_count() const
    {
        return heavy_jobs_count;
    }

private:

    void run()
    {
        std::unique_lock lock{ mtx };

        while (true)
        {
            cv.wait(lock, [this] { return !data.empty() || dying; });

            if (dying) break;

            heavy_jobs_count = 0ll;
            output = Task::DUMMY_OUTPUT{ 0 };
            auto const start_time_data{ std::chrono::steady_clock::now() };
            for (auto const& dummy_process : data)
            {
                output += dummy_process.task->do_stuff();
            }       
            auto const end_time_data{ std::chrono::steady_clock::now() };
            for (auto const& dummy_process : data)
            {
                if (typeid(*dummy_process.task.get()) == typeid(HeavyJob const&)) ++heavy_jobs_count;
            }
            work_time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_data - start_time_data).count();

            data = {};
            control_block.job_is_done();
        }
    }

private:

    std::condition_variable cv{ };
    std::mutex mtx{ };

    std::jthread process;

    Master& control_block;
    SLAVE_JOB data{ };
    Task::DUMMY_OUTPUT output{ };
    bool dying{ false };
    long long work_time_elapsed{ };
    std::size_t heavy_jobs_count{ };
};


static void do_singlethread(DUMMY_DATA const& data) noexcept
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
    std::cout << "Result: " << result << " | Done in " << total_time << "ms - singlethread\n";
}

struct Statistic
{
    std::array<long long, SLAVES_COUNT> timing_per_thread{ };
    std::array<std::size_t, SLAVES_COUNT> number_of_heavy_jobs_per_thread{ };
    long long total_timing{ };
};

static std::vector<Statistic> do_multithread_without_padding(DUMMY_DATA const& data) noexcept
{
    Master control_block{ };
    std::vector<Slave> slaves{ }; // remove smart ptr
    std::generate_n(std::back_inserter(slaves), SLAVES_COUNT, [&control_block] { return Slave{ control_block }; });

    std::vector<Statistic> results{ };
    results.reserve(CHUNKS_COUNT);

    Task::DUMMY_OUTPUT result{ 0ULL };
    long long total_time{ 0LL };
    for (auto const& chunk : data)
    {
        constexpr auto SLAVE_LOADOUT{ CHUNK_SIZE / SLAVES_COUNT };
        if (chunk.size() < SLAVE_LOADOUT) continue;

        auto const start_time_chunk{ std::chrono::steady_clock::now() };
        for (auto const& [j, slave] : std::views::enumerate(slaves))
        {
            slave.set_job(SLAVE_JOB{ chunk.begin() + j * SLAVE_LOADOUT, SLAVE_LOADOUT });
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
    std::cout << "Result: " << result << " | Done in " << total_time << "ms - multithread\n";

    return results;
}

static DUMMY_DATA generate_dataset_evenly()
{
    DUMMY_DATA dataset{ };
    for (auto& chunk : dataset)
    {
        std::generate_n
        (
            std::back_inserter(chunk), 
            CHUNK_SIZE, 
            [counter_max = static_cast<std::size_t>(std::round(1. / Job::CHANCE_OF_HEAVY_JOP_APPEARING)), counter = 0ULL]
            () mutable
            {
                ++counter;
                counter %= counter_max;
                if (counter == 0ULL) return Job{ std::make_unique<HeavyJob>() };
                else return Job{ std::make_unique<LightJob>() };
            }
        );
    }
    return dataset;
}

static DUMMY_DATA generate_dataset_stacked()
{
    DUMMY_DATA dataset{ generate_dataset_evenly() };
    for (auto& chunk : dataset)
    {
        std::ranges::partition(chunk, [](auto const& dummy_process){ return typeid(*dummy_process.task.get()) == typeid(HeavyJob const&); });
    }
    return dataset;
}

int main(int argc, char const* argv[])
{
    using namespace std::string_literals;
    auto const data
    { 
        [&]
        {
            if (argc > 1)
            {
                if (argv[1ULL] == "--stacked"s)
                {
                    return generate_dataset_stacked();
                }
                else if (argv[1ULL] == "--evenly"s)
                {
                    return generate_dataset_evenly();
                }
                else
                {
                    return DUMMY_DATA{ };
                }
            }
            else
            {
                return DUMMY_DATA{ };
            }
        }
        ()
    };

    //do_singlethread(data);
    std::puts("======================================");
    auto const r{ do_multithread_without_padding(data) };

    std::ofstream csv{ "timings.csv" };
    for (std::size_t i{ 0ULL }; i < SLAVES_COUNT; ++i)
    {
        csv << std::format("work_{0:};idle_{0:};heavies_{0:};", i);
    }
    csv << "total_time;total_heavies;total_idle" << std::endl;

    for (auto const& chunk : r)
    {
        long long total_idle{ 0LL };
        for (std::size_t i{ 0ULL }; i < SLAVES_COUNT; ++i)
        {
            auto const cur_idle{ chunk.total_timing - chunk.timing_per_thread[i] };
            csv << std::format("{};{};{};", chunk.timing_per_thread[i], cur_idle, chunk.number_of_heavy_jobs_per_thread[i]);
            total_idle += cur_idle;
        }
        csv << std::format("{};{};{}", chunk.total_timing, std::accumulate(chunk.number_of_heavy_jobs_per_thread.begin(), chunk.number_of_heavy_jobs_per_thread.end(), 0ULL), total_idle) << std::endl;
    }


    return EXIT_SUCCESS;
}