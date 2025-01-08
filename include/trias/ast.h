#ifndef TRIAS_AST_H
#define TRIAS_AST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Предварительные объявления типов
typedef struct ast_node ast_node_t;
typedef struct ast_manager ast_manager_t;
typedef struct ast_directive ast_directive_t;

// Тип операнда
typedef enum {
    OPERAND_IMMEDIATE,  // Непосредственное значение
    OPERAND_REGISTER,   // Регистр
    OPERAND_LABEL,      // Метка
    OPERAND_INDIRECT    // Косвенная адресация
} operand_type_t;

// Области видимости
typedef enum {
    SCOPE_GLOBAL,    // Глобальная область видимости
    SCOPE_LOCAL,     // Локальная область видимости
    SCOPE_MODULE     // Область видимости модуля
} scope_type_t;

// Метка
typedef struct {
    ast_node_t base;           // Базовый узел
    char* name;                // Имя метки
    bool is_local;             // Локальная метка (для макросов)
    int64_t address;          // Адрес метки
    scope_type_t scope;       // Область видимости
    bool is_exported;         // Флаг экспорта метки
} ast_label_t;

// Инструкция
typedef struct {
    ast_node_t base;           // Базовый узел
    char* mnemonic;            // Мнемоника
    ast_operand_t** operands;  // Массив указателей на операнды
    size_t operand_count;      // Количество операндов
    int64_t address;          // Адрес инструкции
    bool is_relative_jump;    // Флаг относительного перехода
} ast_instruction_t;

// Операнд
typedef struct {
    ast_node_t base;           // Базовый узел
    operand_type_t type;       // Тип операнда
    union {
        int64_t immediate;     // Непосредственное значение
        int register_num;      // Номер регистра
        char* label;          // Имя метки
    };
    bool is_indirect;          // Флаг косвенной адресации
    char* source_text;         // Исходный текст числа (для троичных чисел)
    scope_type_t scope;       // Область видимости (для меток)
} ast_operand_t;

// Узел программы
typedef struct ast_program_t {
    ast_node_t base;           // Базовый узел
    ast_node_t** statements;   // Массив операторов
    size_t statement_count;    // Количество операторов
    size_t statement_capacity; // Емкость массива операторов
    ast_manager_t* manager;    // Указатель на менеджер AST
    
    // Специализированные массивы для быстрого доступа
    ast_label_t** labels;      // Массив меток
    size_t label_count;        // Количество меток
    ast_instruction_t** instructions;  // Массив инструкций
    size_t instruction_count;  // Количество инструкций
    ast_directive_t** directives;      // Массив директив
    size_t directive_count;    // Количество директив
} ast_program_t;

#endif // TRIAS_AST_H 