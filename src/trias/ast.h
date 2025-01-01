#ifndef TRIAS_AST_H
#define TRIAS_AST_H

#include <stddef.h>
#include <stdbool.h>

// Типы узлов AST
typedef enum {
    NODE_PROGRAM,     // Программа
    NODE_INSTRUCTION, // Инструкция
    NODE_LABEL,      // Метка
    NODE_DIRECTIVE,  // Директива
    NODE_NUMBER,     // Число
    NODE_STRING,     // Строка
    NODE_IDENTIFIER, // Идентификатор
    NODE_REGISTER,   // Регистр
    NODE_CHAR        // Символ
} node_type_t;

// Режимы адресации
typedef enum {
    ADDR_MODE_IMMEDIATE,  // Непосредственный (#value)
    ADDR_MODE_DIRECT,     // Прямой (value)
    ADDR_MODE_INDIRECT,   // Косвенный (@value)
    ADDR_MODE_REGISTER    // Регистровый (Rn)
} addr_mode_t;

// Структура для хранения информации о метке
typedef struct label_info {
    char* name;     // Имя метки
    int address;    // Адрес метки
    int line;       // Номер строки определения
    struct label_info* next;  // Следующая метка в списке
} label_info_t;

// Структура для хранения информации об инструкции в AST
typedef struct {
    int opcode;                // Код операции
    char* name;               // Имя инструкции
    struct ast_node* operands[2];  // Операнды (максимум 2)
} instruction_node_info_t;

// Структура для хранения информации о директиве
typedef struct {
    int directive_type;  // Тип директивы
    union {
        int address;     // Адрес для ORG
        int value;       // Значение для DB/DW
        char* string;    // Строка для DS
    } value;
} directive_info_t;

// Структура для хранения строковых данных
typedef struct {
    char* text;  // Текст (строка с нулевым окончанием)
} string_value_t;

// Узел AST
typedef struct ast_node {
    node_type_t type;    // Тип узла
    int line;           // Номер строки
    addr_mode_t addr_mode; // Режим адресации для операндов
    union {
        instruction_node_info_t instruction;  // Информация об инструкции
        string_value_t label;                // Информация о метке (имя)
        directive_info_t directive;          // Информация о директиве
        int number;                          // Числовое значение
        string_value_t string;               // Строковое значение
        string_value_t identifier;           // Идентификатор
    } value;
    struct ast_node* next;  // Следующий узел
} ast_node_t;

// Создание нового узла AST
ast_node_t* create_ast_node(node_type_t type);

// Добавление узла в конец списка
void append_ast_node(ast_node_t** head, ast_node_t* node);

// Освобождение памяти, занятой узлом AST
void free_ast(ast_node_t* node);

#endif // TRIAS_AST_H 