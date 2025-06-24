#include "argparse/argparse.hpp"
#include "cmd_args.hpp"
#include "config.hpp"
#include "data_generation.hpp"
#include "experiments.hpp"
#include "multithreading.hpp"
#include "multithreading_pool_generic.hpp"
#include "multithreading_queue.hpp"

int main(int argc, char const* argv[]) {
  argparse::ArgumentParser program{};
  std::apply(
      [&program](auto const&... option) {
        (..., program.add_argument(option).default_value(false).implicit_value(
                  true));
      },
      cmd_args::OPTIONS);
  program.at(cmd_args::ASYNC_THREADS_COUNT)
    .nargs(1)
    .scan<'u', std::size_t>()
    .default_value(config::PoolParams::DEFAULT_ASYNC_THREADS_COUNT);
  program.at(cmd_args::COMPUTE_THREADS_COUNT)
    .nargs(1)
    .scan<'u', std::size_t>()
    .default_value(config::PoolParams::DEFAULT_COMPUTE_THREADS_COUNT);
  program.at(cmd_args::DATASET_SIZE)
    .nargs(1)
    .scan<'u', std::size_t>()
    .default_value(config::PoolParams::DEFAULT_DATASET_SIZE);
  program.at(cmd_args::HEAVY_TASKS_COUNT)
    .nargs(1)
    .scan<'u', std::size_t>()
    .default_value(config::PoolParams::DEFAULT_HEAVY_TASKS_COUNT);
  program.parse_args(argc, argv);

  auto const process_dataset{[&](auto dataset,
                                 std::string const& filename_suffix) {
    static std::string const BASE_FILENAME{"timings"};
    static std::string const FILENAME_SEPARATOR{"_"};

    using namespace std::string_literals;

    if (program[cmd_args::USE_SINGLETHREADING] == true) {
      std::clog << "Singlethreading starts...\n";
      experiments::singlethread::process_data(dataset);
    }
    if (program[cmd_args::USE_MULTITHREADING] == true) {
      std::clog << "Multithreading starts...\n";
      auto const stats{
          experiments::multithread::process_data_without_queue(dataset)};
      StatisticChunk::save_as_csv(stats, BASE_FILENAME + FILENAME_SEPARATOR +
                                             filename_suffix +
                                             FILENAME_SEPARATOR + "nq"s);
    }
    if (program[cmd_args::USE_MULTITHREADING_QUEUE] == true) {
      std::clog << "Multithreading queue starts...\n";
      auto const stats{
          experiments::multithread::process_data_with_queue(dataset)};
      StatisticChunk::save_as_csv(stats, BASE_FILENAME + FILENAME_SEPARATOR +
                                             filename_suffix +
                                             FILENAME_SEPARATOR + "q"s);
    }
    if (program[cmd_args::USE_MULTITHREADING_POOL] == true) {
      std::clog << "Multithreading pool starts...\n";
      experiments::multithread::process_data_with_pool(dataset);
    }
  }};

  if (program[cmd_args::GENERATE_EVENED_DATASET] == true) {
    std::clog << "Processing evened dataset...\n";
    process_dataset(data_generation::get_evened(), "evened");
  }
  if (program[cmd_args::GENERATE_STACKED_DATASET] == true) {
    std::clog << "Processing stacked dataset...\n";
    process_dataset(data_generation::get_stacked(), "stacked");
  }
  if (program[cmd_args::USE_MULTITHREADING_DYNAMIC] == true) {
    std::clog << "Processing data dynamic configured...\n";
    experiments::multithread::process_data_with_pool_dynamic(
        data_generation::get_dynamic(
            program.get<std::size_t>(cmd_args::DATASET_SIZE),
            program.get<std::size_t>(cmd_args::HEAVY_TASKS_COUNT)),
        program.get<std::size_t>(cmd_args::ASYNC_THREADS_COUNT),
        program.get<std::size_t>(cmd_args::COMPUTE_THREADS_COUNT));
  }
  if (program[cmd_args::USE_MULTITHREADING_STEALING] == true) {
    std::clog << "Processing data dynamic configured...\n";
    experiments::multithread::process_data_with_pool_stealing(
      data_generation::get_dynamic(
        program.get<std::size_t>(cmd_args::DATASET_SIZE),
        program.get<std::size_t>(cmd_args::HEAVY_TASKS_COUNT)));
  }

  return EXIT_SUCCESS;
}