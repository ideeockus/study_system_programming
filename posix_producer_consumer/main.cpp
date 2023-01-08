#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iterator>
#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <random>

int get_tid();

pthread_t* my_pthreads_ids;
bool debug = false;
int n_threads;
int max_ms_consumer_sleep;
pthread_key_t cur_tid;

pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t my_cond_full = PTHREAD_COND_INITIALIZER,
                my_cond_empty = PTHREAD_COND_INITIALIZER,
                my_cond_consumer_is_ready = PTHREAD_COND_INITIALIZER;


struct thread_params {
    long my_num = 0;  // producer writes here, consumer reads
    bool empty_flag = true;
    bool done_flag = false;
};

int rand_int(int low, int high)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(low, high);

    return dis(gen);
}

void* producer_routine(void* arg) {
    // Wait for consumer to start
    // Read data, loop through each value and update the value,
    // notify consumer, wait for consumer to process

    struct thread_params* p = (struct thread_params*)arg;

    pthread_mutex_lock(&my_mutex);
    pthread_cond_wait(&my_cond_consumer_is_ready, &my_mutex);

    // read numbers from stdin
    std::string input;
    getline(std::cin, input);

    std::istringstream string_stream(input);
    for (long num; string_stream >> num;) {
        p->my_num = num;
        p->empty_flag = false;

        while (!p->empty_flag) {
            pthread_cond_signal(&my_cond_full);
            pthread_cond_wait(&my_cond_empty, &my_mutex);
        }
    }
    p->done_flag = true;
    // wakeup consumer threads, so they can return calculations
    pthread_cond_broadcast(&my_cond_full);
    pthread_mutex_unlock(&my_mutex);

    pthread_cancel(my_pthreads_ids[2]);
    return NULL;


}

void* consumer_routine(void* arg) {
    // notify about start
    // for every update issued by producer, read the value and add to sum
    // return pointer to result (for particular consumer)
    int last_state, ret;
    struct thread_params* p = (struct thread_params*)arg;
    long* local_sum = new long;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &last_state);
    pthread_cond_broadcast(&my_cond_consumer_is_ready);

    while(!p->done_flag) {
        pthread_mutex_lock(&my_mutex);
        while (p->empty_flag) {
            pthread_cond_signal(&my_cond_empty);
            pthread_cond_wait(&my_cond_full, &my_mutex);


            if (p->done_flag) {
                pthread_mutex_unlock(&my_mutex);
                return  local_sum;
            }
        }

        *local_sum += p->my_num;
        p->empty_flag = true;
        if (debug) std::cout << "tid " << get_tid() << ": psum " << *local_sum << std::endl;
        int sleep_sec = rand_int(0, max_ms_consumer_sleep);
        pthread_mutex_unlock(&my_mutex);
        usleep(1000 * sleep_sec);
    }

    return local_sum;
}

void* consumer_interruptor_routine(void* arg) {
    // wait for consumers to start
    // interrupt random consumer while producer is running
    pthread_mutex_lock(&my_mutex);
    pthread_cond_wait(&my_cond_consumer_is_ready, &my_mutex);
    pthread_mutex_unlock(&my_mutex);

    while(true) {
        pthread_t random_thread_id_to_cancel = my_pthreads_ids[rand_int(2, n_threads+2)];
        pthread_cancel(random_thread_id_to_cancel);
        pthread_testcancel();
    }
}

int run_threads() {
    // start N threads and wait until they're done
    // return aggregated sum of values
    struct thread_params p;

    pthread_key_create(&cur_tid, NULL);
    my_pthreads_ids = new pthread_t[3 + n_threads];

    int producer_created = pthread_create(&my_pthreads_ids[1], NULL, producer_routine, &p);
    int interruptor_created = pthread_create(&my_pthreads_ids[2], NULL, consumer_interruptor_routine, NULL);

    for (int i=0;i<n_threads;i++) {
        int c = pthread_create(&my_pthreads_ids[3 + i], NULL, consumer_routine, &p);
    }

    long* consumers_results = new long[n_threads];
    for (int i=0;i<n_threads;i++) {
        // collect sums from each consumer
        void* result_ptr = new long;
        pthread_join(my_pthreads_ids[3 + i], &result_ptr);
        consumers_results[i] = *(long*)result_ptr;
    }

    // print results
    for (int i=0;i<n_threads;i++) {
        std::cout << consumers_results[i] << " ";
    }
    std::cout << std::endl;

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
    static std::atomic<int> thread_counter = 1;
    void* thread_id_ptr = pthread_getspecific(cur_tid);
    if (thread_id_ptr == NULL) {
        int* new_pid = new int(thread_counter.fetch_add(1));
        pthread_setspecific(cur_tid, new_pid);
        return *new_pid;
    }

    return *(std::atomic<int>*)thread_id_ptr;
}

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
            std::cout << "usage: <./my_prog> [--debug] N_consumer_threads consumer_max_ms" << std::endl;
            return 1;
    }

    // running threads
    std::cout << run_threads() << std::endl;
    return 0;
}
