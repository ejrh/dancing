set(srcs
        basic.c
        dancing.c
        dancing_threads.c
)

add_library(dancing ${srcs})
target_compile_options(dancing PUBLIC ${gen_opts})
