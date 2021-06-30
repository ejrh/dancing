add_executable(matrix_tests matrix_tests.c)
target_link_libraries(matrix_tests dancing ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_test(matrix_tests matrix_tests)
