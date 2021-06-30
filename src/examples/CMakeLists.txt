function(add_example name srcfile)
    add_executable(${name} ${srcfile})
    target_compile_options(${name} PUBLIC ${gen_opts})
    target_link_libraries(${name} dancing ${CMAKE_THREAD_LIBS_INIT})
endfunction()

add_example(pentominoes pentominoes.c)
add_example(queens queens.c)
add_example(sudoku sudoku.c)
