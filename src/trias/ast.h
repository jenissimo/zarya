#ifndef TRIAS_AST_H
#define TRIAS_AST_H

#include "lexer.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Менеджер памяти AST
typedef struct ast_manager ast_manager_t;

// Типы узлов AST
typedef enum {
    AST_INVALID = 0,  // Недействительный узел
    AST_PROGRAM,      // Корневой узел программы
    AST_INSTRUCTION,  // Инструкция
    AST_LABEL,        // Метка
    AST_DIRECTIVE,    // Директива
    AST_OPERAND,      // Операнд
    AST_EXPRESSION,   // Выражение
    AST_MACRO,        // Макрос
} ast_node_type_t;

// Типы операндов
typedef enum {
    OPERAND_IMMEDIATE,  // Непосредственное значение (#42)
    OPERAND_REGISTER,   // Регистр (R0)
    OPERAND_INDIRECT,   // Косвенная адресация (@R1)
    OPERAND_LABEL      // Метка или символ
} operand_type_t;

// Базовая структура для всех узлов
typedef struct ast_node {
    ast_node_type_t type;      // Тип узла
    source_loc_t loc;          // Позиция в исходном коде
    bool is_owned;             // Флаг владения узлом
    struct {
        size_t strong_count;   // Сильные ссылки
        size_t weak_count;     // Слабые ссылки
    } refs;
    struct ast_node* parent;   // Родительский узел
    struct ast_node* next;     // Следующий узел в списке
    struct ast_manager* manager; // Указатель на менеджер AST
    void (*destroy)(struct ast_node*); // Функция уничтожения узла
#ifdef DEBUG
    const char* alloc_file;    // Файл, где был создан узел
    int alloc_line;           // Строка, где был создан узел
#endif
} ast_node_t;

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
} ast_operand_t;

// Инструкция
typedef struct {
    ast_node_t base;           // Базовый узел
    char* mnemonic;            // Мнемоника
    ast_operand_t** operands;  // Массив указателей на операнды
    size_t operand_count;      // Количество операндов
} ast_instruction_t;

// Метка
typedef struct {
    ast_node_t base;           // Базовый узел
    char* name;                // Имя метки
    bool is_local;             // Локальная метка (для макросов)
} ast_label_t;

// Директива
typedef struct {
    ast_node_t base;           // Базовый узел
    char* name;                // Имя директивы
    ast_node_t** args;         // Массив указателей на аргументы
    size_t arg_count;          // Количество аргументов
} ast_directive_t;

// Узел программы
typedef struct ast_program_t {
    ast_node_t base;           // Базовый узел
    ast_node_t** statements;   // Массив операторов
    size_t statement_count;    // Количество операторов
    size_t statement_capacity; // Емкость массива операторов
    ast_manager_t* manager;    // Указатель на менеджер AST
} ast_program_t;

// Создание и уничтожение менеджера AST
ast_manager_t* ast_manager_create(void);
void ast_manager_destroy(ast_manager_t* manager);

// Создание узлов (через менеджер)
ast_program_t* ast_create_program(ast_manager_t* manager);
ast_instruction_t* ast_create_instruction(ast_manager_t* manager, const char* mnemonic, const source_loc_t* loc);
ast_label_t* ast_create_label(ast_manager_t* manager, const char* name, bool is_local, const source_loc_t* loc);
ast_directive_t* ast_create_directive(ast_manager_t* manager, const char* name, const source_loc_t* loc);
ast_operand_t* ast_create_operand(ast_manager_t* manager, operand_type_t type, const source_loc_t* loc);

// Специализированные функции создания операндов
ast_operand_t* ast_create_immediate_operand(ast_manager_t* manager, token_value_t value);
ast_operand_t* ast_create_register_operand(ast_manager_t* manager, token_value_t value);
ast_operand_t* ast_create_indirect_operand(ast_manager_t* manager, token_value_t value);
ast_operand_t* ast_create_label_operand(ast_manager_t* manager, const char* label);

// Управление ссылками
void ast_ref(ast_node_t* node);
void ast_unref(ast_node_t* node);

// Добавление дочерних узлов
void ast_program_add_statement(ast_program_t* program, ast_node_t* statement);
void ast_instruction_add_operand(ast_instruction_t* inst, ast_operand_t* operand);
void ast_directive_add_arg(ast_directive_t* directive, ast_node_t* arg);

// Освобождение узлов (через менеджер)
void ast_free_node(ast_manager_t* manager, ast_node_t* node);

// Отладочный вывод
void ast_dump_node(const ast_node_t* node, int indent);

// Новые функции управления памятью
void ast_strong_ref(ast_node_t* node);
void ast_weak_ref(ast_node_t* node);
bool ast_is_valid(ast_node_t* node);

// Функции уничтожения узлов (объявления)
void ast_destroy_program(ast_node_t* node);
void ast_destroy_label(ast_node_t* node);
void ast_destroy_instruction(ast_node_t* node);
void ast_destroy_operand(ast_node_t* node);
void ast_destroy_directive(ast_node_t* node);

#ifdef DEBUG
    #define AST_TRACK_ALLOC(node) \
        node->alloc_file = __FILE__; \
        node->alloc_line = __LINE__;
#else
    #define AST_TRACK_ALLOC(node)
#endif

#endif // TRIAS_AST_H