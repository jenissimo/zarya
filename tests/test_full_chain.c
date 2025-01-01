#include "unity.h"
#include "zarya_vm.h"
#include "emulator.h"
#include "zarya_config.h"
#include "trias/lexer.h"
#include "trias/parser.h"
#include "trias/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void setUp(void) {
}

void tearDown(void) {
}

void test_full_chain(void) {
    printf("\n=== Тест полной цепочки выполнения ===\n");
    
    // Создаем простую программу на ассемблере:
    // PUSH 5    ; Кладем 5 в стек
    // PUSH 3    ; Кладем 3 в стек
    // ADD       ; Складываем два верхних элемента
    // POP R1    ; Сохраняем результат в R1
    // HALT      ; Останавливаем выполнение
    const char* program = 
        "PUSH 5\n"
        "PUSH 3\n"
        "ADD\n"
        "POP R1\n"
        "HALT\n";
    
    printf("Компиляция программы...\n");
    
    // Компилируем программу
    codegen_t gen;
    TEST_ASSERT_EQUAL(VM_OK, codegen_init(&gen));
    
    // Транслируем исходный код в байт-код
    TEST_ASSERT_EQUAL(VM_OK, assemble_program(&gen, program));
    TEST_ASSERT_FALSE(codegen_had_error(&gen));
    
    // Создаем виртуальную машину
    vm_state_t vm;
    TEST_ASSERT_EQUAL(VM_OK, vm_init(&vm, MEMORY_SIZE_TRYTES));
    
    // Загружаем скомпилированную программу
    TEST_ASSERT_EQUAL(VM_OK, vm_load_program(&vm, (const tryte_t*)gen.code, gen.code_size));
    printf("Размер скомпилированного кода: %zu трайтов\n", gen.code_size);
    
    printf("Выполнение программы...\n");
    
    // Выполняем программу
    TEST_ASSERT_EQUAL(VM_OK, vm_run(&vm));
    
    // Проверяем результат
    printf("Значение в R1: %d (ожидается 8)\n", vm.registers[1].value);
    TEST_ASSERT_EQUAL(8, vm.registers[1].value);  // R1 должен содержать 8 (5 + 3)
    
    // Освобождаем ресурсы
    codegen_free(&gen);
    vm_free(&vm);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_full_chain);
    return UNITY_END();
} 