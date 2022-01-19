#ifndef __DM_LIST_H__
#define __DM_LIST_H__

#include "stdint.h"
#include "stdlib.h"
#include "dm_mem.h"
#include "dm_logger.h"
#include "string.h"

#define DM_LIST_RESIZE_FACTOR 2

#define dm_list(TYPE) struct { size_t capacity, size, elem_size; TYPE* array; }

#define dm_list_init(LIST, TYPE)\
do{\
    (LIST)->capacity = 1;\
    (LIST)->size = 0;\
    (LIST)->elem_size = sizeof(TYPE);\
    (LIST)->array = (TYPE*)dm_alloc(sizeof(TYPE), DM_MEM_LIST);\
}while(0)

#define dm_list_destroy(LIST) dm_free((LIST)->array, (LIST)->capacity * (LIST)->elem_size, DM_MEM_LIST)

#define dm_list_grow(LIST)\
do{\
    if((LIST)->size>=(LIST)->capacity/DM_LIST_RESIZE_FACTOR)\
    {\
        size_t new_capacity = (LIST)->capacity * DM_LIST_RESIZE_FACTOR;\
        dm_mem_db_adjust((new_capacity - (LIST)->capacity) * (LIST)->elem_size, DM_MEM_LIST);\
        (LIST)->array = dm_realloc((LIST)->array, (LIST)->elem_size * new_capacity);\
        (LIST)->capacity = new_capacity;\
    }\
}while(0)

#define dm_list_shrink(LIST)\
do{\
    if((LIST)->size<(LIST)->capacity/DM_LIST_RESIZE_FACTOR)\
    {\
        size_t new_capacity = (LIST)->capacity / DM_LIST_RESIZE_FACTOR;\
        dm_mem_db_adjust((new_capacity - (LIST)->capacity) * (LIST)->elem_size, DM_MEM_LIST);\
        (LIST)->array = dm_realloc((LIST)->array, (LIST)->elem_size * new_capacity);\
        (LIST)->capacity = new_capacity;\
    }\
}while(0)

#define dm_list_append(LIST, ELEM)\
do{\
    (LIST)->array[(LIST)->size] = ELEM;\
    (LIST)->size += 1;\
    dm_list_grow(LIST);\
}while(0)

#define dm_list_insert(LIST, ELEM, INDEX)\
do{\
    if(INDEX<0 || INDEX>(LIST)->size)\
    {\
        DM_LOG_ERROR("Trying to insert element to a list with an invalid index: %d", INDEX);\
    }\
    else\
    {\
        size_t block_size = ((LIST)->size - INDEX) * (LIST)->elem_size;\
        dm_memmove((LIST)->array + INDEX+1, (LIST)->array + INDEX, block_size);\
        (LIST)->array[INDEX] = ELEM;\
        (LIST)->size += 1;\
        dm_list_grow(LIST);\
    }\
}while(0)

#define dm_list_pop_back(LIST)\
do{\
    if((LIST)->size > 0)\
    {\
        (LIST)->size -= 1;\
        dm_list_shrink(LIST);\
    }\
    else\
    {\
        DM_LOG_ERROR("Trying to pop from zero length list!");\
    }\
} while(0)

#define dm_list_pop_at(LIST, INDEX)\
do{\
    if(INDEX<0 || INDEX>(LIST)->size)\
    {\
        DM_LOG_ERROR("Trying to pop element from a list with an invalid index: %d", INDEX);\
    }\
    else\
    {\
        size_t block_size = ((LIST)->size - INDEX) * (LIST)->elem_size;\
        dm_memmove((LIST)->array + INDEX, (LIST)->array + INDEX + 1, block_size);\
        (LIST)->size -= 1;\
        dm_list_shrink(LIST);\
    }\
} while(0)

#define dm_list_clear(LIST)\
do{\
    if((LIST)->size>0)\
    {\
        dm_mem_db_adjust((1 - (LIST)->capacity) * (LIST)->elem_size, DM_MEM_LIST);\
        (LIST)->size = 0;\
        (LIST)->capacity = 1;\
        (LIST)->array = dm_realloc((LIST)->array, (LIST)->elem_size);\
    }\
    else\
    {\
        DM_LOG_WARN("Trying to clear empty list...");\
    }\
}while(0)

// helpful functions
#define dm_list_for_range(LIST, ITER) for(int ITER=0; ITER<LIST.size; ITER++)

#define dm_list_push_array(LIST, ARRAY, SIZE)\
do{\
    for(int i=0; i<SIZE; i++)\
    {\
        dm_list_append(LIST, ARRAY[i]);\
    }\
}while(0)

#endif