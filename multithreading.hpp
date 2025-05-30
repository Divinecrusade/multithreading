#ifndef MULTITHREADING_HPP

#include "Job.hpp"
#include "StatisticChunk.hpp"

#include <mutex>
#include <cassert>

namespace config
{
    using SLAVE_JOB = std::span<Job const>;
}

namespace multithreading
{
    class Master
    {
    public:

        void job_is_done()
        {
            bool notification_needed{ false };
            {
                std::lock_guard lock{ mtx };
                ++slaves_finished_job_count;
                notification_needed = slaves_finished_job_count == config::SLAVES_COUNT;
            }
            if (notification_needed)
            {
                cv.notify_one();
            }
        }

        void wait_for_slaves()
        {
            cv.wait(lock, [this] { return slaves_finished_job_count == config::SLAVES_COUNT; });
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
        {
        }

        ~Slave()
        {
            kill();
        }

        void set_job(config::SLAVE_JOB data_to_process)
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
                    if (typeid(*dummy_process.task.get()) == typeid(HeavyTask const&)) ++heavy_jobs_count;
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
        config::SLAVE_JOB data{ };
        Task::DUMMY_OUTPUT output{ };
        bool dying{ false };
        long long work_time_elapsed{ };
        std::size_t heavy_jobs_count{ };
    };

    std::vector<StatisticChunk> do_experiment(config::DUMMY_DATA const& data);
}

#endif // !MULTITHREADING_HPP