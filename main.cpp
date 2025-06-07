#include "config.hpp"
#include "multithreading_queue.hpp"
#include "data_generation.hpp"
#include "singlethreading.hpp"
#include "multithreading.hpp"
#include "multithreading_pool_generic.hpp"
#include "cmd_args.hpp"

#include "argparse/argparse.hpp"

int main(int argc, char const* argv[])
{
    argparse::ArgumentParser program{ };
    std::apply(
        [&program](auto const&... option) {
            (..., program.add_argument(option).default_value(false).implicit_value(true));
        },
        cmd_args::OPTIONS
    );
    program.parse_args(argc, argv);

    auto const process_dataset{
        [&](auto const& dataset, std::string const& filename_suffix){
            static std::string const BASE_FILENAME{ "timings" };
            static std::string const FILENAME_SEPARATOR{ "_" };

            using namespace std::string_literals;

            if (program[cmd_args::USE_SINGLETHREADING] == true) {
                std::clog << "Singlethreading starts...\n";
                singlethreading::do_experiment(dataset);
            }
            if (program[cmd_args::USE_MULTITHREADING] == true) {
                std::clog << "Multithreading starts...\n";
                auto const stats{ multithreading::do_experiment(dataset) };
                StatisticChunk::save_as_csv(stats, BASE_FILENAME + FILENAME_SEPARATOR + filename_suffix + FILENAME_SEPARATOR + "nq"s);
            }
            if (program[cmd_args::USE_MULTITHREADING_QUEUE] == true) {
                std::clog << "Multithreading queue starts...\n";
                auto const stats{ multithreading::queue::do_experiment(dataset) };
                StatisticChunk::save_as_csv(stats, BASE_FILENAME + FILENAME_SEPARATOR + filename_suffix + FILENAME_SEPARATOR + "q"s);
            }
        }
    };

    if (program[cmd_args::GENERATE_EVENED_DATASET] == true) {
        std::clog << "Processing evened dataset...\n";
        process_dataset(data_generation::get_evened(), "evened");
    }
    if (program[cmd_args::GENERATE_STACKED_DATASET] == true) {
        std::clog << "Processing stacked dataset...\n";
        process_dataset(data_generation::get_stacked(), "stacked");
    }
    if (program[cmd_args::USE_MULTITHREADING_POOL_GENERIC] == true) {
      std::clog << "Processing multithreading with generic pool...\n";
      multithreading::pool::generic::do_experiment();
    }
    
    return EXIT_SUCCESS;
}