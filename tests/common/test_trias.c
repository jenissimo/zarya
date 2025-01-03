#include "unity.h"
#include "zarya_vm.h"
#include "zarya_config.h"

static vm_state_t vm;

// Инициализация перед каждым тестом
void setUp(void) {
    vm_init(&vm, MEMORY_SIZE_TRYTES);
    vm_reset(&vm);
}

// Очистка после каждого теста
void tearDown(void) {
    vm_free(&vm);
}

// Тесты
void test_trias_basic(void) {
    // Базовый тест ТРИАС
    TEST_ASSERT_TRUE(1);  // Заглушка
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_trias_basic);
    
    return UNITY_END();
} 