#include "multithreading_pool_generic.hpp"

#include <iostream>

namespace multithreading::pool::generic {

void do_experiment() {
  Master task_manager{};

  task_manager.Run([] { std::cout << "He"; });
  task_manager.Run([] { std::cout << "ll"; });
  task_manager.Run([] { std::cout << "o "; });
  task_manager.Run([] { std::cout << "wo"; });
  task_manager.Run([] { std::cout << "rld"; });
}
}
