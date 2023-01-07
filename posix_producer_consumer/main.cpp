#include <iostream>
#include <cstdlib>
#include <cstring>
#include <pthread.h>


pthread_t* my_pthreads_ids;
bool debug = false;
int n_threads;
long max_ms_consumer_sleep;

void* producer_routine(void* arg) {
    // Wait for consumer to start
    // Read data, loop through each value and update the value,
    // notify consumer, wait for consumer to process
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

    for (int i=0;i<n_threads;i++) {
        pthread_t ptid;
        int a = pthread_create(&ptid, NULL, consumer_routine, NULL);
    }

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
    std::cout << "N param: " << n_threads << "\n" << "Consumer max sleep: " << max_ms_consumer_sleep << std::endl;
    std::cout << run_threads() << std::endl;
    return 0;
}
