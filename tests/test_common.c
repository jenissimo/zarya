#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "test_common.h"

// Глобальные переменные
vm_state_t vm;
int last_interrupt_num;

// Обработчик прерываний
vm_error_t test_handler(void* context, int interrupt_num) {
    (void)context;  // Подавляем предупреждение о неиспользуемом параметре
    last_interrupt_num = interrupt_num;
    return VM_OK;
}

void setUp(void) {
    // Инициализируем виртуальную машину
    vm_error_t err = vm_init(&vm, MEMORY_SIZE_TRYTES);
    if (err != VM_OK) {
        printf("ERROR: Failed to initialize VM: %d\n", err);
        return;
    }
    printf("DEBUG: setUp: memory_size=%zu, sp=%d\n", vm.memory_size, vm.sp.value);
    
    // Устанавливаем обработчик прерываний
    vm_set_interrupt_handler(&vm, test_handler, NULL);
}

void tearDown(void) {
    // Освобождаем ресурсы виртуальной машины
    vm_free(&vm);
}

// Функции вывода
void test_print_module_start(const char* module_name) {
    if (CURRENT_TEST_MODE == TEST_OUTPUT_NORMAL) {
        printf("\n=== Тесты модуля %s ===\n", module_name);
    } else {
        printf("@MODULE:%s\n", module_name);
    }
}

void test_print_module_end(const char* module_name) {
    if (CURRENT_TEST_MODE == TEST_OUTPUT_NORMAL) {
        printf("=== Конец тестов модуля %s ===\n", module_name);
    }
}

void test_print_result(const char* test_name, bool passed, const char* message) {
    if (CURRENT_TEST_MODE == TEST_OUTPUT_NORMAL) {
        printf("%s: %s\n", test_name, passed ? "OK" : message);
    } else {
        printf("%c%s:%s%s%s\n",
            passed ? '+' : '-',
            test_name,
            passed ? "OK" : "FAIL",
            message && !passed ? "[" : "",
            message && !passed ? message : "");
    }
}

// Функции для тестирования ТРИАС
void test_trias(void) {
    test_print_module_start("ТРИАС");
    
    test_addressing_modes();
    test_labels();
    test_directives();
    test_programs();
    
    test_print_module_end("ТРИАС");
} 