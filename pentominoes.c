#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dancing.h"


#define BOARD_SIZE 8

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


static int arrange_pentomino(Pentomino *p, int i, int j, int flip, int rotation, int board_rows, int board_cols, NodeId square_columns[8][8], NodeId *positions) {
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

        NodeId pos = square_columns[r][c];
        if (pos == 0)
            return 0;

        positions[k] = pos;
    }

    return 1;
}


static Matrix *create_pentominoes_problem() {
    int num_squares = BOARD_SIZE * BOARD_SIZE - 2*2;
    int num_cols = NUM_PENTOMINOES + num_squares;
    int num_rows = 0;
    int i;
    for (i = 0; i < NUM_PENTOMINOES; i++) {
        Pentomino *p = &PENTOMINOES[i];
        num_rows += p->flips * p->rotations * (BOARD_SIZE + 1 - p->rows) * (BOARD_SIZE + 1 - p->cols);
    }
    int num_nodes = (PENTOMINO_LENGTH + 1) * num_rows;
    Matrix *matrix = create_matrix(num_nodes, num_cols);

    NodeId square_columns[BOARD_SIZE][BOARD_SIZE];
    NodeId piece_columns[NUM_PENTOMINOES];

    for (i = 0; i < BOARD_SIZE; i++) {
        int j;
        for (j = 0; j < BOARD_SIZE; j++) {
            if (i >= 3 && i <= 4 && j >= 3 && j <= 4) {
                square_columns[i][j] = 0;
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
        NodeId piece_col = piece_columns[pi];
        int flip, rotation, i, j;
        for (flip = 0; flip < p->flips; flip++)
            for (rotation = 0; rotation < p->rotations; rotation++) {
                int maxi = BOARD_SIZE - ((rotation & 1) ? p->cols : p->rows);
                int maxj = BOARD_SIZE - ((rotation & 1) ? p->rows : p->cols);
                for (i = 0; i <= maxi; i++)
                    for (j = 0; j <= maxj; j++) {
                        NodeId positions[PENTOMINO_LENGTH];
                        if (!arrange_pentomino(p, i, j, flip, rotation, BOARD_SIZE, BOARD_SIZE, square_columns, positions))
                            continue;

                        NodeId node = 0;
                        int k;
                        for (k = 0; k < PENTOMINO_LENGTH; k++) {
                            node = create_node(matrix, node, positions[k]);
                        }
                        node = create_node(matrix, node, piece_col);

                        // {
                        //     printf("%d, %d, %d, %d,     ", i, j, flip, rotation);
                        //     print_row(node);
                        //     printf("\n");
                        // }
                    }
            }
    }

    return matrix;
}


static void decode_column(char *column_name, char **name, int *r, int *c, int *num_cells) {
    if (isalpha((int) column_name[0]))
        *name = column_name;
    else {
        r[*num_cells] = column_name[0] - 48;
        c[*num_cells] = column_name[1] - 48;
        (*num_cells)++;
    }
}


static int print_pentominoes(Matrix *matrix, void *problem) {
    int board[BOARD_SIZE][BOARD_SIZE];
    memset(board, 0, sizeof(board));
    board[3][3] = ' ';
    board[3][4] = ' ';
    board[4][3] = ' ';
    board[4][4] = ' ';

    int i;
    for (i = 0; i < matrix->solution.num; i++) {
        char *name = NULL;
        int r[PENTOMINO_LENGTH];
        int c[PENTOMINO_LENGTH];
        int num_cells = 0;

        decode_column(HEADER(NODE(matrix->solution.data[i]).column).name, &name, r, c, &num_cells);
        NodeId n;
        foreachlink(matrix->solution.data[i], right, n) {
            decode_column(HEADER(NODE(n).column).name, &name, r, c, &num_cells);
        }

        if (name == NULL || num_cells != PENTOMINO_LENGTH) {
            fprintf(stderr, "Warning, could not identify name, r and c\n");
            print_row(matrix, n);
        } else {
            int j;
            for (j = 0; j < num_cells; j++)
                board[r[j]][c[j]] = name[0];
        }
    }

    printf("Pentominoes:\n");
    for (i = 0; i < BOARD_SIZE; i++) {
        int j;
        for (j = 0; j < BOARD_SIZE; j++) {
            printf(" %c", board[i][j]);
        }
        printf("\n");
    }

    return 0;
}


int main(int argc, char *argv[]) {
    Matrix *matrix = create_pentominoes_problem();

    //print_matrix(matrix);

    search_matrix(matrix, print_pentominoes, NULL);

    destroy_matrix(matrix);
    return 0;
}
