#pragma once

#ifndef DANCING_H
#define DANCING_H

#include "extarray.h"
#include "segarray.h"


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
    #define ROOT matrix->root
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
    #if INDEX_NODES
        EXTARRAY(Node) nodes;
        EXTARRAY(Header) headers;
    #else
        SEGARRAY(Node) nodes;
        SEGARRAY(Header) headers;
        NodeId root;
    #endif

    int num_columns;
    int num_rows;
    int num_nodes;

    EXTARRAY(NodeId) solution;

    Callback solution_callback;
    void *solution_baton;

    Callback depth_callback;
    void *depth_baton;

    /* Statistics. */
    long int num_solutions;
    long int search_calls;
} Matrix;


#define foreachlink(h,a,x) for (x = NODE(h).a; x != (h); x = NODE(x).a)


extern Matrix *create_matrix();
extern NodeId create_column(Matrix *matrix, int primary, char *fmt, ...);
extern NodeId create_node(Matrix *matrix, NodeId after, NodeId column);
extern void destroy_matrix(Matrix *matrix);
extern Matrix *clone_matrix(Matrix *matrix);
extern void print_matrix(Matrix *matrix);
extern void print_row(Matrix *matrix, NodeId row);
extern void print_solution(Matrix *matrix);
extern int search_matrix_internal(Matrix *matrix, int depth, int max_depth);
extern NodeId find_column(Matrix *matrix, char *fmt, ...);
extern NodeId find_row(Matrix *matrix, NodeId *columns, int num_columns);
extern void choose_row(Matrix *matrix, NodeId row);
extern int search_matrix(Matrix *matrix, Callback solution_callback, void *baton, int max_depth);

#endif
