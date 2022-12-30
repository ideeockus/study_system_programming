#include <cstdlib>
// #include <cstring>
#include <cstdio>

void mysetup(void *buf, std::size_t size);
void* myalloc(std::size_t size);
void myfree(void *p);

void* mybuf;
size_t mysize;

struct border_marker {
    bool free;
    size_t size;
};

// test code
void print_mybuf_dump();
void print_border_marker(border_marker* bm);

int main()
{
    // size_t rsize = 1000;
    size_t size = 500 * sizeof(char);
    // printf("sizes: %d, %d", rsize, size);
    void* buf = malloc(size);
    mysetup(buf, size);

    print_mybuf_dump();

    void* a = myalloc(100);
    void* b = myalloc(50);
    void* c = myalloc(10);

    printf("a = %p\nb = %p\nc = %p\n", a, b, c);
    // printf("sizeof border_marker - %d", sizeof(border_marker));


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
    // printf("mybuf dump:\n");
    // for (size_t i=0;i<=mysize;i++) {
    //     printf("%x", *((char*)mybuf+i));
    // }
    // printf("\n");

    printf("mybuf dump:\n");
    for (size_t i=0;i<=mysize;i++) {
        printf("%x", *((char*)mybuf+i));
        if (i>1 && i%50 == 0) {
            printf("\t%d\n", i);
        }
    }
    printf("\n");
}

void print_border_marker(border_marker* bm) {
    printf("border marker:\n");
    for (size_t i=0; i<=sizeof(border_marker);i++) {
        printf("%x", *((char*)bm+i));
    }
    printf("\n");
}

// task code

// Попробуйте реализовать динамический аллокатор памяти

// Эта функция будет вызвана перед тем как вызывать myalloc и myfree
// используйте ее чтобы инициализировать ваш аллокатор перед началом
// работы.
//
// buf - указатель на участок логической памяти, который ваш аллокатор
//       должен распределять, все возвращаемые указатели должны быть
//       либо равны NULL, либо быть из этого участка памяти
// size - размер участка памяти, на который указывает buf

border_marker* init_seg(void* buf, size_t size) {
    // set border markers on segment edges
    size_t seg_size = size - 2*sizeof(border_marker);

    border_marker* bm_head = (border_marker*)buf;
    bm_head->free = 1;
    bm_head->size = seg_size;

    border_marker* bm_tail = (border_marker*)((unsigned char*)buf + size - sizeof(border_marker));
    bm_tail->free = 1;
    bm_tail->size = seg_size;

    return bm_head;
}

border_marker* get_tail_by_head(border_marker* bm_head) {
    unsigned char* buf_ptr = (unsigned char*)bm_head;
    border_marker* bm_tail = (border_marker*)(buf_ptr + sizeof(border_marker) + bm_head->size);
    return bm_tail;
}

border_marker* get_next_seg(border_marker* bm_head) {
    unsigned char* buf_ptr = (unsigned char*)bm_head;
    unsigned char* next_head_ptr = buf_ptr + 2*sizeof(border_marker) + bm_head->size;
    
    // check for buf bounds
    if (next_head_ptr > mybuf + mysize) {
        return NULL;
    }

    border_marker* next_head = (border_marker*)next_head_ptr;
    return next_head;
}

border_marker* get_prev_seg(border_marker* bm_head) {
    unsigned char* buf_ptr = (unsigned char*)bm_head;
    unsigned char* prev_tail_ptr = buf_ptr - sizeof(border_marker);

    // check for buf bounds
    if (prev_tail_ptr < mybuf) {
        return NULL;
    }

    // border_marker* prev_tail = (border_marker*)prev_tail_ptr;
    unsigned char* prev_head_ptr = prev_tail_ptr - ((border_marker*)prev_tail_ptr)->size - sizeof(border_marker);

    border_marker* prev_head = (border_marker*)prev_head_ptr;
    return prev_head;
}

void merge_segs(border_marker* start_bm_head, border_marker* end_bm_head) {
    border_marker* end_bm_tail = get_tail_by_head(start_bm_head);

    // size of merged segment
    // ??? recheck later
    size_t new_seg_size = (unsigned char*)end_bm_tail - (unsigned char*)start_bm_head;
    start_bm_head->size = new_seg_size;
    end_bm_tail->size = new_seg_size;
}

border_marker* try_use_seg(border_marker* bm_head, size_t use_size) {
    // try to use segment, or return null if impossible

    if (bm_head->free == false || bm_head->size < use_size) {
        return NULL;
    }

    size_t remaining_size = bm_head->size - use_size;
    border_marker* bm_tail = get_tail_by_head(bm_head);

    if (remaining_size <= 2*sizeof(border_marker)) {
        // allocate entire segment+
        bm_head->free = false;
        bm_tail->free = false;

        printf("hoba\n");

        return bm_head;
    }

    // otherwise divide segment
    unsigned char* bm_head_ptr = (unsigned char*)bm_head;
    // 2* sizeof ???
    border_marker* new_seg = init_seg(bm_head_ptr, use_size + 2*sizeof(border_marker));

    unsigned char* next_bm_head_ptr = bm_head_ptr + use_size + 2*sizeof(border_marker);
    // size_t next_seg_size = remaining_size - 2*sizeof(border_marker);
    init_seg(next_bm_head_ptr, remaining_size);

    // border_marker* next_bm_head = (border_marker*)next_bm_head_ptr;
    new_seg->free = false;
    get_tail_by_head(new_seg)->free=false;

    return new_seg;
}

border_marker* merge_nearest_segs(border_marker* bm_head) {
    // check if nearest segments is free, then merge them

    // border_marker* very_first_bm_head = (border_marker*)mybuf;

    border_marker* next_bm_head = get_next_seg(bm_head);

    if (next_bm_head->free == true) {
        merge_segs(bm_head, next_bm_head);
    }

    border_marker* prev_bm_head = get_prev_seg(bm_head);

    if (prev_bm_head->free == true) {
        merge_segs(prev_bm_head, bm_head);
        bm_head = prev_bm_head;
    }

    return bm_head;
}



void mysetup(void *buf, size_t size) {
    mybuf = buf;
    mysize = size;

    init_seg(buf, size);
}

// Функция аллокации
void* myalloc(size_t size)
{
    // border_marker bm = border_marker { 1, 100 };
    // std::memcpy(mybuf, &bm, sizeof(border_marker));
    printf("allocation %d\n", size);

    // very first bm_head
    border_marker* cur_bm_head = (border_marker*)mybuf;
    printf("cur_bm_head: {free: %d; size: %d}\n",
        cur_bm_head->free, cur_bm_head->size);
    while(cur_bm_head->free == false || cur_bm_head->size < size) {
        printf("seg %d\n in use, next", (unsigned char*)cur_bm_head);
        cur_bm_head = get_next_seg(cur_bm_head);
    }

    if (cur_bm_head == NULL) {
        return NULL;
    }

    printf("seg %p is ok\n", cur_bm_head);
    border_marker* seg_bm_head = try_use_seg(cur_bm_head, size);
    if (seg_bm_head == NULL) {
        printf("allocation fault");
    }
    print_mybuf_dump();


    // print_border_marker(bm_head);

    return seg_bm_head;
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
