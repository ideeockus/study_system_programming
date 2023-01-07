#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>
#include <iterator>
#include <queue>
#include <pthread.h>


pthread_t* my_pthreads_ids;
bool debug = false;
int n_threads;
long max_ms_consumer_sleep;

void* producer_routine(void* arg) {
    // Wait for consumer to start
    // Read data, loop through each value and update the value,
    // notify consumer, wait for consumer to process
    char* x = new char;

    std::cin >> *x;

    std::cout << x << std::endl;
}

void* consumer_routine(void* arg) {
    // notify about start
    // for every update issued by producer, read the value and add to sum
    // return pointer to result (for particular consumer)
}

void* consumer_interruptor_routine(void* arg) {
    // wait for consumers to start
    // interrupt random consumer while producer is running
}

int run_threads() {
    // start N threads and wait until they're done
    // return aggregated sum of values

    std::cout << "running producer thread" << std::endl;
    pthread_t producer_ptid;
    int b = pthread_create(&producer_ptid, NULL, producer_routine, NULL);
    std::cout << "producer ptid " << producer_ptid << std::endl;


    for (int i=0;i<n_threads;i++) {
        pthread_t ptid;
        int a = pthread_create(&ptid, NULL, consumer_routine, NULL);
    }

    pthread_join(producer_ptid, NULL);

    return 0;
}

int get_tid() {
    // 1 to 3+N thread ID
    return 0;
}

void parse_command_args(char** args) {
    n_threads = atoi(args[0]);
    max_ms_consumer_sleep = atoi(args[1]);
}

int main(int argc, char** argv) {
    // just check argv
//    for (int i=0;i<argc;i++) {
//        std::cout << *(argv + i) << std::endl;
//    }
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
            std::cout << "usage: <./my_prog> [--debug] N_consumer_threads consumer_max_ms" << std::endl;
            break;
    }
    std::cout << "N threads: " << n_threads << std::endl;
    std::cout << "Consumer max sleep (ms): " << max_ms_consumer_sleep << std::endl;
    std::cout << "Debug mode: " << debug << std::endl;

    // read numbers from stdin
    std::vector<int> numbers;

    std::string input;
    getline(std::cin, input);
    std::istringstream string_stream(input);
    for (int num; string_stream >> num;) {
        numbers.push_back(num);
    }
//    for (auto i:numbers) {
//        std::cout << i << std::endl;
//    }
//    std::vector<int> my_ints = std::vector<int>{ std::istream_iterator<int>{string_stream} };

    // running threads
    std::cout << run_threads() << std::endl;
    return 0;
}
