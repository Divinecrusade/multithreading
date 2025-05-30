#include "config.hpp"
#include "multithreading_queue.hpp"
#include "data_generation.hpp"
#include "singlethread.hpp"

int main(int argc, char const* argv[])
{
    using namespace std::string_literals;
    auto const dataset
    { 
        [&]
        {
            if (argc > 1)
            {
                if (argv[1ULL] == "--stacked"s)
                {
                    return data_generation::get_stacked();
                }
                else if (argv[1ULL] == "--evenly"s)
                {
                    return data_generation::get_evenly();
                }
                else
                {
                    return config::DUMMY_DATA{ };
                }
            }
            else
            {
                return config::DUMMY_DATA{ };
            }
        }
        ()
    };

    singlethread::do_experiment(dataset);
    std::puts("======================================");
    auto const result {
        multithreading::queue::do_experiment(dataset)
    };

    StatisticChunk::save_as_csv(
        result, 
        "timings.csv"
    );

    return EXIT_SUCCESS;
}