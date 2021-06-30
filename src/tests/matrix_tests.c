#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "dancing.h"


START_TEST(test_create_and_destroy)
{
    Matrix *matrix = create_matrix();

    ck_assert_int_eq(matrix->num_columns, 0);
    ck_assert_int_eq(matrix->num_rows, 0);
    ck_assert_int_eq(matrix->num_nodes, 0);

    destroy_matrix(matrix);
}
END_TEST

START_TEST(test_add_stuff)
{
    Matrix *matrix = create_matrix();

    NodeId a = create_column(matrix, 1, "A");
    NodeId b = create_column(matrix, 1, "B");
    NodeId c = create_column(matrix, 1, "C");

    NodeId x = create_node(matrix, 0, a);
    NodeId y = create_node(matrix, x, c);

    ck_assert_int_eq(matrix->num_columns, 3);
    ck_assert_int_eq(matrix->num_rows, 1);
    ck_assert_int_eq(matrix->num_nodes, 2);

    ck_assert_int_eq(NODE(a).down, x);
    ck_assert_int_eq(NODE(b).down, b);
    ck_assert_int_eq(NODE(c).down, y);

    ck_assert_int_eq(NODE(x).down, a);
    ck_assert_int_eq(NODE(y).down, c);

    ck_assert_int_eq(NODE(x).left, y);
    ck_assert_int_eq(NODE(x).right, y);
    ck_assert_int_eq(NODE(y).left, x);
    ck_assert_int_eq(NODE(y).right, x);

    destroy_matrix(matrix);
}
END_TEST

Suite *matrix_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Matrix");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_create_and_destroy);
    tcase_add_test(tc_core, test_add_stuff);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = matrix_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
