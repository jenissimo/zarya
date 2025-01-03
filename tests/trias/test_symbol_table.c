#include "unity.h"
#include "trias/symbol_table.h"
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
void test_symbol_table_basic(void) {
    symbol_table_t* table = symbol_table_create();
    TEST_ASSERT_NOT_NULL(table);
    
    // Добавляем символ
    tryte_t value = create_tryte_from_int(42);
    source_loc_t pos = {1, 1};  // Строка 1, колонка 1
    
    vm_error_t err = symbol_table_add(table, "test", SYMBOL_CONSTANT, value,
                                    SYMBOL_FLAG_DEFINED, pos);
    TEST_ASSERT_EQUAL(VM_OK, err);
    
    // Проверяем поиск символа
    symbol_t* symbol = symbol_table_lookup(table, "test");
    TEST_ASSERT_NOT_NULL(symbol);
    TEST_ASSERT_EQUAL_STRING("test", symbol->name);
    TEST_ASSERT_EQUAL(SYMBOL_CONSTANT, symbol->type);
    TEST_ASSERT_EQUAL(42, symbol->value.value);
    TEST_ASSERT_EQUAL(SYMBOL_FLAG_DEFINED, symbol->flags);
    TEST_ASSERT_EQUAL(1, symbol->pos.line);
    TEST_ASSERT_EQUAL(1, symbol->pos.column);
    
    // Проверяем отсутствующий символ
    symbol = symbol_table_lookup(table, "nonexistent");
    TEST_ASSERT_NULL(symbol);
    
    symbol_table_destroy(table);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_symbol_table_basic);
    
    return UNITY_END();
} 