#ifndef CMD_ARGS_HPP
#define CMD_ARGS_HPP

#include <array>
#include <string_view>

namespace cmd_args {
using namespace std::string_view_literals;
static constexpr auto GENERATE_STACKED_DATASET{"--stacked"sv};
static constexpr auto GENERATE_EVENED_DATASET{"--evened"sv};

static constexpr auto USE_SINGLETHREADING{"--singlethreading"sv};
static constexpr auto USE_MULTITHREADING{"--multithreading"sv};
static constexpr auto USE_MULTITHREADING_QUEUE{"--multithreading-queue"sv};
static constexpr auto USE_MULTITHREADING_POOL{"--multithreading-pool"sv};

static constexpr auto USE_MULTITHREADING_DYNAMIC{"--multithreading-dynamic"sv};
static constexpr auto ASYNC_THREADS_COUNT{"--async-threads-count"sv};
static constexpr auto COMPUTE_THREADS_COUNT{"--compute-threads-count"sv};
static constexpr auto DATASET_SIZE{"--dataset-size"sv};
static constexpr auto HEAVY_TASKS_COUNT{"--heavy-tasks-count"sv};

static constexpr std::array OPTIONS{GENERATE_STACKED_DATASET,
                                    GENERATE_EVENED_DATASET,
                                    USE_SINGLETHREADING,
                                    USE_MULTITHREADING,
                                    USE_MULTITHREADING_QUEUE,
                                    USE_MULTITHREADING_POOL,
                                    USE_MULTITHREADING_DYNAMIC,
                                    ASYNC_THREADS_COUNT,
                                    COMPUTE_THREADS_COUNT,
                                    DATASET_SIZE,
                                    HEAVY_TASKS_COUNT};
}  // namespace cmd_args

#endif  // !CMD_ARGS