#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dancing.h"


typedef struct {
    char *start_filename;
} SudokuOptions;


typedef struct {
    int size;
} SudokuProblem;


static void parse_command_line(int argc, char *argv[], SudokuOptions *options) {
    int i;
    for (i = 1; i < argc; i++) {
        if (*argv[i] == 'f')
            options->start_filename = argv[++i];
    }
}


static void prespecify_sudoku_cell(Matrix *matrix, int row_num, int column_num, char symbol) {
    NodeId cols[2];
    cols[0] = find_column(matrix, "R%d_%c", row_num, symbol);
    cols[1] = find_column(matrix, "C%d_%c", column_num, symbol);
    NodeId row = find_row(matrix, cols, 2);
    choose_row(matrix, row);
}


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

    prespecify_sudoku_cell(matrix, 0, 0, 'c');
    prespecify_sudoku_cell(matrix, 0, 1, 'd');

    return matrix;
}


static void read_start_file(char *filename, SudokuProblem *problem, Matrix *matrix) {
    char *board = calloc(problem->size * problem->size, sizeof(int));

    int i;
    for (i = 0; i < problem->size; i++) {
        int j;
        for (int j = 0; i < problem->size; i++) {
            int ch = fgetc(stdin);
        }
    }

    free(board);
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


static int print_sudoku(Matrix *matrix, SudokuProblem *problem) {
    char *board = calloc(problem->size * problem->size, sizeof(int));

    int i;
    for (i = 0; i < matrix->solution.num; i++) {
        NodeId n;
        int row = -1, col = -1;
        char symbol = '.';

        decode_column(HEADER(NODE(matrix->solution.data[i]).column).name, &row, &col, &symbol);
        foreachlink(matrix->solution.data[i], right, n) {
            decode_column(HEADER(NODE(n).column).name, &row, &col, &symbol);
        }

        if (row == -1 || col == -1 || symbol == -1) {
            fprintf(stderr, "Warning, could not identify row, col and symbol\n");
            print_row(matrix, n);
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
    SudokuOptions options = { NULL };
    parse_command_line(argc, argv, &options);

    SudokuProblem problem;
    problem.size = 4;
    Matrix *matrix = create_sudoku_problem(problem.size);

    if (options.start_filename != NULL) {
        read_start_file(options.start_filename, &problem, matrix);
    }

    print_matrix(matrix);
    
    search_matrix(matrix, (Callback) print_sudoku, &problem, 0);

    destroy_matrix(matrix);
    return 0;
}
