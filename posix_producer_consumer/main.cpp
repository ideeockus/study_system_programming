#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iterator>
#include <pthread.h>
#include <unistd.h>
#include <atomic>

int get_tid();

pthread_t* my_pthreads_ids;
bool debug = false;
int n_threads;
long max_ms_consumer_sleep;
pthread_key_t cur_tid;

pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t my_cond_full = PTHREAD_COND_INITIALIZER,
                my_cond_empty = PTHREAD_COND_INITIALIZER,
                my_cond_consumer_is_ready = PTHREAD_COND_INITIALIZER;


struct thread_params {
    long my_buf;  // producer writes here, consumer reads
    bool empty_flag = false;
    bool done_flag = false;
};

void* producer_routine(void* arg) {
    // Wait for consumer to start
    // Read data, loop through each value and update the value,
    // notify consumer, wait for consumer to process

    // read numbers from stdin
//    std::vector<int> numbers;
    long* my_num = (long*)arg;

//    std::cout << "producer tid: " << get_tid() << std::endl;

    std::cout << "producer - try block mutex" << std::endl;
    pthread_mutex_lock(&my_mutex);
    std::cout << "producer - blocked mutex" << std::endl;
    std::cout << "producer - wait consumer ready" << std::endl;
    pthread_cond_wait(&my_cond_consumer_is_ready, &my_mutex);
//    pthread_mutex_unlock(&my_mutex);

    std::cout << "Enter your numbers: " << std::endl;
    std::string input;
    getline(std::cin, input);

    std::istringstream string_stream(input);
    for (long num; string_stream >> num;) {
//        while (*my_num != -1); // spinlock
//        pthread_mutex_lock(&my_mutex);
        *my_num = num;
//        pthread_cond_signal(&my_cond_full);
        std::cout << "producer - full signal sent & waiting empty" << std::endl;
//        pthread_cond_wait(&my_cond_empty, &my_mutex);
//        timespec s = {0, max_ms_consumer_sleep * 1000000};
//        while (pthread_cond_timedwait(&my_cond_empty, &my_mutex, &s) && *my_num != -1)
        while (*my_num != -1)
        {
//            std::cout << *my_num << std::endl;
            pthread_cond_signal(&my_cond_full);
            pthread_cond_wait(&my_cond_empty, &my_mutex);
        }
        std::cout << "producer - got signal empty" << std::endl;

//        pthread_mutex_unlock(&my_mutex);
    }
    std::cout << "producer - unlocking mutex" << std::endl;
    pthread_mutex_unlock(&my_mutex);

    // TODO kill interruptor thread


}

void* consumer_routine(void* arg) {
    // notify about start
    // for every update issued by producer, read the value and add to sum
    // return pointer to result (for particular consumer)
    int last_state, ret;
    long* my_num = (long*)arg;
    long* local_sum = new long;

//    std::cout << "consumer tid: " << get_tid() << std::endl;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &last_state);
    std::cout << "consumer " <<  get_tid() <<" - signal ready" << std::endl;
    pthread_cond_broadcast(&my_cond_consumer_is_ready);

    while(true) {
        std::cout << "consumer " <<  get_tid() <<" - try lock mutex" << std::endl;
        pthread_mutex_lock(&my_mutex);
        while (*my_num == -1) {
            pthread_cond_signal(&my_cond_empty);
            std::cout << "consumer " <<  get_tid() <<" - empty signal sent" << std::endl;
            std::cout << "consumer " <<  get_tid() <<" - wait cond full" << std::endl;
            pthread_cond_wait(&my_cond_full, &my_mutex);
        }
//        pthread_cond_wait(&my_cond_full, &my_mutex);
//        timespec s = {0, max_ms_consumer_sleep * 1000000};
//        while (pthread_cond_timedwait(&my_cond_full, &my_mutex, &s) && *my_num == -1);
//        while (pthread_cond_wait(&my_cond_full, &my_mutex) && *my_num == -1);
        *local_sum += *my_num;
        *my_num = -1;
        std::cout << "tid " << get_tid() << ": psum " << *local_sum << std::endl;
//        pthread_cond_signal(&my_cond_empty);
//        std::cout << "consumer - empty signal sent" << std::endl;
        std::cout << "consumer " <<  get_tid() <<" - unlocking mutex, sleeping " << 1000 * max_ms_consumer_sleep << std::endl;
        pthread_mutex_unlock(&my_mutex);
        usleep(1000 * max_ms_consumer_sleep);  // TODO add random
    }

    return local_sum;
}

