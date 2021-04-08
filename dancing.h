#pragma once

#ifndef DANCING_H
#define DANCING_H


#define INDEX_NODES 1

#if INDEX_NODES
    typedef unsigned int NodeId;

    #define NODE(id) (matrix->nodes[id])
    #define HEADER(id) (matrix->headers[id])
    #define ROOT 0
#else
    struct Header;

    typedef struct Node *NodeId;

    #define NODE(id) (*id)
    #define HEADER(id) (*(Header *) id)
    #define ROOT ((Node *) &matrix->headers[0])
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

typedef struct Matrix {
    int num_nodes;
    int max_nodes;
    Node *nodes;
    int num_headers;
    int max_headers;
    Header *headers;
    int num_rows;

    /* Statistics. */
    long int num_solutions;
    long int search_calls;
} Matrix;

typedef int (*Callback)(Matrix *matrix, NodeId *solution, int solution_size, void *baton);


#define foreachlink(h,a,x) for (x = NODE(h).a; x != (h); x = NODE(x).a)


extern Matrix *create_matrix();
extern NodeId create_column(Matrix *matrix, int primary, char *fmt, ...);
extern NodeId create_node(Matrix *matrix, NodeId after, NodeId column);
extern void destroy_matrix(Matrix *matrix);
extern void print_matrix(Matrix *matrix);
extern void print_row(Matrix *matrix, NodeId row);
extern void print_solution(Matrix *matrix, NodeId *solution, int solution_size);
extern int search_matrix(Matrix *matrix, Callback solution_callback, void *baton);


#endif
