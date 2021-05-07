#pragma once

#ifndef BASIC_H
#define BASIC_H

#include "dancing.h"


typedef struct {
    int num_threads;         /* -j N */
    int thread_depth;        /* -d N */
    int problem_size;        /* -n N */
    int print_matrix;        /* -p */
    int print_stats;         /* -z */
    int print_solution;      /* -s */
    char *input_filename;    /* -f FILENAME */
} Options;

typedef struct {
    Matrix *matrix;
} Problem;


typedef Problem *CreateProblem(Options *options);
typedef void DestroyProblem(Problem *problem);


extern int basic_main(int argc, char *argv[], CreateProblem create_problem, DestroyProblem destroy_problem);


#endif
