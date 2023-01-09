#include <cstring>
#include <iostream>
#include "producer_consumer.h"

bool debug = false;
int n_threads;
int max_ms_consumer_sleep;

void parse_command_args(char** args) {
  n_threads = atoi(args[0]);
  max_ms_consumer_sleep = atoi(args[1]);
}

int main(int argc, char** argv) {
  // parse arguments
  switch (argc) {
    case 3:
      parse_command_args(argv + 1);
      break;
    case 4:
      if (strcmp(argv[1], "--debug") == 0) debug = true;
      parse_command_args(argv + 2);
      break;
    default:
      std::cout
          << "usage: <./my_prog> [--debug] N_consumer_threads consumer_max_ms"
          << std::endl;
      return 1;
  }

  // running threads
  std::cout << run_threads(debug, n_threads, max_ms_consumer_sleep)
            << std::endl;
  return 0;
}
