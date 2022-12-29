#include <cstdlib>
#include <cstring>
#include <cstdio>

void mysetup(void *buf, std::size_t size);
void* myalloc(std::size_t size);
void myfree(void *p);

void* mybuf;
std::size_t mysize;

struct border_marker {
    bool free;
    std::size_t size;
};

// test code
void print_mybuf_dump();
void print_border_marker(border_marker* bm);

int main()
{
    // size_t rsize = 1000;
    std::size_t size = 500 * sizeof(char);
    // printf("sizes: %d, %d", rsize, size);
    void* buf = malloc(size);
    mysetup(buf, size);

    print_mybuf_dump();

    void* a = myalloc(100);


    // char* a = (char*)myalloc(100);
    // std::cout << "a" << std::endl;
    // for (int i=0; i<=100;i++) {
        // printf("%c", *(a+i));
    // }

    // debug code
    // border_marker bm = border_marker { 1, 100 };
    // // char* border_marker_buf = (char*)&bm;

    // // void* custom_buf = malloc(size);
    // // void* custom_buf = buf;
    // std::memcpy(mybuf, &bm, sizeof(border_marker));

    // // for (std::size_t i=0; i<=sizeof(border_marker);i++) {
    // //     printf("%x", *((char*)&bm+i));
    // // }
    // // printf("\n");
    // print_border_marker(&bm);

    // for (std::size_t i=0;i<=mysize;i++) {
    //     printf("%x", *((char*)mybuf+i));
    // }
    // printf("\n");


    print_mybuf_dump();

    return 0;
}

void print_mybuf_dump() {
    printf("mybuf dump:\n");
    for (std::size_t i=0;i<=mysize;i++) {
        printf("%x", *((char*)mybuf+i));
    }
    printf("\n");
}

void print_border_marker(border_marker* bm) {
    printf("border marker:\n");
    for (std::size_t i=0; i<=sizeof(border_marker);i++) {
        printf("%x", *((char*)bm+i));
    }
    printf("\n");
}

// Попробуйте реализовать динамический аллокатор памяти

// Эта функция будет вызвана перед тем как вызывать myalloc и myfree
// используйте ее чтобы инициализировать ваш аллокатор перед началом
// работы.
//
// buf - указатель на участок логической памяти, который ваш аллокатор
//       должен распределять, все возвращаемые указатели должны быть
//       либо равны NULL, либо быть из этого участка памяти
// size - размер участка памяти, на который указывает buf

void mysetup(void *buf, std::size_t size)
{
    mybuf = buf;
    mysize = size;
}

// Функция аллокации
void* myalloc(std::size_t size)
{
    // border_marker bm = border_marker { 1, 100 };
    // std::memcpy(mybuf, &bm, sizeof(border_marker));
    struct border_marker* bm_head = (struct border_marker*)mybuf;
    bm_head->free = 1;
    bm_head->size=100;

    print_border_marker(bm);

    return mybuf;
}

// Функция освобождения
void myfree(void *p)
{}

/*
1) структура для граничного маркера
2) при аллокации создать в свободном граничный маркер, затем выделить кусок, затем снова граничный маркер
3) вернуть указатель на начало свободного куска
4) при освобождении проверить соседние граничные маркеры:
    - если сегменты свободны - слить и отметить освобожденными все вместе
    - иначе освободить только текущий
*/
