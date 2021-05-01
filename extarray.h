#pragma once

#ifndef EXTARRAY_H
#define EXTARRAY_H

#include <string.h>


typedef struct {
    void *data;
    int num;
    int max;
} ExtArray;


#define EXTARRAY(t) struct { \
    t *data; \
    int num; \
    int max; \
} 

static inline void extarray_ensure(ExtArray *array, int wanted, size_t size) {
    if (wanted > array->max) {
	    int new_max = 16;
	    while (new_max <= wanted)
		    new_max *= 2;
	    array->data = realloc(array->data, new_max * size);
	    array->max = new_max;
    }
}

static inline void *extarray_alloc(ExtArray *array, size_t size) {
    extarray_ensure(array, array->num + 1, size);
    void *ptr = array->data + array->num * size;
    array->num++;
    return ptr;
}

static inline void extarray_copy(ExtArray *dest_array, ExtArray *array, size_t size) {
    extarray_ensure(dest_array, array->num, size);
    memcpy(dest_array->data, array->data, array->num * size);
    dest_array->num = array->num;
}

#define EXTARRAY_ENSURE(array, wanted) extarray_ensure((ExtArray *) &array, wanted, sizeof(array.data[0]))

#define EXTARRAY_ALLOC(array) extarray_alloc((ExtArray *) &array, sizeof(array.data[0]))

#define EXTARRAY_FREE(array) free(array.data)

#define EXTARRAY_COPY(dest_array, array) extarray_copy((ExtArray *) &dest_array, (ExtArray *) &array, sizeof(array.data[0]))


#endif
