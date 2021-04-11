#pragma once

#ifndef EXTARRAY_H
#define EXTARRAY_H

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

static inline void *array_allocate(ExtArray *array, size_t size) {
    if (array->num >= array->max) {
	    int new_max = 16;
	    while (new_max <= array->num)
		new_max *= 2;
	    array->data = realloc(array->data, new_max * size);
	    array->max = new_max;
    }
    void *ptr = array->data + array->num * size;
    array->num++;
    return ptr;
}

#define array_alloc(array) array_allocate((ExtArray *) &array, sizeof(array.data[0]))


#endif
