#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "logging.h"

// Подсистема для логирования
#define LOG_SUBSYS LOG_TRIAS

// Размер хэш-таблицы
#define HASH_SIZE 256

// Простая хэш-функция для строк
static unsigned int hash_string(const char* str) {
    unsigned int hash = 0;
    while (*str) {
        hash = hash * 31 + *str++;
    }
    return hash % HASH_SIZE;
}

// Создание новой области видимости
static scope_t* scope_create(const char* name, scope_t* parent) {
    scope_t* scope = (scope_t*)malloc(sizeof(scope_t));
    if (!scope) {
        return NULL;
    }
    
    scope->name = strdup(name);
    scope->parent = parent;
    memset(scope->buckets, 0, sizeof(scope->buckets));
    
    if (!scope->name) {
        free(scope);
        return NULL;
    }
    
    return scope;
}

// Уничтожение области видимости
static void scope_destroy(scope_t* scope) {
    if (!scope) {
        return;
    }
    
    // Освобождаем все символы в хэш-таблице
    for (int i = 0; i < HASH_SIZE; i++) {
        symbol_node_t* node = scope->buckets[i];
        while (node) {
            symbol_node_t* next = node->next;
            free(node->symbol.name);
            free(node);
            node = next;
        }
    }
    
    free(scope->name);
    free(scope);
}

// Создание таблицы символов
symbol_table_t* symbol_table_create(void) {
    symbol_table_t* table = (symbol_table_t*)malloc(sizeof(symbol_table_t));
    if (!table) {
        return NULL;
    }
    
    // Создание глобальной области видимости
    table->global_scope = scope_create("global", NULL);
    if (!table->global_scope) {
        free(table);
        return NULL;
    }
    
    // Установка текущей области как глобальной
    table->current_scope = table->global_scope;
    
    return table;
}

// Уничтожение таблицы символов
void symbol_table_destroy(symbol_table_t* table) {
    if (!table) {
        return;
    }
    
    // Освобождаем все области видимости
    scope_t* scope = table->current_scope;
    while (scope) {
        scope_t* parent = scope->parent;
        scope_destroy(scope);
        scope = parent;
    }
    
    free(table);
}

// Создание новой области видимости
scope_t* symbol_table_push_scope(symbol_table_t* table, const char* name) {
    if (!table || !name) {
        return NULL;
    }
    
    scope_t* scope = scope_create(name, table->current_scope);
    if (!scope) {
        return NULL;
    }
    
    table->current_scope = scope;
    return scope;
}

// Возврат к родительской области видимости
void symbol_table_pop_scope(symbol_table_t* table) {
    if (!table || !table->current_scope || !table->current_scope->parent) {
        return;  // Нельзя выйти из глобальной области
    }
    
    scope_t* old_scope = table->current_scope;
    table->current_scope = old_scope->parent;
    scope_destroy(old_scope);
}

// Добавление символа
vm_error_t symbol_table_add(symbol_table_t* table, const char* name,
                           symbol_type_t type, tryte_t value,
                           symbol_flags_t flags, source_loc_t pos) {
    if (!table || !name) {
        return VM_ERROR_INVALID_PARAMETER;
    }
    
    // Проверяем, не определён ли уже символ в текущей области
    if (symbol_table_lookup_local(table, name)) {
        LOG_ERROR(LOG_SUBSYS, "Symbol '%s' already defined at %zu:%zu",
                 name, pos.line, pos.column);
        return VM_ERROR_SYMBOL_ALREADY_DEFINED;
    }
    
    // Создаём новый узел
    symbol_node_t* node = (symbol_node_t*)malloc(sizeof(symbol_node_t));
    if (!node) {
        return VM_ERROR_OUT_OF_MEMORY;
    }
    
    // Инициализируем символ
    node->symbol.name = strdup(name);
    node->symbol.type = type;
    node->symbol.value = value;
    node->symbol.flags = flags;
    node->symbol.pos = pos;
    node->symbol.scope = table->current_scope;
    
    if (!node->symbol.name) {
        free(node);
        return VM_ERROR_OUT_OF_MEMORY;
    }
    
    // Добавляем в хэш-таблицу текущей области
    unsigned int hash = hash_string(name);
    node->next = table->current_scope->buckets[hash];
    table->current_scope->buckets[hash] = node;
    
    return VM_OK;
}

// Поиск символа в конкретной области видимости
static symbol_t* scope_lookup(scope_t* scope, const char* name) {
    unsigned int hash = hash_string(name);
    symbol_node_t* node = scope->buckets[hash];
    
    while (node) {
        if (strcmp(node->symbol.name, name) == 0) {
            return &node->symbol;
        }
        node = node->next;
    }
    
    return NULL;
}

// Поиск символа (в текущей и родительских областях)
symbol_t* symbol_table_lookup(symbol_table_t* table, const char* name) {
    if (!table || !name) {
        return NULL;
    }
    
    // Ищем в текущей и родительских областях
    scope_t* scope = table->current_scope;
    while (scope) {
        symbol_t* symbol = scope_lookup(scope, name);
        if (symbol) {
            return symbol;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

// Поиск символа только в текущей области
symbol_t* symbol_table_lookup_local(symbol_table_t* table, const char* name) {
    if (!table || !name) {
        return NULL;
    }
    
    return scope_lookup(table->current_scope, name);
}

// Определение значения символа
vm_error_t symbol_table_define(symbol_table_t* table, const char* name,
                              tryte_t value, symbol_flags_t flags) {
    if (!table || !name) {
        return VM_ERROR_INVALID_PARAMETER;
    }
    
    symbol_t* symbol = symbol_table_lookup(table, name);
    if (!symbol) {
        return VM_ERROR_SYMBOL_NOT_FOUND;
    }
    
    // Проверяем, не определён ли уже символ
    if (symbol->flags & SYMBOL_FLAG_DEFINED) {
        LOG_ERROR(LOG_SUBSYS, "Symbol '%s' already defined at %zu:%zu",
                 name, symbol->pos.line, symbol->pos.column);
        return VM_ERROR_SYMBOL_ALREADY_DEFINED;
    }
    
    symbol->value = value;
    symbol->flags |= flags | SYMBOL_FLAG_DEFINED;
    
    return VM_OK;
}

// Итерация по символам в области видимости
static void scope_foreach(scope_t* scope, symbol_visitor_t visitor, void* user_data) {
    for (int i = 0; i < HASH_SIZE; i++) {
        symbol_node_t* node = scope->buckets[i];
        while (node) {
            visitor(&node->symbol, user_data);
            node = node->next;
        }
    }
}

// Итерация по символам
void symbol_table_foreach(symbol_table_t* table, symbol_visitor_t visitor, void* user_data) {
    if (!table || !visitor) {
        return;
    }
    
    // Итерируемся по всем областям видимости
    scope_t* scope = table->current_scope;
    while (scope) {
        scope_foreach(scope, visitor, user_data);
        scope = scope->parent;
    }
} 