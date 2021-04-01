#pragma once

#ifndef DANCING_H
#define DANCING_H


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

    /* Statistics. */
    long int num_solutions;
    long int search_calls;
} Matrix;

typedef int (*Callback)(Matrix *matrix, Node **solution, int solution_size);


#define foreachlink(h,a,x) for (x = (h)->a; x != (h); x = x->a)


extern Matrix *create_matrix(int max_nodes, int max_headers);
extern Header *create_column(Matrix *matrix, int primary, char *fmt, ...);
extern Node *create_node(Matrix *matrix, Node *after, Header *column);
extern void destroy_matrix(Matrix *matrix);
extern void print_matrix(Matrix *matrix);
extern void print_solution(Node **solution, int solution_size);
extern int search_matrix(Matrix *matrix, Callback solution_callback);


#endif
