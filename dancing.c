#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

#include "dancing.h"


static void insert_horizontally(Node *node, Node *after) {
    node->left = after;
    node->right = after->right;
    after->right = node;
    node->right->left = node;
}


static void insert_vertically(Node *node, Node *after) {
    node->up = after;
    node->down = after->down;
    after->down = node;
    node->down->up = node;
}


static void remove_horizontally(Node *node) {
    node->left->right = node->right;
    node->right->left = node->left;
}


static void remove_vertically(Node *node) {
    node->up->down = node->down;
    node->down->up = node->up;
}


static void restore_horizontally(Node *node) {
    node->left->right = node;
    node->right->left = node;
}


static void restore_vertically(Node *node) {
    node->up->down = node;
    node->down->up = node;
}


static Header *allocate_header(Matrix *matrix) {
    Header *header = &matrix->headers[matrix->num_headers];

    Node *last = matrix->root.node.left;
    insert_horizontally((Node *) header, last);
    matrix->root.size++;

    header->node.up = (Node *) header;
    header->node.down = (Node *) header;
    header->index = matrix->num_headers;
    header->size = 0;

    matrix->num_headers++;
    return header;
}


static Node *allocate_node(Matrix *matrix) {
    Node *node = &matrix->nodes[matrix->num_nodes];
    matrix->num_nodes++;
    return node;
}


Matrix *create_matrix(int max_nodes, int max_headers) {
    Matrix *matrix = malloc(sizeof(Matrix));
    matrix->num_nodes = 0;
    matrix->nodes = malloc(max_nodes * sizeof(Node));
    matrix->num_headers = 0;
    matrix->headers = malloc(max_headers * sizeof(Header));
    matrix->root.node.up = (Node *) &matrix->root;
    matrix->root.node.down = (Node *) &matrix->root;
    matrix->root.node.left = (Node *) &matrix->root;
    matrix->root.node.right = (Node *) &matrix->root;
    matrix->root.index = -1;
    matrix->root.name = "ROOT";
    matrix->root.size = 0;
    matrix->root.primary = 1;
    return matrix;
}


Header *create_column(Matrix *matrix, int primary, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    Header *column = allocate_header(matrix);
    column->name = strdup(buffer);
    column->primary = primary;

    Header *prev_column = (Header *) column->node.left;
    if (primary && !prev_column->primary) {
        fprintf(stderr, "Warning, primary column '%s' appears after non-primary '%s'!\n", column->name, prev_column->name);
    }

    return column;
}


Node *create_node(Matrix *matrix, Node *after, Header *column) {
    Node *node = allocate_node(matrix);
    node->column = column;
    Node *last = column->node.up;
    insert_vertically(node, last);
    column->size++;

    if (after == NULL) {
        node->left = node;
        node->right = node;
    } else {
        insert_horizontally(node, after);
    }

    return node;
}


void destroy_matrix(Matrix *matrix) {
    free(matrix->nodes);
    Node *n;
    foreachlink(&matrix->root.node, right, n) {
        Header *h = (Header *) n;
        free(h->name);
    }
    free(matrix->headers);
    free(matrix);
}


void print_matrix(Matrix *matrix) {
    Node *n;
    printf("ROOT");
    int col = 0;
    foreachlink(&matrix->root.node, right, n) {
        Header *column = (Header *) n;
        while (col++ < column->index)
            printf("\t");
        printf("\t%s (%d)", column->name, column->size);
    }
    printf("\n");

    foreachlink(&matrix->root.node, right, n) {
        Header *column = (Header *) n;
        Node *n2;
        foreachlink(n, down, n2) {
            Node *p = n2->left;
            if (p->column->index < column->index)
                continue;
            col = 0;
            while (col++ < column->index)
                printf("\t0");
            printf("\t1");
            Node *n3;
            foreachlink(n2, right, n3) {
                while (col++ < n3->column->index)
                    printf("\t0");
                printf("\t1");
            }
            while (col++ < matrix->root.size)
                printf("\t0");
            printf("\n");
        }
    }
}


static Header *choose_column(Matrix *matrix) {
    int best_size = INT_MAX;
    Header *best_column = NULL;
    Node *n;
    foreachlink((Node *) &matrix->root, right, n) {
        Header *column = (Header *) n;
        if (!column->primary)
            break;
        if (column->size < best_size) {
            best_column = column;
            best_size = column->size;
        if (best_size <= 1)
            break;
        }
    }

    return best_column;
}


static void cover_column(Matrix *matrix, Header *column) {
    //printf("Covering %s\n", column->name);
    remove_horizontally((Node *) column);
    Node *n;
    foreachlink((Node *) column, down, n) {
        Node *n2;
        foreachlink(n, right, n2) {
            //printf("Hiding value in column %s\n", n2->column->name);
            remove_vertically(n2);
            n2->column->size--;
        }
    }
    matrix->root.size--;
}


static void uncover_column(Matrix *matrix, Header *column) {
    //printf("Uncovering %s\n", column->name);
    restore_horizontally((Node *) column);
    Node *n;
    foreachlink((Node *) column, up, n) {
        Node *n2;
        foreachlink(n, left, n2) {
            n2->column->size++;
            restore_vertically(n2);
        }
    }
    matrix->root.size++;
}


static void print_row(Node *row) {
    printf("%s", row->column->name);
    Node *n;
    foreachlink(row, right, n) {
        printf(", %s", n->column->name);
    }
}


void print_solution(Node **solution, int solution_size) {
    int i;
    printf("Solution:   ");
    for (i = 0; i < solution_size; i++) {
        print_row(solution[i]);
        printf(";   ");
    }
    printf("\n");
}


int search_matrix_internal(Matrix *matrix, Callback solution_callback, Node **solution, int depth, void *baton) {
    int result = 0;

    matrix->search_calls++;
    Header *column = choose_column(matrix);
    if (column == NULL) {
        int result = solution_callback(matrix, solution, depth, baton);
        matrix->num_solutions++;
        return result;
    }
    //printf("Chose %s of size %d\n", column->name, column->size);

    cover_column(matrix, column);

    Node *row;
    foreachlink((Node *) column, down, row) {
        solution[depth] = row;
        Node *col;
        foreachlink(row, right, col) {
            cover_column(matrix, col->column);
        }

        result = search_matrix_internal(matrix, solution_callback, solution, depth + 1, baton);

        foreachlink(row, left, col) {
            uncover_column(matrix, col->column);
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

    Node **solution = malloc(sizeof(Node*) * matrix->root.size);

    int result = search_matrix_internal(matrix, solution_callback, solution, 0, baton);

    printf("Search calls: %ld\n", matrix->search_calls);
    printf("Solutions found: %ld\n", matrix->num_solutions);
    free(solution);

    return result;
}
