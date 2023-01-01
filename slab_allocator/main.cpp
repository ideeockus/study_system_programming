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


struct slab_header {

};

struct slab_object {

};


/**
 * Эта структура представляет аллокатор, вы можете менять
 * ее как вам удобно. Приведенные в ней поля и комментарии
 * просто дают общую идею того, что вам может понадобится
 * сохранить в этой структуре.
 **/
struct cache {
    void* totally_free; /* список пустых SLAB-ов для поддержки cache_shrink */
    void* partially_busy; /* список частично занятых SLAB-ов */
    void* totally_busy; /* список заполненых SLAB-ов */

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
void cache_setup(struct cache *cache, size_t object_size)
{
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


/**
 * Функция освобождения будет вызвана когда работа с
 * аллокатором будет закончена. Она должна освободить
 * всю память занятую аллокатором. Проверяющая система
 * будет считать ошибкой, если не вся память будет
 * освбождена.
 **/
void cache_release(struct cache *cache)
{
    /* Реализуйте эту функцию. */
}


/**
 * Функция аллокации памяти из кеширующего аллокатора.
 * Должна возвращать указатель на участок памяти размера
 * как минимум object_size байт (см cache_setup).
 * Гарантируется, что cache указывает на корректный
 * инициализированный аллокатор.
 **/
void *cache_alloc(struct cache *cache)
{
    /* Реализуйте эту функцию. */
}


/**
 * Функция освобождения памяти назад в кеширующий аллокатор.
 * Гарантируется, что ptr - указатель ранее возвращенный из
 * cache_alloc.
 **/
void cache_free(struct cache *cache, void *ptr)
{
    /* Реализуйте эту функцию. */
}


/**
 * Функция должна освободить все SLAB, которые не содержат
 * занятых объектов. Если SLAB не использовался для аллокации
 * объектов (например, если вы выделяли с помощью alloc_slab
 * память для внутренних нужд вашего алгоритма), то освбождать
 * его не обязательно.
 **/
void cache_shrink(struct cache *cache)
{
    /* Реализуйте эту функцию. */
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

