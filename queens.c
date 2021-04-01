#include <stdio.h>

#include "dancing.h"


static Matrix *create_queens_problem(int size) {
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
            Node *node = create_node(matrix, NULL, r_col);
            node = create_node(matrix, node, f_col);
            node = create_node(matrix, node, a_col);
            node = create_node(matrix, node, b_col);
        }
    }

    return matrix;
}


int main(int argc, char *argv[]) {
    Matrix *matrix = create_queens_problem(8);

    print_matrix(matrix);

    Node *solution[100];
    search_matrix(matrix, 0, solution);

    destroy_matrix(matrix);
    return 0;
}
