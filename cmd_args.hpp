#ifndef CMD_ARGS_HPP
#define CMD_ARGS_HPP

#include <string_view>
#include <array>

namespace cmd_args
{
    static constexpr std::string_view GENERATE_STACKED_DATASET{ "--stacked" };
    static constexpr std::string_view GENERATE_EVENED_DATASET{ "--evened" };
    static constexpr std::string_view USE_SINGLETHREADING{ "--singlethreading" };
    static constexpr std::string_view USE_MULTITHREADING{ "--multithreading" };
    static constexpr std::string_view USE_MULTITHREADING_QUEUE{ "--multithreading-queue" };
    static constexpr std::string_view USE_MULTITHREADING_POOL_GENERIC{ "--multithreading-pool-generic" };

    static constexpr std::array OPTIONS{
        GENERATE_STACKED_DATASET,
        GENERATE_EVENED_DATASET,
        USE_SINGLETHREADING,
        USE_MULTITHREADING,
        USE_MULTITHREADING_QUEUE, 
        USE_MULTITHREADING_POOL_GENERIC
    };
}


#endif // !CMD_ARGS