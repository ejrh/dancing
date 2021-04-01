#include <stdio.h>
#include <stdlib.h>

#include "dancing.h"


typedef struct {
    int size;
} QueensProblem;


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


static void decode_column(Header *column, int *rank, int *file) {
    if (column->name[0] == 'R')
        *rank = atoi(&column->name[1]);
    else if (column->name[0] == 'F')
        *file = atoi(&column->name[1]);
}


static int print_queens(Matrix *matrix, Node **solution, int solution_size, QueensProblem *problem) {
    int *board = calloc(problem->size * problem->size, sizeof(int));

    int i;
    for (i = 0; i < solution_size; i++) {
        Node *n;
        int rank = -1, file = -1;

        decode_column(solution[i]->column, &rank, &file);
        foreachlink(solution[i], right, n) {
            decode_column(n->column, &rank, &file);
        }

        if (rank == -1 || file == -1) {
            fprintf(stderr, "Warning, could not identify rank and file\n");
            print_solution(solution, solution_size);
        } else {
            board[rank * problem->size + file] = 1;
        }
    }

    printf("Queens:\n");
    for (i = 0; i < problem->size; i++) {
        int j;
        for (j = 0; j < problem->size; j++) {
            printf(" %d", board[i * problem->size + j]);
        }
        printf("\n");
    }

    free(board);

    return 0;
}


int main(int argc, char *argv[]) {
    QueensProblem problem;
    problem.size = 8;
    Matrix *matrix = create_queens_problem(problem.size);

    //print_matrix(matrix);

    search_matrix(matrix, (Callback) print_queens, &problem);

    destroy_matrix(matrix);
    return 0;
}
