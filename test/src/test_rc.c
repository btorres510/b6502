#include "b6502/rc.h"
#include "unity.h"
#include "unity_fixture.h"

TEST_GROUP(RC);

static int* num = NULL;

TEST_SETUP(RC) {
  num = rc_alloc(sizeof(*num), NULL);
  *num = 1;
}

TEST_TEAR_DOWN(RC) {
  if (num) {
    rc_strong_release((void*)&num);
  }
}

TEST(RC, test_rc_strong) {
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);

  int* num2 = rc_strong_retain(num);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 2);

  rc_strong_release((void*)&num2);
  TEST_ASSERT_NULL(num2);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);

  rc_strong_release((void*)&num);
  TEST_ASSERT_NULL(num);
}

TEST(RC, test_rc_weak) {
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);

  int* num2 = rc_weak_retain(num);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 1);

  int* num3 = rc_weak_retain(num);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 2);

  rc_weak_release((void*)&num3);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 1);
  TEST_ASSERT_NULL(num3);

  rc_strong_release((void*)&num);
  TEST_ASSERT_NULL(num);

  num2 = rc_weak_check((void*)&num2);
  TEST_ASSERT_NULL(num2);
}

TEST(RC, test_rc_downcast) {
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);

  int* num2 = rc_strong_retain(num);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 2);

  num = rc_downcast((void*)&num);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 1);

  num2 = rc_downcast((void*)&num2);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 0);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 1);
  TEST_ASSERT_NULL(num2);

  num = rc_weak_check((void*)&num);
  TEST_ASSERT_NULL(num);
}

TEST(RC, test_rc_upgrade) {
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);

  int* num2 = rc_weak_retain(num);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 1);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 1);

  num2 = rc_upgrade((void*)&num2);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 2);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 0);

  int* num3 = rc_weak_retain(num);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(num), 2);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(num), 1);

  rc_strong_release((void*)&num2);
  TEST_ASSERT_NULL(num2);
  rc_strong_release((void*)&num);
  TEST_ASSERT_NULL(num);

  num3 = rc_upgrade((void*)&num3);
  TEST_ASSERT_NULL(num3);
}

TEST(RC, test_rc_array) {
  int* array = rc_alloc(5 * sizeof(*array), NULL);
  TEST_ASSERT_EQUAL_INT(rc_strong_count(array), 1);
  TEST_ASSERT_EQUAL_INT(rc_weak_count(array), 0);
  rc_strong_release((void*)&array);
  TEST_ASSERT_NULL(array);
}

TEST_GROUP_RUNNER(RC) {
  RUN_TEST_CASE(RC, test_rc_strong)
  RUN_TEST_CASE(RC, test_rc_weak)
  RUN_TEST_CASE(RC, test_rc_upgrade)
  RUN_TEST_CASE(RC, test_rc_downcast)
  RUN_TEST_CASE(RC, test_rc_array)
}
