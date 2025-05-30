#ifndef MULTITHREADING_QUEUE
#define MULTITHREADING_QUEUE

#include "Job.hpp"
#include "StatisticChunk.hpp"

#include <mutex>
#include <cassert>
#include <gsl/gsl>

namespace multithreading::queue
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
            assert(cur_task >= cur_workload.size());
        }

        void add_workload(config::CHUNK_VIEW new_workload)
        {
            cur_workload = new_workload;
            cur_task = 0;
        }

        std::optional<config::SLAVE_TASK> get_task()
        {
            auto const old_task{ cur_task++ };
            if (old_task >= cur_workload.size()) return std::nullopt;
            return cur_workload.cbegin() + old_task;
        }

    private:

        std::condition_variable cv{ };
        std::mutex mtx{ };
        std::unique_lock<std::mutex> lock{ mtx };

        config::CHUNK_VIEW cur_workload{ };
        std::atomic<gsl::index> cur_task{ };

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

        void chunk_load()
        {
            chunk_loaded = true;
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
                cv.wait(lock, [this] { return chunk_loaded || dying; });

                if (dying) break;

                heavy_jobs_count = 0ll;
                output = Task::DUMMY_OUTPUT{ 0 };
                work_time_elapsed = 0;
                for (auto cur_task{ control_block.get_task() }; cur_task.has_value(); cur_task = control_block.get_task())
                {
                    auto const start_time_data{ std::chrono::steady_clock::now() };
                    output += cur_task.value()->task->do_stuff();
                    auto const end_time_data{ std::chrono::steady_clock::now() };
                    work_time_elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(end_time_data - start_time_data).count();

                    if (typeid(*(cur_task.value()->task.get())) == typeid(HeavyTask const&)) ++heavy_jobs_count;
                }

                chunk_loaded = false;
                control_block.job_is_done();
            }
        }

    private:

        std::condition_variable cv{ };
        std::mutex mtx{ };

        std::jthread process;

        Master& control_block;
        Task::DUMMY_OUTPUT output{ };

        bool dying{ false };
        bool chunk_loaded{ false };

        long long work_time_elapsed{ };
        std::size_t heavy_jobs_count{ };
    };

    std::vector<StatisticChunk> do_experiment(config::DUMMY_DATA const& data);
}

#endif // !MULTITHREADING_QUEUE