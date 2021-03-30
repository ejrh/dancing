#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

struct Header;

typedef struct Node {
    struct Node *up, *down;
    struct Node *left, *right;
    struct Header *column;
} Node;

typedef struct Header {
    Node node;
    char *name;
    int size;
    int primary;
} Header;

typedef struct Matrix {
    int num_nodes;
    Node *nodes;
    int num_headers;
    Header *headers;
    Node root;
} Matrix;

#define foreachlink(h,a,x) for (x = (h)->a; x != (h); x = x->a)

void insert_horizontally(Node *node, Node *after) {
    node->left = after;
    node->right = after->right;
    after->right = node;
    node->right->left = node;
}

void insert_vertically(Node *node, Node *after) {
    node->up = after;
    node->down = after->down;
    after->down = node;
    node->down->up = node;
}

void remove_horizontally(Node *node) {
    node->left->right = node->right;
    node->right->left = node->left;
}

void remove_vertically(Node *node) {
    node->up->down = node->down;
    node->down->up = node->up;
}

void restore_horizontally(Node *node) {
    node->left->right = node;
    node->right->left = node;
}

void restore_vertically(Node *node) {
    node->up->down = node;
    node->down->up = node;
}

Header *allocate_header(Matrix *matrix) {
    Header *header = &matrix->headers[matrix->num_headers];

    Node *last = matrix->root.left;
    insert_horizontally((Node *) header, last);

    header->node.up = (Node *) header;
    header->node.down = (Node *) header;
    header->size = 0;

    matrix->num_headers++;
    return header;
}

Node *allocate_node(Matrix *matrix, Node *after, Header *column) {
    Node *node = &matrix->nodes[matrix->num_nodes];
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

    matrix->num_nodes++;
    return node;
}

Matrix *create_matrix(int max_nodes, int max_headers) {
    Matrix *matrix = malloc(sizeof(Matrix));
    matrix->num_nodes = 0;
    matrix->nodes = malloc(max_nodes * sizeof(Node));
    matrix->num_headers = 0;
    matrix->headers = malloc(max_headers * sizeof(Header));
    matrix->root.up = &matrix->root;
    matrix->root.down = &matrix->root;
    matrix->root.left = &matrix->root;
    matrix->root.right = &matrix->root;
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

    return column;
}

void destroy_matrix(Matrix *matrix) {
    free(matrix->nodes);
    Node *n;
    foreachlink(&matrix->root, right, n) {
        Header *h = (Header *) n;
        free(h->name);
    }
    free(matrix->headers);
    free(matrix);
}

void print_matrix(Matrix *matrix) {
    Node *n;
    Header *visible_columns[100];
    int num_visible_columns = 0;
    foreachlink(&matrix->root, right, n) {
        Header *column = (Header *) n;
        printf("\t%s (%d)", column->name, column->size);
        visible_columns[num_visible_columns] = column;
        num_visible_columns++;
    }
    printf("\n");

    int col_pos = 0;
    foreachlink(&matrix->root, right, n) {
        Node *n2;
        foreachlink(n, down, n2) {
            Node *visible_nodes[100];
            memset(visible_nodes, 0, sizeof(visible_nodes));
            visible_nodes[col_pos] = n2;
            Node *n3;
            foreachlink(n2, right, n3) {
                Header *n3_col = n3->column;
                int i;
                for (i = 0; i < num_visible_columns; i++) {
                    if (visible_columns[i] == n3_col) {
                        visible_nodes[i] = n3;
                        break;
                    }
                }
                if (i <= col_pos) {
                    goto skip_row;
                }
            }

            int i;
            for (i = 0; i < num_visible_columns; i++) {
                if (visible_nodes[i]) {
                    printf("\t1");
                } else {
                    printf("\t0");
                }
            }
            printf("\n");
        skip_row:
            ;
        }
        col_pos++;
    }
}

Header *choose_column(Matrix *matrix) {
    int best_size = INT_MAX;
    Header *best_column = NULL;
    Node *n;
    foreachlink(&matrix->root, right, n) {
        Header *column = (Header *) n;
        if (!column->primary)
            continue;
        if (column->size < best_size) {
            best_column = column;
            best_size = column->size;
        }
    }

    return best_column;
}

void cover_column(Matrix *matrix, Header *column) {
    //printf("Covering %s\n", column->name);
    remove_horizontally((Node *) column);
    Node *n;
    foreachlink((Node *) column, down, n) {
        Node *n2;
        foreachlink(n, right, n2) {
            remove_vertically(n2);
            n2->column->size--;
        }
    }
}

void uncover_column(Matrix *matrix, Header *column) {
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
}

void print_row(Node *row) {
    printf("%s", row->column->name);
    Node *n;
    foreachlink(row, right, n) {
        printf(", %s", n->column->name);
    }
}

void print_solution(int size, Node **solution) {
    int i;
    printf("Solution:   ");
    for (i = 0; i < size; i++) {
        print_row(solution[i]);
        printf(";   ");
    }
    printf("\n");
}

static long search_calls = 0;

int search(Matrix *matrix, int depth, Node **solution) {
    search_calls++;
    Header *column = choose_column(matrix);
    if (column == NULL) {
        print_solution(depth, solution);
        return 1;
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

        search(matrix, depth + 1, solution);

        foreachlink(row, left, col) {
            uncover_column(matrix, col->column);
        }
    }

    uncover_column(matrix, column);

    return 0;
}

Matrix *create_queens_problem(int size) {
    int r_cols = size;
    int f_cols = size;
    int a_cols = 2 * size - 1;
    int b_cols = 2 * size - 1;
    int num_nodes = 4 * size * size;
    Matrix *matrix = create_matrix(num_nodes, r_cols + f_cols + a_cols + b_cols);

    int i;
    for (i = 0; i < size; i++) {
        create_column(matrix, 1, "R%d", i);
    }
    for (i = 0; i < size; i++) {
        create_column(matrix, 1, "F%d", i);
    }
    for (i = 0; i < 2 * size - 1; i++) {
        create_column(matrix, 0, "A%d", i);
    }
    for (i = 0; i < 2 * size - 1; i++) {
        create_column(matrix, 0, "B%d", i);
    }

    for (i = 0; i < size; i++) {
        int j;
        for (j = 0; j < size; j++) {
            int a = i + j;
            int b = size - 1 - i + j;
            Header *r_col = &matrix->headers[i];
            Header *f_col = &matrix->headers[r_cols + j];
            Header *a_col = &matrix->headers[r_cols + f_cols + a];
            Header *b_col = &matrix->headers[r_cols + f_cols + a_cols + b];
            Node *node = allocate_node(matrix, NULL, r_col);
            node = allocate_node(matrix, node, f_col);
            node = allocate_node(matrix, node, a_col);
            node = allocate_node(matrix, node, b_col);
        }
    }

    return matrix;
}

int main(int argc, char *argv[]) {
    Matrix *matrix = create_queens_problem(8);

    print_matrix(matrix);

    Node *solution[100];
    search(matrix, 0, solution);

    destroy_matrix(matrix);
    printf("Search calls: %ld\n", search_calls);
    return 0;
}
