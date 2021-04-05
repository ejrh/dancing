#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dancing.h"


typedef struct {
    int size;
} SudokuProblem;


static Matrix *create_sudoku_problem(int size) {
    int block_size = (size == 4) ? 2 : (size == 9) ? 3 : 4;
    char *symbols = (size == 4) ? "abcd" : (size == 9) ? "123456789" : "0123456789abcdef";
    int num_symbols = size;
    Matrix *matrix = create_matrix();

    NodeId spot_headers[16][16];
    int i, j, k;
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            spot_headers[i][j] = create_column(matrix, 1, "X%d%d", i, j);
        }
    }
    NodeId row_headers[16][16];
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            row_headers[i][j] = create_column(matrix, 1, "R%d_%c", i, symbols[j]);
        }
    }
    NodeId column_headers[16][16];
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            column_headers[i][j] = create_column(matrix, 1, "C%d_%c", i, symbols[j]);
        }
    }
    NodeId block_headers[4][4][16];
    for (i = 0; i < block_size; i++) {
        for (j = 0; j < block_size; j++) {
            for (k = 0; k < size; k++) {
                block_headers[i][j][k] = create_column(matrix, 1, "B%d%d_%c", i, j, symbols[k]);
            }
        }
    }

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            for (k = 0; k < num_symbols; k++) {
                NodeId x_col = spot_headers[i][j];
                NodeId r_col = row_headers[i][k];
                NodeId c_col = column_headers[j][k];
                NodeId b_col = block_headers[i/block_size][j/block_size][k];
                NodeId node = create_node(matrix, 0, x_col);
                node = create_node(matrix, node, r_col);
                node = create_node(matrix, node, c_col);
                node = create_node(matrix, node, b_col);
            }
        }
    }

    return matrix;
}


static void decode_column(char *column_name, int *row, int *col, char *symbol) {
    if (column_name[0] == 'R')
        *row = atoi(&column_name[1]);
    else if (column_name[0] == 'C')
        *col = atoi(&column_name[1]);
    else
        return;

    *symbol = column_name[strlen(column_name) - 1];    
}


static int print_sudoku(Matrix *matrix, NodeId *solution, int solution_size, SudokuProblem *problem) {
    char *board = calloc(problem->size * problem->size, sizeof(int));

    int i;
    for (i = 0; i < solution_size; i++) {
        NodeId n;
        int row = -1, col = -1;
        char symbol = '.';

        decode_column(HEADER(NODE(solution[i]).column).name, &row, &col, &symbol);
        foreachlink(solution[i], right, n) {
            decode_column(HEADER(NODE(n).column).name, &row, &col, &symbol);
        }

        if (row == -1 || col == -1 || symbol == -1) {
            fprintf(stderr, "Warning, could not identify row, col and symbol\n");
            print_solution(matrix, solution, solution_size);
        } else {
            board[row * problem->size + col] = symbol;
        }

    }

    printf("Sudoku:\n");
    for (i = 0; i < problem->size; i++) {
        int j;
        for (j = 0; j < problem->size; j++) {
            printf(" %c", board[i * problem->size + j]);
        }
        printf("\n");
    }

    free(board);

    return 0;
}


int main(int argc, char *argv[]) {
    SudokuProblem problem;
    problem.size = 4;
    Matrix *matrix = create_sudoku_problem(problem.size);

    print_matrix(matrix);

    search_matrix(matrix, (Callback) print_sudoku, &problem);

    destroy_matrix(matrix);
    return 0;
}
