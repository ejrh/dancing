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
    int index;
    char *name;
    int size;
    int primary;
} Header;

typedef struct Matrix {
    int num_nodes;
    Node *nodes;
    int num_headers;
    Header *headers;
    Header root;
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
    matrix->root.node.up = (Node *) &matrix->root;
    matrix->root.node.down = (Node *) &matrix->root;
    matrix->root.node.left = (Node *) &matrix->root;
    matrix->root.node.right = (Node *) &matrix->root;
    matrix->root.index = -1;
    matrix->root.name = "ROOT";
    matrix->root.size = 0;
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

Header *choose_column(Matrix *matrix) {
    int best_size = INT_MAX;
    Header *best_column = NULL;
    Node *n;
    foreachlink((Node *) &matrix->root, right, n) {
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
            //printf("Hiding value in column %s\n", n2->column->name);
            remove_vertically(n2);
            n2->column->size--;
        }
    }
    matrix->root.size--;
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
    matrix->root.size++;
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


#define NUM_PENTOMINOES 12

#define PENTOMINO_LENGTH 5

typedef struct Pentomino {
    char *name;
    int r[PENTOMINO_LENGTH];
    int c[PENTOMINO_LENGTH];
    int flips;
    int rotations;
    int rows;
    int cols;
} Pentomino;

static Pentomino PENTOMINOES[NUM_PENTOMINOES] = {
    { "F", {0,0,1,1,2}, {1,2,0,1,1}, 2, 4, 3, 3 },
    { "I", {0,1,2,3,4}, {0,0,0,0,0}, 1, 2, 5, 1 },
    { "L", {0,1,2,3,3}, {0,0,0,0,1}, 2, 4, 4, 2 },
    { "N", {0,1,2,2,3}, {1,1,0,1,0}, 2, 4, 4, 2 },
    { "P", {0,0,1,1,2}, {0,1,0,1,0}, 2, 4, 3, 2 },
    { "T", {0,0,0,1,2}, {0,1,2,1,1}, 1, 4, 3, 3 },
    { "U", {0,0,1,1,1}, {0,2,0,1,2}, 1, 4, 2, 3 },
    { "V", {0,1,2,2,2}, {0,0,0,1,2}, 1, 4, 3, 3 },
    { "W", {0,1,1,2,2}, {0,0,1,1,2}, 1, 4, 3, 3 },
    { "X", {0,1,1,1,2}, {1,0,1,2,1}, 1, 1, 3, 3 },
    { "Y", {0,1,1,2,3}, {1,0,1,1,1}, 2, 4, 4, 2 },
    { "Z", {0,0,1,2,2}, {0,1,1,1,2}, 2, 2, 3, 3 }
};

int arrange_pentomino(Pentomino *p, int i, int j, int flip, int rotation, int board_rows, int board_cols, Header *square_columns[8][8], Header **positions) {
    int k;
    for (k = 0; k < PENTOMINO_LENGTH; k++) {
        int r = p->r[k];
        int c = p->c[k];

        if (flip)
            c = p->cols - 1 - c;

        int rows = p->rows;
        int cols = p->cols;
        int ri = rotation;
        while (ri > 0) {
            int t = r;
            r = c;
            c = rows - 1 - t;
            t = rows;
            rows = cols;
            cols = t;
            ri--;
        }

        r += i;
        c += j;

        if (r < 0 || c < 0 || r >= board_rows || c >= board_cols)
            return 0;

        Header *pos = square_columns[r][c];
        if (pos == NULL)
            return 0;

        positions[k] = pos;
    }

    return 1;
}

Matrix *create_pentominoes_problem() {
    int board_size = 8;
    int num_squares = board_size*board_size - 2*2;
    int num_cols = NUM_PENTOMINOES + num_squares;
    int num_rows = 0;
    int i;
    for (i = 0; i < NUM_PENTOMINOES; i++) {
        Pentomino *p = &PENTOMINOES[i];
        num_rows += p->flips * p->rotations * (board_size + 1 - p->rows) * (board_size + 1 - p->cols);
    }
    int num_nodes = (PENTOMINO_LENGTH + 1) * num_rows;
    Matrix *matrix = create_matrix(num_nodes, num_cols);

    Header *square_columns[8][8];
    Header *piece_columns[NUM_PENTOMINOES];

    for (i = 0; i < board_size; i++) {
        int j;
        for (j = 0; j < board_size; j++) {
            if (i >= 3 && i <= 4 && j >= 3 && j <= 4) {
                square_columns[i][j] = NULL;
            } else {
                square_columns[i][j] = create_column(matrix, 1, "%d%d", i, j);
            }
        }
    }

    for (i = 0; i < NUM_PENTOMINOES; i++) {
        piece_columns[i] = create_column(matrix, 1, "%s", PENTOMINOES[i].name);
    }

    int pi;
    for (pi = 0; pi < NUM_PENTOMINOES; pi++) {
        Pentomino *p = &PENTOMINOES[pi];
        Header *piece_col = piece_columns[pi];
        int flip, rotation, i, j;
        for (flip = 0; flip < p->flips; flip++)
            for (rotation = 0; rotation < p->rotations; rotation++) {
                int maxi = board_size - ((rotation & 1) ? p->cols : p->rows);
                int maxj = board_size - ((rotation & 1) ? p->rows : p->cols);
                for (i = 0; i <= maxi; i++)
                    for (j = 0; j <= maxj; j++) {
                        Header *positions[PENTOMINO_LENGTH];
                        if (!arrange_pentomino(p, i, j, flip, rotation, board_size, board_size, square_columns, positions))
                            continue;

                        Node *node = NULL;
                        int k;
                        for (k = 0; k < PENTOMINO_LENGTH; k++) {
                            node = allocate_node(matrix, node, positions[k]);
                        }
                        node = allocate_node(matrix, node, piece_col);

                        {
                            printf("%d, %d, %d, %d,     ", i, j, flip, rotation);
                            print_row(node);
                            printf("\n");
                        }
                    }
            }
    }

    return matrix;
}


int main(int argc, char *argv[]) {
    //Matrix *matrix = create_queens_problem(8);
    Matrix *matrix = create_pentominoes_problem();

    print_matrix(matrix);

    Node *solution[100];
    search(matrix, 0, solution);

    destroy_matrix(matrix);
    printf("Search calls: %ld\n", search_calls);
    return 0;
}
