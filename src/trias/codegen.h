#ifndef TRIAS_CODEGEN_H
#define TRIAS_CODEGEN_H

#include "zarya_vm.h"
#include "ast.h"
#include "instructions.h"

// Максимальное количество меток
#define MAX_LABELS 1024

// Структура метки
typedef struct {
    char* name;    // Имя метки
    int address;   // Адрес метки
} label_t;

// Таблица меток
typedef struct {
    label_t labels[MAX_LABELS];
    size_t count;
} label_table_t;

// Структура генератора кода
typedef struct {
    tryte_t* code;           // Сгенерированный код
    size_t code_size;        // Размер кода в трайтах
    size_t code_capacity;    // Емкость буфера кода
    int current_address;     // Текущий адрес
    bool had_error;          // Флаг ошибки
    label_table_t labels;    // Таблица меток
} codegen_t;

// Инициализация генератора кода
vm_error_t codegen_init(codegen_t* gen);

// Освобождение ресурсов генератора кода
void codegen_free(codegen_t* gen);

// Эмиссия одной инструкции
vm_error_t codegen_emit(codegen_t* gen, const instruction_t* inst);

// Генерация кода для инструкции
vm_error_t generate_instruction(codegen_t* gen, ast_node_t* node);

// Генерация кода для программы
vm_error_t codegen_generate(codegen_t* gen, ast_node_t* program);

// Сборка программы из исходного текста
vm_error_t assemble_program(codegen_t* gen, const char* source);

// Получение сгенерированного кода
const word_t* codegen_get_code(const codegen_t* gen, size_t* size);

// Проверка наличия ошибок
bool codegen_had_error(const codegen_t* gen);

// Сообщение об ошибке
#define codegen_error(gen, msg) do { \
    fprintf(stderr, "Ошибка генерации кода: %s\n", msg); \
    (gen)->had_error = true; \
} while (0)

#endif // TRIAS_CODEGEN_H 