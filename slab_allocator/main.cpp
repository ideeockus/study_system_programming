#include <cstdlib>
#include <cstdio>
#include <cstdint>

/**
 * Эти две функции вы должны использовать для аллокации
 * и освобождения памяти в этом задании. Считайте, что
 * внутри они используют buddy аллокатор с размером
 * страницы равным 4096 байтам.
 **/

/**
 * Аллоцирует участок размером 4096 * 2^order байт,
 * выровненный на границу 4096 * 2^order байт. order
 * должен быть в интервале [0; 10] (обе границы
 * включительно), т. е. вы не можете аллоцировать больше
 * 4Mb за раз.
 **/
void *alloc_slab(int order);
/**
 * Освобождает участок ранее аллоцированный с помощью
 * функции alloc_slab.
 **/
void free_slab(void *slab);

struct slab_object {
    slab_object* next;
};

struct slab_header {
    unsigned int free_slabs;
    slab_header* next;
    slab_header* prev;
    slab_object* free_block;
};


/**
 * Эта структура представляет аллокатор, вы можете менять
 * ее как вам удобно. Приведенные в ней поля и комментарии
 * просто дают общую идею того, что вам может понадобится
 * сохранить в этой структуре.
 **/
struct cache {
    slab_header totally_free; /* список пустых SLAB-ов для поддержки cache_shrink */
    slab_header partially_busy; /* список частично занятых SLAB-ов */
    slab_header totally_busy; /* список заполненых SLAB-ов */

    size_t object_size; /* размер аллоцируемого объекта */
    int slab_order; /* используемый размер SLAB-а */
    size_t slab_objects; /* количество объектов в одном SLAB-е */ 
};


/**
 * Функция инициализации будет вызвана перед тем, как
 * использовать это кеширующий аллокатор для аллокации.
 * Параметры:
 *  - cache - структура, которую вы должны инициализировать
 *  - object_size - размер объектов, которые должен
 *    аллоцировать этот кеширующий аллокатор 
 **/
void cache_setup(struct cache *cache, size_t object_size) {
    int min_slab_objects = 2 << 7;

    // служебные хедеры, для упрощения работы со списком
    cache->totally_free = {0, NULL, NULL, NULL};
    cache->partially_busy = {0, NULL, NULL, NULL};
    cache->totally_busy = {0, NULL, NULL, NULL};

    // предварительный размер SLAB'а
    size_t slab_estimated_mem = (sizeof(slab_object) + object_size) * min_slab_objects + sizeof(slab_header);

    // нужно найти slab_order
    int i = 0;
    while(i <= 10 && 4096 * (1 << i) <= slab_estimated_mem) {
        i++;
    }

    // размер должен быть до 4Mib
    size_t slab_mem = 4096 * 1 << i;
    size_t slab_objects = (slab_mem - sizeof(slab_header)) / (object_size + sizeof(slab_object));

    cache->object_size = object_size;
    cache->slab_order = i;
    cache->slab_objects = slab_objects;
}


void release_slab_list(slab_header* slab_list) {
    printf("releasing list %p\n", slab_list);
    slab_header* slab = slab_list;
    if(slab && slab->prev) slab->prev->next = NULL;
    while(slab) {
        slab_header* slab_to_release = slab;
        slab = slab->next; 
        free(slab_to_release);
    }
}

void move_slab(slab_header* slab, slab_header* list) {
    printf("moving slab %p to %p\n", slab, list);
    // обновить данные соседей
    if (slab->prev) slab->prev->next = slab->next;
    if (slab->next) slab->next->prev = slab->prev;

    // обновить данные самого SLAB'а
    slab->next = list->next;
    slab->prev = list;

    // обновить данные новых соседей
    if(list->next) list->next->prev = slab;
    list->next = slab;
}

/**
 * Функция освобождения будет вызвана когда работа с
 * аллокатором будет закончена. Она должна освободить
 * всю память занятую аллокатором. Проверяющая система
 * будет считать ошибкой, если не вся память будет
 * освбождена.
 **/
void cache_release(struct cache *cache) {
    release_slab_list(cache->totally_free.next);
    release_slab_list(cache->partially_busy.next);
    release_slab_list(cache->totally_busy.next);
}


/**
 * Функция аллокации памяти из кеширующего аллокатора.
 * Должна возвращать указатель на участок памяти размера
 * как минимум object_size байт (см cache_setup).
 * Гарантируется, что cache указывает на корректный
 * инициализированный аллокатор.
 **/
