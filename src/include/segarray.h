#pragma once

#ifndef SEGARRAY_H
#define SEGARRAY_H


#define SEGMENT_SIZE 100

typedef struct {
    EXTARRAY(void *) current;
    EXTARRAY(void *) segments;
} SegArray;

#define SEGARRAY(t) struct { \
    EXTARRAY(t) current; \
    EXTARRAY(t *) segments; \
}

static inline void *segarray_alloc(SegArray *array, size_t size) {
    if (array->current.num >= array->current.max) {
        if (array->current.max > 0) {
            void **pos = EXTARRAY_ALLOC(array->segments);
            *pos = array->current.data;
            array->current.num = 0;
            array->current.max = 0;
            array->current.data = NULL;
        }
        extarray_ensure((ExtArray *) &array->current, SEGMENT_SIZE, size);
    }

    return extarray_alloc((ExtArray *) &array->current, size);
}

static inline void segarray_free(SegArray *array) {
    EXTARRAY_FREE(array->current);
    int i;
    for (i = 0; i < array->segments.num; i++) {
        free(array->segments.data[i]);
    }
    EXTARRAY_FREE(array->segments);
}

#define SEGARRAY_ALLOC(array) segarray_alloc((SegArray *) &array, sizeof(array.current.data[0]))

#define SEGARRAY_FREE(array) segarray_free((SegArray *) &array)


#endif
