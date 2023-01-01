#include <cstdlib>
#include <cstdio>

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
    slab_header* totally_free; /* список пустых SLAB-ов для поддержки cache_shrink */
    slab_header* partially_busy; /* список частично занятых SLAB-ов */
    slab_header* totally_busy; /* список заполненых SLAB-ов */

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
    /* Реализуйте эту функцию. */
    int min_slab_objects = 2 << 7;

    cache->totally_free = NULL;
    cache->partially_busy = NULL;
    cache->totally_busy = NULL;

    // предварительный размер SLAB'а
    size_t slab_estimated_mem = (sizeof(slab_object) + object_size) * min_slab_objects + sizeof(slab_header);

    // нужно найти slab_order
    int i = 0;
    while(i <= 10 && 4096 * (1 << i) <= slab_estimated_mem) {
        i++;
    }

    // размер должен быть до 4Mib
    size_t slab_mem = 4096 * 1 << i;
    size_t slab_objects = (slab_mem - sizeof(slab_header)) / object_size;

    cache->object_size = object_size;
    cache->slab_order = i;
    cache->slab_objects = slab_objects;
}


void release_slab_list(void* slab_list) {
    slab_header* slab = slab_list;
    while(slab) {
        slab_to_release = slab;
        slab = slab->next; 
        free(slab_to_release);
    }
}

/**
 * Функция освобождения будет вызвана когда работа с
 * аллокатором будет закончена. Она должна освободить
 * всю память занятую аллокатором. Проверяющая система
 * будет считать ошибкой, если не вся память будет
 * освбождена.
 **/
void cache_release(struct cache *cache) {
    /* Реализуйте эту функцию. */
    // slab_header* slab = cache->totally_free;
    // while(slab) {
    //     cache->totally_free = slab->next;
    //     free(slab);
    //     slab = cache->totally_free;
    // }
    release_slab_list(cache->totally_free);
    release_slab_list(cache->partially_busy);
    release_slab_list(cache->totally_busy);
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
    slab_header* partially_busy_slab = cache->partially_busy;
    // while(partially_busy_slab != NULL && partially_busy_slab->free_slabs == 0) {
    //     partially_busy_slab->next;
    // }

    if (partially_busy_slab != NULL) {
        // можно аллоцировать в SLAB'е из partially_busy
        uint8_t* ptr = (uint8_t*)(partially_busy_slab->free_block) + sizeof(slab_object);

        partially_busy_slab->free_block = partially_busy_slab->free_block->next;
        partially_busy_slab->free_slabs -= 1;

        // нужно ли перенести в список totally_busy
        if (partially_busy_slab->free_slabs == 0) {
            // убрать из списка partially_busy
            cache->partially_busy = partially_busy_slab->next;

            // вставить в начало totally_busy
            partially_busy_slab->next = cache->totally_busy->next;
            cache->totally_busy->next = partially_busy_slab;
            partially_busy_slab->prev = NULL


        }
        return ptr;

    } else /*if (partially_busy_slab == NULL)*/ {
        // частично свободных нету, значит нужно выделять новый SLAB
        slab_header* slab = alloc_slab(cache->slab_order);

        // инициализация нового SLAB'а
        slab->free_slabs = cache->slab_objects-1;
        slab->next = NULL;
        slab->prev = NULL;
        slab->free_block = NULL;

        // помещение нового SLAB'а в partially_busy
        cache->partially_busy = slab;

        // нужно разбить новый SLAB на блоки
        uint8_t* slab_block_ptr = (uint8_t*)slab + sizeof(slab_header);
        slab_object slab_block = ((slab_object*)slab_block_ptr);
        slab->free_block = slab_block;
        for(int i = 0;i < cache->slab_objects; i++){
            slab_block->next = slab_block_ptr + sizeof(slab_object) + cache->object_size;
            slab_block_ptr = slab_block->next;
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
    uint8_t* slab_ptr = slab_block_ptr &~(1<<i - 1);

    slab_object* slab_block = (slab_object*)slab_block_ptr;
    slab_header* slab = (slab_header*)slab_ptr;

    // возвращение блока в SLAB вставкой в начало связного списка
    slab_block->next = slab->free_block;
    slab->free_block = slab_block;

    slab->free_slabs += 1;

    // в какой из списков нужно поставить SLAB
    if (slab->free_slabs >= cache->slab_objects) {
        slab->next = cache->totally_free;
        slab->prev = NULL;
        cache->totally_free = slab;
    } else {
        slab->prev->next = slab->next;
        slab->next = cache->partially_busy;
        cache->partially_busy = slab;
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
    release_slab_list(cache->totally_free);
}



// test code
int main() {
    
    
    return 0;
}



void *alloc_slab(int order) {
    size_t size = 4096 * 1 << order;
    return aligned_alloc(size, size);
}


void free_slab(void *slab) {
    free(slab);
}