void *cache_alloc(struct cache *cache) {
    // сначала попробовать аллоцировать из partially_busy
    slab_header* partially_busy_slab = cache->partially_busy.next;

    if (partially_busy_slab != NULL) {
        // можно аллоцировать в SLAB'е из partially_busy
        uint8_t* ptr = (uint8_t*)(partially_busy_slab->free_block) + sizeof(slab_object);

        partially_busy_slab->free_block = partially_busy_slab->free_block->next;
        partially_busy_slab->free_slabs -= 1;

        // нужно ли перенести в список totally_busy
        if (partially_busy_slab->free_slabs == 0) {
            move_slab(partially_busy_slab, &cache->totally_busy);
        }
        return ptr;

    } else {
        // частично свободных нету, значит нужно выделять новый SLAB
        printf("alloc to NEW slab\n");

        slab_header* slab = (slab_header*)alloc_slab(cache->slab_order);

        // инициализация нового SLAB'а
        slab->free_slabs = cache->slab_objects-1;
        slab->next = NULL;
        slab->prev = NULL;
        slab->free_block = NULL;

        // помещение нового SLAB'а в partially_busy
        move_slab(slab, &cache->partially_busy);

        // нужно разбить новый SLAB на блоки
        uint8_t* slab_block_ptr = (uint8_t*)slab + sizeof(slab_header);
        slab_object* slab_block = ((slab_object*)slab_block_ptr);
        slab->free_block = slab_block;
        for(int i = 0;i < cache->slab_objects; i++){
            slab_block->next = (slab_object*)(slab_block_ptr + sizeof(slab_object) + cache->object_size);
            slab_block_ptr = (uint8_t*)slab_block->next;
            slab_block = ((slab_object*)slab_block_ptr);
        }

        // т.к. первый свободный блок будет аллоцирован, сюда нужно записать следующий
        uint8_t* ptr = (uint8_t*)(slab->free_block) + sizeof(slab_object);
        slab->free_block = slab->free_block->next;

        return ptr;
    }

}

/**
 * Функция освобождения памяти назад в кеширующий аллокатор.
 * Гарантируется, что ptr - указатель ранее возвращенный из
 * cache_alloc.
 **/
void cache_free(struct cache *cache, void *ptr) {
    uint8_t* slab_block_ptr = (uint8_t*)ptr - sizeof(slab_object);
    uint8_t* slab_ptr = (uint8_t*)((uintptr_t)slab_block_ptr & ~((1<<cache->slab_order+12) - 1));

    slab_object* slab_block = (slab_object*)slab_block_ptr;
    slab_header* slab = (slab_header*)slab_ptr;

    // возвращение блока в SLAB вставкой в начало связного списка
    slab_block->next = slab->free_block;
    slab->free_block = slab_block;

    slab->free_slabs += 1;

    // в какой из списков нужно поставить SLAB
    if (slab->free_slabs >= cache->slab_objects) {
        move_slab(slab, &cache->totally_free);
    } else if(slab->free_slabs - 1 == 0) {
        move_slab(slab, &cache->partially_busy);
    }

}


/**
 * Функция должна освободить все SLAB, которые не содержат
 * занятых объектов. Если SLAB не использовался для аллокации
 * объектов (например, если вы выделяли с помощью alloc_slab
 * память для внутренних нужд вашего алгоритма), то освбождать
 * его не обязательно.
 **/
void cache_shrink(struct cache *cache) {
    release_slab_list(cache->totally_free.next);
}


// test code
void print_cache_info(cache* cache);
void print_slab_list(slab_header* slab_list);
void print_slab_info(slab_header* slab);

int main() {
    // printf("start tests\n");
    struct cache* cache = (struct cache*)malloc(sizeof(struct cache));
    cache_setup(cache, 500);

    void* a = cache_alloc(cache);
    void* b = cache_alloc(cache);
    void* c = cache_alloc(cache);

    printf("%p %p %p\n\n", a, b, c);
    print_cache_info(cache);
    printf("\n");

    for(int i = 0;i<cache->slab_objects*4;i++) {
        void* p = cache_alloc(cache);
    }

    slab_header* slab_to_free = cache->totally_busy.next;
    
    while(slab_to_free) {
        uint8_t* ptr = (uint8_t*)slab_to_free + sizeof(slab_header);
        for(int i = 0;i<cache->slab_objects;i++) {
            ptr += sizeof(slab_object);
            cache_free(cache, ptr);
        }
        slab_to_free = slab_to_free->next;
    }


    print_cache_info(cache);
    printf("\n");

    // cache_free(cache, a);
    // cache_free(cache, c);
    // cache_free(cache, b);
    cache_shrink(cache);

    print_cache_info(cache);

    cache_release(cache);

    print_cache_info(cache);

    
    return 0;
}

void print_cache_info(cache* cache) {
    printf("cache addr: %p\n", cache);
    printf("object_size: %d\n", cache->object_size);
    printf("slab_order: %d\n", cache->slab_order);
    printf("slab_objects: %d\n", cache->slab_objects);
    printf("lists:\n");
    printf("\ttotally_free:\n");
    print_slab_list(cache->totally_free.next);
    printf("\tpartially_busy:\n");
    print_slab_list(cache->partially_busy.next);
    printf("\ttotally_busy:\n");
    print_slab_list(cache->totally_busy.next);
}

void print_slab_list(slab_header* slab_list) {
    slab_header* slab = slab_list;
    while(slab) {
        print_slab_info(slab);
        slab = slab->next;
    }
}

void print_slab_info(slab_header* slab) {
    printf("\033[1mslab addr\033[0m: %p\n", slab);
    printf("next slab addr: %p\n", slab->next);
    printf("prev slab addr: %p\n", slab->prev);
    printf("free_slabs: %d\n", slab->free_slabs);
    printf("free_block: %p\n", slab->free_block);
}


void *alloc_slab(int order) {
    size_t size = 4096 * 1 << order;
    return aligned_alloc(size, size);
}


void free_slab(void *slab) {
    free(slab);
}

