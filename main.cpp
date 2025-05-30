#include "config.hpp"
#include "multithreading_queue.hpp"
#include "data_generation.hpp"
#include "singlethreading.hpp"
#include "multithreading.hpp"

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

    singlethreading::do_experiment(dataset);
    std::puts("======================================");
    auto result {
        multithreading::queue::do_experiment(dataset)
    };

    //StatisticChunk::save_as_csv(
    //    result, 
    //    "timings.csv"
    //);

    result = multithreading::do_experiment(dataset);

    return EXIT_SUCCESS;
}