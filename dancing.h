#pragma once

#ifndef DANCING_H
#define DANCING_H

#include "extarray.h"


#define INDEX_NODES 1

#if INDEX_NODES
    typedef unsigned int NodeId;

    #define NODE(id) (matrix->nodes.data[id])
    #define HEADER(id) (matrix->headers.data[id])
    #define ROOT 0
#else
    struct Header;

    typedef struct Node *NodeId;

    #define NODE(id) (*id)
    #define HEADER(id) (*(Header *) id)
    #define ROOT ((Node *) &matrix->headers.data[0])
#endif

typedef struct Node {
    NodeId up, down;
    NodeId left, right;
    NodeId column;
} Node;

typedef struct Header {
#if INDEX_NODES == 0
    Node node;
#endif
    char *name;
    int index;
    int size;
    int primary;
} Header;

struct Matrix;

typedef int (*Callback)(struct Matrix *matrix, void *baton);

typedef struct Matrix {
    EXTARRAY(Node) nodes;
    EXTARRAY(Header) headers;

    int num_columns;
    int num_rows;
    int num_nodes;

    EXTARRAY(NodeId) solution;

    Callback solution_callback;
    void *solution_baton;

    /* Statistics. */
    long int num_solutions;
    long int search_calls;
} Matrix;


#define foreachlink(h,a,x) for (x = NODE(h).a; x != (h); x = NODE(x).a)


extern Matrix *create_matrix();
extern NodeId create_column(Matrix *matrix, int primary, char *fmt, ...);
extern NodeId create_node(Matrix *matrix, NodeId after, NodeId column);
extern void destroy_matrix(Matrix *matrix);
extern void print_matrix(Matrix *matrix);
extern void print_row(Matrix *matrix, NodeId row);
extern void print_solution(Matrix *matrix);
extern int search_matrix(Matrix *matrix, Callback solution_callback, void *baton);


#endif
