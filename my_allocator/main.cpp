#include <cstdlib>
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

border_marker* get_next_seg(border_marker* bm_head);
border_marker* get_prev_seg(border_marker* bm_head);
border_marker* get_tail_by_head(border_marker* bm_head);
border_marker* init_seg(void* buf, size_t size);
border_marker* try_use_seg(border_marker* bm_head, size_t use_size);
border_marker* merge_nearest_segs(border_marker* bm_head);
void merge_segs(border_marker* start_bm_head, border_marker* end_bm_head);

// test code
void print_mybuf_dump();
void print_mybuf_dump_colored();
void print_border_marker(border_marker* bm);

int main()
{
    size_t size = 500 * sizeof(char);
    void* buf = malloc(size);
    mysetup(buf, size);

    print_mybuf_dump_colored();

    void* a = myalloc(10);
    print_mybuf_dump_colored();

    void* b = myalloc(200);
    print_mybuf_dump_colored();
    // void* c = myalloc(200);
    // void* d = myalloc(5);

    // printf("a = %p\nb = %p\nc = %p\n", a, b, c);
    // printf("d = %p\n", d);
    // print_mybuf_dump_colored();


    myfree(a);
    // myfree(c);
    myfree(b);

    print_mybuf_dump_colored();

    return 0;
}

void print_mybuf_dump() {
    // print buf dump without color
    printf("mybuf dump:\n");
    for (size_t i=0;i<=mysize;i++) {
        printf("%x", *((unsigned char*)mybuf+i));
        if (i>1 && i%50 == 0) {
            printf("\t%d", i);
            printf("\t%p\n", (unsigned char*)mybuf+i);
        }
    }
    printf("\n");
}

void print_mybuf_dump_colored() {
    // print buf dump with colors for border markers
    void* head_start_pos = mybuf;
    void* head_end_pos = head_start_pos + sizeof(border_marker);
    void* tail_start_pos = get_tail_by_head((border_marker*)head_start_pos);
    void* tail_end_pos = tail_start_pos + sizeof(border_marker);

    printf("\033[1mmybuf dump:\033[0m\n");
    for (size_t i=0;i<=mysize;i++) {
        void* ptr = mybuf + i;

        // add ANSI color seqs to output
        if (ptr == head_start_pos) {
            printf("\033[31m");
            tail_start_pos = get_tail_by_head((border_marker*)head_start_pos);
            tail_end_pos = tail_start_pos + sizeof(border_marker);
        } else if (ptr == tail_start_pos) {
            printf("\033[34m");
        } else if (ptr == head_end_pos || ptr == tail_end_pos) {
            printf("\033[0m");

            head_start_pos = (void*)get_next_seg((border_marker*)head_start_pos);
            if (head_start_pos == NULL) {
                head_start_pos = mybuf;
            }
            head_end_pos = head_start_pos + sizeof(border_marker);
        }
        printf("%x", *((unsigned char*)ptr));
        if (i>1 && i%50 == 0) {
            printf("\t%d", i);
            printf("\t%p\n", ptr);
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
// задача - реализовать динамический аллокатор памяти

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
    if (next_head_ptr + sizeof(border_marker) > mybuf + mysize) {
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
    printf("merging segments: %p  and %p\n", start_bm_head, end_bm_head);

    border_marker* end_bm_tail = get_tail_by_head(end_bm_head);

    // size of merged segment
    size_t new_seg_size = start_bm_head->size + end_bm_head->size + 2*sizeof(border_marker);
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

        return bm_head;
    }

    // otherwise divide segment
    unsigned char* bm_head_ptr = (unsigned char*)bm_head;
    border_marker* new_seg = init_seg(bm_head_ptr, use_size + 2*sizeof(border_marker));

    unsigned char* next_bm_head_ptr = bm_head_ptr + use_size + 2*sizeof(border_marker);
    init_seg(next_bm_head_ptr, remaining_size);

    new_seg->free = false;
    get_tail_by_head(new_seg)->free=false;

    return new_seg;
}

border_marker* merge_nearest_segs(border_marker* bm_head) {
    // check if nearest segments is free, then merge them
    border_marker* next_bm_head = get_next_seg(bm_head);

    if (next_bm_head!=NULL && next_bm_head->free == true) {
        merge_segs(bm_head, next_bm_head);
    }

    border_marker* prev_bm_head = get_prev_seg(bm_head);

    if (prev_bm_head!=NULL && prev_bm_head->free == true) {
        merge_segs(prev_bm_head, bm_head);
        bm_head = prev_bm_head;
    }

    return bm_head;
}


// buf - указатель на участок логической памяти, который ваш аллокатор
//       должен распределять, все возвращаемые указатели должны быть
//       либо равны NULL, либо быть из этого участка памяти
// size - размер участка памяти, на который указывает buf
void mysetup(void *buf, size_t size) {
    mybuf = buf;
    mysize = size;

    init_seg(buf, size);
}

// Функция аллокации
void* myalloc(size_t size) {
    printf("allocation %d\n", size);

    // very first bm_head
    border_marker* cur_bm_head = (border_marker*)mybuf;
    while(cur_bm_head != NULL && (cur_bm_head->free == false || cur_bm_head->size < size)) {
        cur_bm_head = get_next_seg(cur_bm_head);
    }

    if (cur_bm_head == NULL) {
        return NULL;
    }

    border_marker* seg_bm_head = try_use_seg(cur_bm_head, size);
    if (seg_bm_head == NULL) {
        printf("allocation fault");
        return NULL;
    }

    void* seg_ptr = (unsigned char*)seg_bm_head + sizeof(border_marker);

    return seg_ptr;
}

// Функция освобождения
void myfree(void* p) {
    unsigned char* bm_head_ptr = (unsigned char*)p - sizeof(border_marker);
    printf("free %p\n", bm_head_ptr);

    border_marker* bm_head = merge_nearest_segs((border_marker*)bm_head_ptr);
    border_marker* bm_tail = get_tail_by_head(bm_head);

    bm_head->free = true;
    bm_tail->free = true;
}
