#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "dancing.h"


static void *array_allocate(void **array, int *max, int *num, size_t size) {
    if (*num >= *max) {
	    (*max) *= 2;
	    *array = realloc(*array, *max * size);
    }
    void *ptr = *array + *num * size;
    (*num)++;
    return ptr;
}

#define array_alloc(array,max,num) array_allocate((void **) &array, &max, &num, sizeof(array[0]))


static void insert_horizontally(Matrix *matrix, NodeId node, NodeId after) {
    NODE(node).left = after;
    NODE(node).right = NODE(after).right;
    NODE(after).right = node;
    NODE(NODE(node).right).left = node;
}


static void insert_vertically(Matrix *matrix, NodeId node, NodeId after) {
    NODE(node).up = after;
    NODE(node).down = NODE(after).down;
    NODE(after).down = node;
    NODE(NODE(node).down).up = node;
}


static void remove_horizontally(Matrix *matrix, NodeId node) {
    NODE(NODE(node).left).right = NODE(node).right;
    NODE(NODE(node).right).left = NODE(node).left;
}


static void remove_vertically(Matrix *matrix, NodeId node) {
    NODE(NODE(node).up).down = NODE(node).down;
    NODE(NODE(node).down).up = NODE(node).up;
}


static void restore_horizontally(Matrix *matrix, NodeId node) {
    NODE(NODE(node).left).right = node;
    NODE(NODE(node).right).left = node;
}


static void restore_vertically(Matrix *matrix, NodeId node) {
    NODE(NODE(node).up).down = node;
    NODE(NODE(node).down).up = node;
}


static NodeId allocate_node(Matrix *matrix) {
    #if INDEX_NODES
        NodeId id = matrix->num_nodes;
        array_alloc(matrix->nodes, matrix->max_nodes, matrix->num_nodes);
        return id;
    #else
        return array_alloc(matrix->nodes, matrix->max_nodes, matrix->num_nodes);
    #endif
}


static NodeId allocate_header(Matrix *matrix) {
    #if INDEX_NODES
        if (matrix->num_nodes != matrix->num_headers) {
            fprintf(stderr, "Warning, non-header nodes already inserted!");
            matrix->num_headers = matrix->num_nodes;
        }
        NodeId id = allocate_node(matrix);
        array_alloc(matrix->headers, matrix->max_headers, matrix->num_headers);
    #else
        NodeId id = array_alloc(matrix->headers, matrix->max_headers, matrix->num_headers);
    #endif

    if (id != ROOT) {
        NodeId last = NODE(ROOT).left;
        insert_horizontally(matrix, id, last);
    } else {
        NODE(id).left = id;
        NODE(id).right = id;
    }

    NODE(id).up = id;
    NODE(id).down = id;
    HEADER(id).index = matrix->num_headers - 1;
    HEADER(id).size = 0;
    return id;
}


Matrix *create_matrix() {
    Matrix *matrix = malloc(sizeof(Matrix));
    matrix->max_nodes = 10000;
    matrix->num_nodes = 0;
    matrix->nodes = malloc(matrix->max_nodes * sizeof(Node));
    matrix->max_headers = 1000;
    matrix->num_headers = 0;
    matrix->headers = malloc(matrix->max_headers * sizeof(Header));
    NodeId root = allocate_header(matrix);
    matrix->num_rows = 0;
    NODE(root).up = root;
    NODE(root).down = root;
    NODE(root).left = root;
    NODE(root).right = root;
    HEADER(root).name = "ROOT";
    HEADER(root).primary = 1;
    return matrix;
}


NodeId create_column(Matrix *matrix, int primary, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    NodeId column = allocate_header(matrix);
    HEADER(column).name = strdup(buffer);
    HEADER(column).primary = primary;

    NodeId prev_column = NODE(column).left;
    if (primary && !HEADER(prev_column).primary) {
        fprintf(stderr, "Warning, primary column '%s' appears after non-primary '%s'!\n", HEADER(column).name, HEADER(prev_column).name);
    }

    return column;
}


NodeId create_node(Matrix *matrix, NodeId after, NodeId column) {
    NodeId node = allocate_node(matrix);
    NODE(node).column = column;
    NodeId last = NODE(column).up;
    insert_vertically(matrix, node, last);
    HEADER(column).size++;

    if (after == 0) {
        NODE(node).left = node;
        NODE(node).right = node;
        matrix->num_rows++;
    } else {
        insert_horizontally(matrix, node, after);
    }

    return node;
}


void destroy_matrix(Matrix *matrix) {
    NodeId n;
    foreachlink(ROOT, right, n) {
        free(HEADER(n).name);
    }
    free(matrix->nodes);
    free(matrix->headers);
    free(matrix);
}


