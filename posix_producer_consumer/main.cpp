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

    int ret;

    ret = pthread_mutex_lock(&my_mutex);
    if (ret) {
        errno = ret;
        perror("pthread_mutex_lock");
        return NULL;
    }
    ret = pthread_cond_wait(&my_cond_consumer_is_ready, &my_mutex);
    if (ret) {
        errno = ret;
        perror("pthread_cond_wait");
        return NULL;
    }

    // read numbers from stdin
    std::string input;
    getline(std::cin, input);

    std::istringstream string_stream(input);
    for (long num; string_stream >> num;) {
        p->my_num = num;
        p->empty_flag = false;

        std::cout << "p->empty_flag " << p->empty_flag << std::endl;
        while (!p->empty_flag) {
            ret = pthread_cond_signal(&my_cond_full);
            if (ret) {
                errno = ret;
                perror("pthread_cond_signal");
                return NULL;
            }
            std::cout << "waiting empty" << std::endl;
            ret = pthread_cond_wait(&my_cond_empty, &my_mutex);
            if (ret) {
                errno = ret;
                perror("pthread_cond_wait");
                return NULL;
            }
        }
        std::cout << "oops" << std::endl;
    }
    std::cout << "out" << std::endl;
    p->done_flag = true;
    // wakeup consumer threads, so they can return calculations
    ret = pthread_cond_broadcast(&my_cond_full);
    if (ret) {
        errno = ret;
        perror("pthread_cond_broadcast");
        return NULL;
    }
    ret = pthread_mutex_unlock(&my_mutex);
    if (ret) {
        errno = ret;
        perror("pthread_mutex_unlock");
        return NULL;
    }

    ret = pthread_cancel(my_pthreads_ids[2]);
    if (ret) {
        errno = ret;
        perror("pthread_cancel");
        return NULL;
    }
    return NULL;

}

void* consumer_routine(void* arg) {
    // notify about start
    // for every update issued by producer, read the value and add to sum
    // return pointer to result (for particular consumer)
    int last_state, ret;
    struct thread_params* p = (struct thread_params*)arg;
    long* local_sum = new long(0);

    ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &last_state);
    if (ret) {
        errno = ret;
        perror("pthread_setcancelstate");
        return NULL;
    }
    ret = pthread_cond_broadcast(&my_cond_consumer_is_ready);
    if (ret) {
        errno = ret;
        perror("pthread_cond_broadcast");
        return NULL;
    }

    while(!p->done_flag) {
        ret = pthread_mutex_lock(&my_mutex);
        if (ret) {
            errno = ret;
            perror("pthread_mutex_lock");
            return NULL;
        }
        while (p->empty_flag) {
            std::cout << get_tid() << " sending signal empty" << std::endl;
            ret = pthread_cond_signal(&my_cond_empty);
            if (ret) {
                errno = ret;
                perror("pthread_cond_signal");
                return NULL;
            }
            std::cout << get_tid() << " waiting full, p->empty_flag " << p->empty_flag << std::endl;
//            std::cout << my_pthreads_ids[1] << " is id of producer " << std::endl;
            if (p->done_flag) {
                ret = pthread_mutex_unlock(&my_mutex);
                if (ret) {
                    errno = ret;
                    perror("pthread_mutex_unlock");
                    return NULL;
                }
                return  local_sum;
            }
            ret = pthread_cond_wait(&my_cond_full, &my_mutex);
            if (ret) {
                errno = ret;
                perror("pthread_cond_wait");
                return NULL;
            }


//            if (p->done_flag) {
//                ret = pthread_mutex_unlock(&my_mutex);
//                if (ret) {
//                    errno = ret;
//                    perror("pthread_mutex_unlock");
//                    return NULL;
//                }
//                return  local_sum;
//            }
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
    int ret;

    while(true) {
        pthread_t random_thread_id_to_cancel = my_pthreads_ids[rand_int(3, n_threads+2)];
        pthread_testcancel();
        if (random_thread_id_to_cancel == 0) continue;
        ret = pthread_cancel(random_thread_id_to_cancel);
        if (ret) {
            errno = ret;
            perror("pthread_cancel");
            return NULL;
        }
    }
}

int run_threads() {
    // start N threads and wait until they're done
    // return aggregated sum of values
    struct thread_params p;
    int ret;

    pthread_key_create(&cur_tid, NULL);
    my_pthreads_ids = new pthread_t[3 + n_threads];

    ret = pthread_create(&my_pthreads_ids[1], NULL, producer_routine, &p);
    if (ret) {
        errno = ret;
        perror("pthread_create");
        return -1;
    }
    ret = pthread_create(&my_pthreads_ids[2], NULL, consumer_interruptor_routine, NULL);
    if (ret) {
        errno = ret;
        perror("pthread_create");
        return -1;
    }

    for (int i=0;i<n_threads;i++) {
        ret = pthread_create(&my_pthreads_ids[3 + i], NULL, consumer_routine, &p);
        if (ret) {
            errno = ret;
            perror("pthread_create");
            return -1;
        }
    }

//    long* consumers_results = new long[n_threads];
    long aggregated_sum = 0;
    for (int i=0;i<n_threads;i++) {
        // collect sums from each consumer
        void* result_ptr = new long(0);
        ret = pthread_join(my_pthreads_ids[3 + i], &result_ptr);
        if (ret) {
            errno = ret;
            perror("pthread_join");
            return -1;
        }
//        consumers_results[i] = *(long*)result_ptr;
        aggregated_sum += *(long*)result_ptr;
        delete (long*)result_ptr;
    }

//    for (int i=0;i<n_threads;i++) {
//        std::cout << consumers_results[i] << " ";
//    }
//    std::cout << std::endl;
//
//    delete[] consumers_results;

    ret = pthread_join(my_pthreads_ids[1], NULL);
    if (ret) {
        errno = ret;
        perror("pthread_join");
        return -1;
    }
    ret = pthread_join(my_pthreads_ids[2], NULL);
    if (ret) {
        errno = ret;
        perror("pthread_join");
        return -1;
    }
    delete my_pthreads_ids;

    return aggregated_sum;
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
