#include <queue>
#include <cstdio>

std::queue <int> myq;
int current_thread_id;
int current_thread_worked_ticks;
int max_thread_work_ticks;

void run_next_thread() {
    if (!myq.empty()) {
        current_thread_id = myq.front();
        printf("run thread %d\n", current_thread_id);
        myq.pop();
        current_thread_worked_ticks = 0;
    } else {
        current_thread_id = -1;
    }
    
}

/**
 * Функция будет вызвана перед каждым тестом, если вы
 * используете глобальные и/или статические переменные
 * не полагайтесь на то, что они заполнены 0 - в них
 * могут храниться значения оставшиеся от предыдущих
 * тестов.
 *
 * timeslice - квант времени, который нужно использовать.
 * Поток смещается с CPU, если пока он занимал CPU функция
 * timer_tick была вызвана timeslice раз.
 **/
void scheduler_setup(int timeslice)
{
    current_thread_id = -1;
    current_thread_worked_ticks = 0;
    max_thread_work_ticks = timeslice;
}

/**
 * Функция вызывается, когда создается новый поток управления.
 * thread_id - идентификатор этого потока и гарантируется, что
 * никакие два потока не могут иметь одинаковый идентификатор.
 **/
void new_thread(int thread_id)
{
    printf("new thread %d\n", thread_id);
    myq.push(thread_id);

    if (current_thread_id == -1) {
        run_next_thread();
    }
}

/**
 * Функция вызывается, когда поток, исполняющийся на CPU,
 * завершается. Завершится может только поток, который сейчас
 * исполняется, поэтому thread_id не передается. CPU должен
 * быть отдан другому потоку, если есть активный
 * (незаблокированный и незавершившийся) поток.
 **/
void exit_thread()
{
    printf("exit thread %d\n", current_thread_id);
    run_next_thread();
}

/**
 * Функция вызывается, когда поток, исполняющийся на CPU,
 * блокируется. Заблокироваться может только поток, который
 * сейчас исполняется, поэтому thread_id не передается. CPU
 * должен быть отдан другому активному потоку, если таковой
 * имеется.
 **/
void block_thread()
{
    printf("block thread %d\n", current_thread_id);
    run_next_thread();
}

/**
 * Функция вызывается, когда один из заблокированных потоков
 * разблокируется. Гарантируется, что thread_id - идентификатор
 * ранее заблокированного потока.
 **/
void wake_thread(int thread_id)
{
    printf("wake thread %d\n", thread_id);
    new_thread(thread_id);
    // myq.push(thread_id);
    // if (current_thread_id == -1) {
    //     run_next_thread();
    // }
}

/**
 * Ваш таймер. Вызывается каждый раз, когда проходит единица
 * времени.
 **/
void timer_tick()
{
    current_thread_worked_ticks++;
    printf("current_thread_worked_ticks %d\n", current_thread_worked_ticks);

    if (current_thread_worked_ticks >= max_thread_work_ticks) {
        myq.push(current_thread_id);
        run_next_thread();
    }
}

/**
 * Функция должна возвращать идентификатор потока, который в
 * данный момент занимает CPU, или -1 если такого потока нет.
 * Единственная ситуация, когда функция может вернуть -1, это
 * когда нет ни одного активного потока (все созданные потоки
 * либо уже завершены, либо заблокированы).
 **/
int current_thread()
{
    return current_thread_id;
}


// test code
void tick(int ticks) {
    for (int i=0;i<ticks;i++) {
        timer_tick();
    }
}

int main() {

    scheduler_setup(10);

    printf("current_thread: %d\n", current_thread());
    new_thread(1);
    new_thread(2);
    new_thread(100);

    tick(5);
    tick(8);
    block_thread();
    tick(20);
    wake_thread(2);
    tick(20);
    exit_thread();
    tick(20);
    printf("current_thread: %d\n", current_thread());

    exit_thread();
    exit_thread();
    printf("current_thread: %d\n", current_thread());



    return 0;
}