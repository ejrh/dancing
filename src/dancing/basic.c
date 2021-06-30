#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "basic.h"
#include "dancing.h"
#include "dancing_threads.h"


static void print_help() {
    fprintf(stderr, "Options:\n"
        "    -j N          Number of workers (default: no multithreading)\n"
        "    -d N          Depth at which to fork if multithreading (default: 3)\n"
        "    -n N          Problem size (problem-specific)\n"
        "    -p            Print matrix (default: no)\n"
        "    -z            Print statistics (default: no)\n"
        "    -s            Print solutions (default: no)\n"
        "    -f FILENAME   Initial problem file (default: no initial problem)\n");
    exit(1);
}


static void parse_command_line(int argc, char *argv[], Options *options) {
    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            printf("Skipping funny option %s\n", argv[i]);
            continue;
        }
        char symbol = argv[i][1];
        switch (symbol) {
            case 'j': {
                options->num_threads = atoi(argv[++i]);
            } break;

            case 'd': {
                options->thread_depth = atoi(argv[++i]);
            } break;

            case 'n': {
                options->problem_size = atoi(argv[++i]);
            } break;

            case 'p': {
                options->print_matrix = 1;
            } break;

            case 'z': {
                options->print_stats = 1;
            } break;

            case 's': {
                options->print_solution = 1;
            } break;

            case 'f': {
                options->input_filename = argv[++i];
            } break;

            case 'h': {
                print_help();
            } break;

            default: {
                printf("Skipping funny option %s\n", argv[i]);
                continue;
            } break;
        }
    }
}


static int quiet_callback(Matrix *matrix, void *baton) {
    return 0;
}


int basic_main(int argc, char *argv[], CreateProblem create_problem, DestroyProblem destroy_problem) {
    Options options;

    options.num_threads = 0;
    options.thread_depth = 3;
    options.problem_size = 1;
    options.print_matrix = 0;
    options.print_stats = 0;
    options.print_solution = 0;
    options.input_filename = NULL;

    parse_command_line(argc, argv, &options);

    Problem *problem = create_problem(&options);

    if (options.print_matrix) {
        print_matrix(problem->matrix);    
    }

    /* If we're not printing the solution, just replace the callback with a quiet one. */
    if (!options.print_solution) {
        problem->matrix->solution_callback = quiet_callback;
    }

    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    
    if (options.num_threads > 0) {
        search_with_threads(problem->matrix, options.thread_depth, options.num_threads);
    } else {
        search_matrix(problem->matrix, 0);
    }

    struct timespec stop_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time);

    double search_time = stop_time.tv_sec + stop_time.tv_nsec/1E+9
                       - start_time.tv_sec - start_time.tv_nsec/1E+9;

    if (options.print_stats) {
        fprintf(stderr, "Matrix size: %d columns, %d rows, %d nodes\n", problem->matrix->num_columns, problem->matrix->num_rows, problem->matrix->num_nodes);
        fprintf(stderr, "Search calls: %ld\n", problem->matrix->search_calls);
        fprintf(stderr, "Solutions found: %ld\n", problem->matrix->num_solutions);
        fprintf(stderr, "Search time: %0.3f seconds\n", search_time);
        if (options.num_threads > 0) {
            fprintf(stderr, "Messages: %ld\n", problem->matrix->num_messages);
            fprintf(stderr, "Subsearches: %ld\n", problem->matrix->num_subsearches);
        }
    }

    destroy_problem(problem);

    return 0;
}