void* consumer_interruptor_routine(void* arg) {
    // wait for consumers to start
    // interrupt random consumer while producer is running
    std::cout << "interruptor - try lock mutex" << std::endl;
    pthread_mutex_lock(&my_mutex);
    std::cout << "interruptor - wait consumer ready" << std::endl;
    pthread_cond_wait(&my_cond_consumer_is_ready, &my_mutex);
    pthread_mutex_unlock(&my_mutex);

    while(true) {
        //TODO get random thread id
        int random_thread_to_cancel = 0;
//        std::cout << "interruptor - cancelling " << random_thread_to_cancel << std::endl;
//        pthread_cancel(random_thread_to_cancel);
    }
}

int run_threads() {
    // start N threads and wait until they're done
    // return aggregated sum of values
    long* my_num = new long;  // shared between threads
    int ret;

    *my_num = -1;

    pthread_key_create(&cur_tid, NULL);
    my_pthreads_ids = new pthread_t[3 + n_threads];

    std::cout << "3 x tid: " << get_tid() << " - " << get_tid() << " - " << get_tid() << std::endl;

    std::cout << "running threads" << std::endl;
//    pthread_t producer_ptid, interruptor_ptid;
    int producer_created = pthread_create(&my_pthreads_ids[1], NULL, producer_routine, my_num);
    int interruptor_created = pthread_create(&my_pthreads_ids[2], NULL, consumer_interruptor_routine, NULL);
    std::cout << "producer ptid " << &my_pthreads_ids[1] << std::endl;
    std::cout << "interruptor_ptid " << &my_pthreads_ids[2] << std::endl;

    std::cout << "3 x tid: " << get_tid() << " - " << get_tid() << " - " << get_tid() << std::endl;

//    pthread_t* consumer_threads_ids = new pthread_t[n_threads];
    for (int i=0;i<n_threads;i++) {
        int c = pthread_create(&my_pthreads_ids[3 + i], NULL, consumer_routine, my_num);
    }

    std::cout << "3 x tid: " << get_tid() << " - " << get_tid() << " - " << get_tid() << std::endl;

    long* consumers_results = new long[n_threads];
    for (int i=0;i<n_threads;i++) {
        // collect sums from each consumer
        void* result_ptr = (void*)(consumers_results + i);
        pthread_join(my_pthreads_ids[3 + i], &result_ptr);
//        std::cout << *(long*)result_ptr << std::endl;
    }

    pthread_join(my_pthreads_ids[1], NULL);
    pthread_join(my_pthreads_ids[2], NULL);
//    if (ret) {
//        errno = ret;
//        perror("pthread_join");
//        return -1;
//    }

    return 0;
}

int get_tid() {
    // 1 to 3+N thread ID
//    static int thread_counter = 0;
    static std::atomic<int> thread_counter = 0;
    void* thread_id_ptr = pthread_getspecific(cur_tid);
//    std::cout << "get_tid " << thread_id_ptr << std::endl;
    if (thread_id_ptr == NULL) {
        thread_counter.fetch_add(1);
        pthread_setspecific(cur_tid, &thread_counter);
        return thread_counter;
    }

//    std::cout << "debug tid " << *(int*)thread_id_ptr << " " << (int*)thread_id_ptr << " " << thread_id_ptr << std::endl;

    return *(std::atomic<int>*)thread_id_ptr;
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
            return 1;
    }
    std::cout << "N threads: " << n_threads << std::endl;
    std::cout << "Consumer max sleep (ms): " << max_ms_consumer_sleep << std::endl;
    std::cout << "Debug mode: " << debug << std::endl;

//    for (auto i:numbers) {
//        std::cout << i << std::endl;
//    }
//    std::vector<int> my_ints = std::vector<int>{ std::istream_iterator<int>{string_stream} };

    // running threads
    std::cout << run_threads() << std::endl;
    return 0;
}
