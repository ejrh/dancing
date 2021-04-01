#include <stdio.h>

#include "dancing.h"


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


static int arrange_pentomino(Pentomino *p, int i, int j, int flip, int rotation, int board_rows, int board_cols, Header *square_columns[8][8], Header **positions) {
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

static Matrix *create_pentominoes_problem() {
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


int found_solution(Matrix *matrix, Node **solution, int solution_size) {
    print_solution(solution, solution_size);
    return 0;
}


int main(int argc, char *argv[]) {
    Matrix *matrix = create_pentominoes_problem();

    print_matrix(matrix);

    search_matrix(matrix, found_solution);

    destroy_matrix(matrix);
    return 0;
}
