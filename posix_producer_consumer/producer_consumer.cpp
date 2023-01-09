#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>

pthread_t* my_pthreads_ids;
pthread_key_t cur_tid;

pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t my_cond_full = PTHREAD_COND_INITIALIZER,
        my_cond_empty = PTHREAD_COND_INITIALIZER,
        my_cond_consumers_is_ready = PTHREAD_COND_INITIALIZER;

struct thread_params {
    long my_num = 0;  // producer writes here, consumer reads
    bool empty_flag = true;
    bool done_flag = false;
    bool consumers_are_running = false;
    bool debug = false;
    int n_threads;
    int max_ms_consumer_sleep;
};

int rand_int(int low, int high) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(low, high);

    return dis(gen);
}

int get_tid() {
    // 1 to 3+N thread ID
    static std::atomic<int> thread_counter = {1};
    void* tid = pthread_getspecific(cur_tid);
    if (tid == NULL) {
        int* new_tid = new int(thread_counter.fetch_add(1));
        pthread_setspecific(cur_tid, new_tid);
        return *new_tid;
    }

    return *(std::atomic<int>*)tid;
}

void tid_destructor(void* ptr) {
    int* tid = (int*)ptr;
    delete tid;
}

void* producer_routine(void* arg) {
    // Wait for consumer to start
    // Read data, loop through each value and update the value,
    // notify consumer, wait for consumer to process

    struct thread_params* p = (struct thread_params*)arg;

    pthread_mutex_lock(&my_mutex);
    if (!p->consumers_are_running) pthread_cond_wait(&my_cond_consumers_is_ready, &my_mutex);

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
    int last_state;
    struct thread_params* p = (struct thread_params*)arg;
    long* local_sum = new long(0);

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &last_state);
    //  pthread_cond_broadcast(&my_cond_consumer_is_ready);

    while (!p->done_flag) {
        pthread_mutex_lock(&my_mutex);
        while (p->empty_flag) {
            pthread_cond_signal(&my_cond_empty);
            if (p->done_flag) {
                pthread_mutex_unlock(&my_mutex);
                return local_sum;
            }
            pthread_cond_wait(&my_cond_full, &my_mutex);

            // if (p->done_flag) {
            //   pthread_mutex_unlock(&my_mutex);
            //   return local_sum;
            // }
        }

        *local_sum += p->my_num;
        p->empty_flag = true;
        if (p->debug)
            std::cout << "tid " << get_tid() << ": psum " << *local_sum << std::endl;
        int sleep_sec = rand_int(0, p->max_ms_consumer_sleep);
        pthread_mutex_unlock(&my_mutex);
        usleep(1000 * sleep_sec);
    }

    return local_sum;
}

void* consumer_interruptor_routine(void* arg) {
    // wait for consumers to start
    // interrupt random consumer while producer is running
    struct thread_params* p = (struct thread_params*)arg;

    pthread_mutex_lock(&my_mutex);
    if (!p->consumers_are_running) pthread_cond_wait(&my_cond_consumers_is_ready, &my_mutex);
    pthread_mutex_unlock(&my_mutex);

    while (true) {
        int rand = rand_int(2, p->n_threads + 2);
        //      std::cout << "cc " << rand << std::endl;
        pthread_t random_thread_id_to_cancel = my_pthreads_ids[rand];
        pthread_testcancel();
        // if (p->done_flag) return NULL;
        //    std::cout << "bb " << random_thread_id_to_cancel << std::endl;
        if (random_thread_id_to_cancel == 0) continue;
        //    std::cout << "aa " << random_thread_id_to_cancel << std::endl;
        pthread_cancel(random_thread_id_to_cancel);
    }
}

// the declaration of run threads can be changed as you like
int run_threads(bool debug, int n_threads, int max_ms_consumer_sleep) {
    // start N threads and wait until they're done
    // return aggregated sum of values
    struct thread_params p = {0,     true,      false, false,
                              debug, n_threads, max_ms_consumer_sleep};

    pthread_key_create(&cur_tid, tid_destructor);
    my_pthreads_ids = new pthread_t[3 + n_threads];

    pthread_create(&my_pthreads_ids[1], NULL, producer_routine, &p);
    pthread_create(&my_pthreads_ids[2], NULL, consumer_interruptor_routine, &p);

    for (int i = 0; i < n_threads; i++) {
        pthread_create(&my_pthreads_ids[3 + i], NULL, consumer_routine, &p);
    }

    // signal that consumers started
    pthread_mutex_lock(&my_mutex);
    p.consumers_are_running = true;
    pthread_mutex_unlock(&my_mutex);
    pthread_cond_broadcast(&my_cond_consumers_is_ready);

    // long* consumers_results = new long[n_threads];
    long aggregated_sum = 0;
    for (int i = 0; i < n_threads; i++) {
        // collect sums from each consumer
        void* result_ptr;
        pthread_join(my_pthreads_ids[3 + i], &result_ptr);
        // consumers_results[i] = *(long*)result_ptr;
        aggregated_sum += *(long*)result_ptr;
        delete (long*)result_ptr;
    }

    // print results
    // for (int i = 0; i < n_threads; i++) {
    //   std::cout << consumers_results[i] << " ";
    // }
    // std::cout << std::endl;

    // long aggregated_sum = 0;
    // for (int i = 0; i < n_threads; i++) {
    //   aggregated_sum += consumers_results[i];
    // }

    pthread_join(my_pthreads_ids[1], NULL);
    pthread_join(my_pthreads_ids[2], NULL);
    //    if (ret) {
    //        errno = ret;
    //        perror("pthread_join");
    //        return -1;
    //    }
    // delete[] consumers_results;
    delete[] my_pthreads_ids;

    return aggregated_sum;
}
