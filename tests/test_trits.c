#include "unity.h"
#include "types.h"

void setUp(void) {
    // Код инициализации перед каждым тестом
}

void tearDown(void) {
    // Код очистки после каждого теста
}

void test_trit_validation(void) {
    TEST_ASSERT_TRUE(is_valid_trit(-1));
    TEST_ASSERT_TRUE(is_valid_trit(0));
    TEST_ASSERT_TRUE(is_valid_trit(1));
    TEST_ASSERT_FALSE(is_valid_trit(-2));
    TEST_ASSERT_FALSE(is_valid_trit(2));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_trit_validation);
    return UNITY_END();
} 