#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dancing.h"


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
        NodeId id = matrix->nodes.num;
        EXTARRAY_ALLOC(matrix->nodes);
        return id;
    #else
        return SEGARRAY_ALLOC(matrix->nodes);
    #endif
}


static NodeId allocate_header(Matrix *matrix) {
    #if INDEX_NODES
        if (matrix->nodes.num != matrix->headers.num) {
            fprintf(stderr, "Warning, non-header nodes already inserted!");
            matrix->headers.num = matrix->nodes.num;
        }
        NodeId id = allocate_node(matrix);
        EXTARRAY_ALLOC(matrix->headers);
    #else
        NodeId id = SEGARRAY_ALLOC(matrix->headers);
        if (matrix->headers.segments.num == 0 && matrix->headers.current.num == 1) {
            matrix->root = id;
        }
    #endif

    if (id != ROOT) {
        NodeId last = NODE(ROOT).left;
        insert_horizontally(matrix, id, last);
        HEADER(id).index = HEADER(last).index + 1;
    } else {
        NODE(id).left = id;
        NODE(id).right = id;
        HEADER(id).index = 0;
    }

    NODE(id).up = id;
    NODE(id).down = id;
    HEADER(id).size = 0;
    return id;
}


Matrix *create_matrix() {
    Matrix *matrix = malloc(sizeof(Matrix));
    memset(matrix, 0, sizeof(Matrix));

    NodeId root = allocate_header(matrix);
    matrix->num_rows = 0;
    NODE(root).up = root;
    NODE(root).down = root;
    NODE(root).left = root;
    NODE(root).right = root;
    HEADER(root).name = "ROOT";
    HEADER(root).primary = 1;

    EXTARRAY_ENSURE(matrix->solution, 100);

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

    matrix->num_columns++;
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

    matrix->num_nodes++;
    return node;
}


void destroy_matrix(Matrix *matrix) {
    NodeId n;
    foreachlink(ROOT, right, n) {
        free(HEADER(n).name);
    }
#if INDEX_NODES
    EXTARRAY_FREE(matrix->nodes);
    EXTARRAY_FREE(matrix->headers);
#else
    SEGARRAY_FREE(matrix->nodes);
    SEGARRAY_FREE(matrix->headers);
#endif
    EXTARRAY_FREE(matrix->solution);
    free(matrix);
}


#define FIXUP(ptr, old_array, new_array) (ptr = (ptr) - (void *) (old_array) + (void *) (new_array));


Matrix *clone_matrix(Matrix *matrix) {
#if 0
    Matrix *new_matrix = create_matrix();
    EXTARRAY_COPY(new_matrix->nodes, matrix->nodes);
    EXTARRAY_COPY(new_matrix->headers, matrix->headers);
    EXTARRAY_COPY(new_matrix->solution, matrix->solution);
    
    /* Make sure column names aren't shared with original. */
    NodeId n;
    foreachlink(ROOT, right, n) {
        HEADER(n).name = strdup(HEADER(n).name);
    }
    
#if INDEX_NODES
#else
    /* Fix up node pointers. */
    int i;
    for (i = 0; i < new_matrix->nodes.num; i++) {
	Node *node = &new_matrix->nodes.data[i];
	FIXUP(node->left, matrix->nodes.data, new_matrix->nodes.data);
	FIXUP(node->right, matrix->nodes.data, new_matrix->nodes.data);
	FIXUP(node->up, matrix->nodes.data, new_matrix->nodes.data);
	FIXUP(node->down, matrix->nodes.data, new_matrix->nodes.data);
	FIXUP(node->column, matrix->nodes.data, new_matrix->nodes.data);
    }
#endif
    
    return new_matrix;
#endif
    return NULL;
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
                printf("\t");
            printf("\t*");
            NodeId n3;
            foreachlink(n2, right, n3) {
                while (col++ < HEADER(NODE(n3).column).index)
                    printf("\t");
                printf("\t*");
            }
            while (col++ < HEADER(ROOT).size)
                printf("\t");
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
        // if (HEADER(n).size < 1)
        //     continue;
        // printf("select %s\n", HEADER(n).name);
        // return n;
    }

    return best_column;
}


static void cover_column(Matrix *matrix, NodeId column) {
    //printf("cover %s\n", HEADER(column).name);
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
    //printf("uncover %s\n", HEADER(column).name);
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


void print_solution(Matrix *matrix) {
    int i;
    printf("Solution:   ");
    for (i = 0; i < matrix->solution.num; i++) {
        print_row(matrix, matrix->solution.data[i]);
        printf(";   ");
    }
    printf("\n");
}


int search_matrix_internal(Matrix *matrix, int depth, int max_depth) {
    //printf("depth %d\n", depth);
    //print_matrix(matrix);

    int result = 0;

    matrix->search_calls++;
    
    if (depth >= max_depth) {
	return matrix->depth_callback(matrix, matrix->depth_baton);
    }	
    
    NodeId column = choose_column(matrix);
    if (column == 0) {
        int result = matrix->solution_callback(matrix, matrix->solution_baton);
        matrix->num_solutions++;
        return result;
    }
    //printf("Chose %s of size %d\n", column->name, column->size);

    cover_column(matrix, column);

    NodeId *solution_spot = &matrix->solution.data[matrix->solution.num];
    matrix->solution.num++;

    NodeId row;
    foreachlink(column, down, row) {
        *solution_spot = row;
        //printf("add to solution ");
        //print_row(matrix, row);
        //printf("\n");

        NodeId col;
        foreachlink(row, right, col) {
            cover_column(matrix, NODE(col).column);
        }

        result = search_matrix_internal(matrix, depth + 1, max_depth);

        foreachlink(row, left, col) {
            uncover_column(matrix, NODE(col).column);
        }

        if (result)
            break;
    }

    matrix->solution.num--;

    uncover_column(matrix, column);

    return result;
}


NodeId find_column(Matrix *matrix, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    NodeId n;
    foreachlink(ROOT, right, n) {
        if (strcmp(HEADER(n).name, buffer) == 0)
            return n;
    }

    return 0;

}


NodeId find_row(Matrix *matrix, NodeId *columns, int num_columns) {
    int num_found;

    NodeId n;
    foreachlink(columns[0], down, n) {
        num_found = 1;

        NodeId n2;
        foreachlink(n, right, n2) {
            int j;
            for (j = 1; j < num_columns; j++) {
                if (columns[j] == NODE(n2).column) {
                    num_found++;
                }
            }
        }

        if (num_found >= num_columns)
            return n;
    }

    return 0;
}


void choose_row(Matrix *matrix, NodeId row) {
    NodeId *solution_spot = EXTARRAY_ALLOC(matrix->solution);
    *solution_spot = row;

    cover_column(matrix, NODE(row).column);
    NodeId col;
    foreachlink(row, right, col) {
        cover_column(matrix, NODE(col).column);
    }
    printf("Chose row: ");
    print_row(matrix, row);
    printf("\n");
}


int search_matrix(Matrix *matrix, int max_depth) {
    matrix->search_calls = 0;
    matrix->num_solutions = 0;

    EXTARRAY_ENSURE(matrix->solution, matrix->num_rows);

    if (max_depth <= 0)
	    max_depth = INT_MAX;

    int result = search_matrix_internal(matrix, 0, max_depth);

    return result;
}