void print_matrix(Matrix *matrix) {
    NodeId n;
    printf("ROOT");
    int col = 1;
    foreachlink(ROOT, right, n) {
        while (col++ < HEADER(n).index)
            printf("\t");
        printf("\t%s (%d)", HEADER(n).name, HEADER(n).size);
    }
    printf("\n");

    foreachlink(ROOT, right, n) {
        NodeId n2;
        foreachlink(n, down, n2) {
            NodeId p = NODE(n2).left;
            if (HEADER(NODE(p).column).index < HEADER(n).index)
                continue;
            col = 1;
            while (col++ < HEADER(n).index)
                printf("\t0");
            printf("\t1");
            NodeId n3;
            foreachlink(n2, right, n3) {
                while (col++ < HEADER(NODE(n3).column).index)
                    printf("\t0");
                printf("\t1");
            }
            while (col++ < HEADER(ROOT).size)
                printf("\t0");
            printf("\n");
        }
    }
}


static NodeId choose_column(Matrix *matrix) {
    int best_size = INT_MAX;
    NodeId best_column = 0;
    NodeId n;
    foreachlink(ROOT, right, n) {
        if (!HEADER(n).primary)
            break;
        if (HEADER(n).size < best_size) {
            best_column = n;
            best_size = HEADER(n).size;
        if (best_size <= 1)
            break;
        }
    }

    return best_column;
}


static void cover_column(Matrix *matrix, NodeId column) {
    //printf("Covering %s\n", column->name);
    remove_horizontally(matrix, column);
    NodeId n;
    foreachlink(column, down, n) {
        NodeId n2;
        foreachlink(n, right, n2) {
            //printf("Hiding value in column %s\n", n2->column->name);
            remove_vertically(matrix, n2);
            HEADER(NODE(n2).column).size--;
        }
    }
    //matrix->root.size--;
}


static void uncover_column(Matrix *matrix, NodeId column) {
    //printf("Uncovering %s\n", column->name);
    restore_horizontally(matrix, column);
    NodeId n;
    foreachlink(column, up, n) {
        NodeId n2;
        foreachlink(n, left, n2) {
            HEADER(NODE(n2).column).size++;
            restore_vertically(matrix, n2);
        }
    }
    //matrix->root.size++;
}


void print_row(Matrix *matrix, NodeId row) {
    printf("%s", HEADER(NODE(row).column).name);
    NodeId n;
    foreachlink(row, right, n) {
        printf(", %s", HEADER(NODE(n).column).name);
    }
}


void print_solution(Matrix *matrix, NodeId *solution, int solution_size) {
    int i;
    printf("Solution:   ");
    for (i = 0; i < solution_size; i++) {
        print_row(matrix, solution[i]);
        printf(";   ");
    }
    printf("\n");
}


static int search_matrix_internal(Matrix *matrix, Callback solution_callback, NodeId *solution, int depth, void *baton) {
    int result = 0;

    matrix->search_calls++;
    NodeId column = choose_column(matrix);
    if (column == 0) {
        int result = solution_callback(matrix, solution, depth, baton);
        matrix->num_solutions++;
        return result;
    }
    //printf("Chose %s of size %d\n", column->name, column->size);

    cover_column(matrix, column);

    NodeId row;
    foreachlink(column, down, row) {
        solution[depth] = row;
        NodeId col;
        foreachlink(row, right, col) {
            cover_column(matrix, NODE(col).column);
        }

        result = search_matrix_internal(matrix, solution_callback, solution, depth + 1, baton);

        foreachlink(row, left, col) {
            uncover_column(matrix, NODE(col).column);
        }

        if (result)
            break;
    }

    uncover_column(matrix, column);

    return result;
}


int search_matrix(Matrix *matrix, Callback solution_callback, void *baton) {
    matrix->search_calls = 0;
    matrix->num_solutions = 0;

    NodeId *solution = malloc(sizeof(NodeId) * matrix->num_headers);

    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    int result = search_matrix_internal(matrix, solution_callback, solution, 0, baton);

    struct timespec stop_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time);

    double search_time = stop_time.tv_sec + stop_time.tv_nsec/1E+9
                       - start_time.tv_sec - start_time.tv_nsec/1E+9;

    printf("Matrix size: %d columns, %d rows, %d cells\n", matrix->num_headers - 1, matrix->num_rows, matrix->num_nodes);
    printf("Search calls: %ld\n", matrix->search_calls);
    printf("Solutions found: %ld\n", matrix->num_solutions);
    printf("Search time: %0.3f seconds\n", search_time);
    free(solution);

    return result;
}
