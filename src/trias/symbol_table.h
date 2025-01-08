#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include "types.h"
#include "errors.h"  // Для vm_error_t
#include "lexer.h"  // Для source_loc_t

// Тип символа
typedef enum {
    SYMBOL_UNDEFINED,    // Неопределённый символ (используется до определения)
    SYMBOL_LABEL,       // Метка (адрес в памяти)
    SYMBOL_CONSTANT,    // Константа (значение)
    SYMBOL_MACRO,       // Имя макроса
} symbol_type_t;

// Флаги символа
typedef enum {
    SYMBOL_FLAG_NONE     = 0,
    SYMBOL_FLAG_DEFINED  = 1 << 0,  // Символ определён
    SYMBOL_FLAG_GLOBAL   = 1 << 1,  // Глобальный символ
    SYMBOL_FLAG_EXPORT   = 1 << 2,  // Экспортируемый символ
    SYMBOL_FLAG_IMPORT   = 1 << 3,  // Импортируемый символ
    SYMBOL_FLAG_LOCAL    = 1 << 4,  // Локальный символ (для макросов)
} symbol_flags_t;

// Структура символа
typedef struct {
    char* name;              // Имя символа
    symbol_type_t type;      // Тип символа
    tryte_t value;          // Значение (адрес или константа)
    symbol_flags_t flags;    // Флаги
    source_loc_t pos;        // Позиция в исходном коде
    struct scope* scope;     // Область видимости
} symbol_t;

// Узел хэш-таблицы для символов
typedef struct symbol_node {
    symbol_t symbol;
    struct symbol_node* next;
} symbol_node_t;

// Область видимости
typedef struct scope {
    char* name;              // Имя области (для отладки)
    struct scope* parent;    // Родительская область
    symbol_node_t* buckets[256];  // Хэш-таблица символов
} scope_t;

// Таблица символов
typedef struct symbol_table {
    scope_t* current_scope;  // Текущая область видимости
    scope_t* global_scope;   // Глобальная область видимости
} symbol_table_t;

// Создание таблицы символов
symbol_table_t* symbol_table_create(void);

// Уничтожение таблицы символов
void symbol_table_destroy(symbol_table_t* table);

// Очистка таблицы символов
void symbol_table_clear(symbol_table_t* table);

// Создание новой области видимости
scope_t* symbol_table_push_scope(symbol_table_t* table, const char* name);

// Возврат к родительской области видимости
void symbol_table_pop_scope(symbol_table_t* table);

// Добавление символа
vm_error_t symbol_table_add(symbol_table_t* table, const char* name, 
                           symbol_type_t type, tryte_t value,
                           symbol_flags_t flags, source_loc_t pos);

// Поиск символа (в текущей и родительских областях)
symbol_t* symbol_table_lookup(symbol_table_t* table, const char* name);

// Поиск символа только в текущей области
symbol_t* symbol_table_lookup_local(symbol_table_t* table, const char* name);

// Определение значения символа
vm_error_t symbol_table_define(symbol_table_t* table, const char* name,
                              tryte_t value, symbol_flags_t flags);

// Итерация по символам (для отладки и кодогенерации)
typedef void (*symbol_visitor_t)(symbol_t* symbol, void* user_data);
void symbol_table_foreach(symbol_table_t* table, symbol_visitor_t visitor, void* user_data);

// Получение текущей области видимости
scope_t* symbol_table_get_current_scope(symbol_table_t* table);

#endif // SYMBOL_TABLE_H 