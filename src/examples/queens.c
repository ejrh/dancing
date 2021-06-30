#include <stdio.h>
#include <stdlib.h>

#include "basic.h"
#include "dancing.h"


typedef struct {
    Problem problem;
    int size;
} QueensProblem;


static void decode_column(char *column_name, int *rank, int *file) {
    if (column_name[0] == 'R')
        *rank = atoi(&column_name[1]);
    else if (column_name[0] == 'F')
        *file = atoi(&column_name[1]);
}


static int print_queens(Matrix *matrix, QueensProblem *problem) {
    int *board = calloc(problem->size * problem->size, sizeof(int));
    
    int i;
    for (i = 0; i < matrix->solution.num; i++) {
        NodeId n;
        int rank = -1, file = -1;

        decode_column(HEADER(NODE(matrix->solution.data[i]).column).name, &rank, &file);
        foreachlink(matrix->solution.data[i], right, n) {
            decode_column(HEADER(NODE(n).column).name, &rank, &file);
        }

        if (rank == -1 || file == -1) {
            fprintf(stderr, "Warning, could not identify rank and file\n");
            print_row(matrix, n);
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


static QueensProblem *create_queens_problem(Options *options) {
    QueensProblem *problem = malloc(sizeof(QueensProblem));
    memset(problem, 0, sizeof(QueensProblem));
    Matrix *matrix = problem->problem.matrix = create_matrix();
    int size = problem->size = options->problem_size;

    NodeId r_cols[size];
    NodeId f_cols[size];
    NodeId a_cols[2 * size - 1];
    NodeId b_cols[2 * size - 1];
    
    int i;
    for (i = 0; i < size; i++) {
        r_cols[i] = create_column(matrix, 1, "R%d", i);
    }
    for (i = 0; i < size; i++) {
        f_cols[i] = create_column(matrix, 1, "F%d", i);
    }
    for (i = 0; i < 2 * size - 1; i++) {
        a_cols[i] = create_column(matrix, 0, "A%d", i);
    }
    for (i = 0; i < 2 * size - 1; i++) {
        b_cols[i] = create_column(matrix, 0, "B%d", i);
    }

    for (i = 0; i < size; i++) {
        int j;
        for (j = 0; j < size; j++) {
            int a = i + j;
            int b = size - 1 - i + j;
            NodeId r_col = r_cols[i];
            NodeId f_col = f_cols[j];
            NodeId a_col = a_cols[a];
            NodeId b_col = b_cols[b];
            NodeId node = create_node(matrix, 0, r_col);
            node = create_node(matrix, node, f_col);
            node = create_node(matrix, node, a_col);
            node = create_node(matrix, node, b_col);
        }
    }

    matrix->solution_callback = (Callback) print_queens;
    matrix->solution_baton = problem;        

    return problem;
}


static void destroy_queens_problem(QueensProblem *problem) {
    destroy_matrix(problem->problem.matrix);
    free(problem);
}


int main(int argc, char *argv[]) {
    return basic_main(argc, argv, (CreateProblem *) create_queens_problem, (DestroyProblem *) destroy_queens_problem);
}
