#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "unity.h"
#include "zarya_vm.h"

// Глобальные переменные для тестов
extern vm_state_t vm;
extern int last_interrupt_num;

// Обработчик прерываний для тестов
vm_error_t test_handler(void* context, int interrupt_num);

// Функции инициализации и очистки для Unity
void setUp(void);
void tearDown(void);

// Режимы вывода тестов
typedef enum {
    TEST_OUTPUT_NORMAL,  // Обычный вывод
    TEST_OUTPUT_COMPACT  // Компактный вывод для AI
} test_output_mode_t;

// Текущий режим вывода определяется при компиляции
#ifdef TEST_OUTPUT_COMPACT
#define CURRENT_TEST_MODE TEST_OUTPUT_COMPACT
#else
#define CURRENT_TEST_MODE TEST_OUTPUT_NORMAL
#endif

// Функции для вывода в разных форматах
void test_print_module_start(const char* module_name);
void test_print_module_end(const char* module_name);
void test_print_result(const char* test_name, bool passed, const char* message);

// Функции для тестирования ТРИАС
void test_trias(void);
void test_addressing_modes(void);
void test_labels(void);
void test_directives(void);
void test_programs(void);

#endif // TEST_COMMON_H 