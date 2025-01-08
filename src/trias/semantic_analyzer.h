#ifndef TRIAS_SEMANTIC_ANALYZER_H
#define TRIAS_SEMANTIC_ANALYZER_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

// Тип семантического анализатора
typedef struct semantic_analyzer semantic_analyzer_t;

// Создание и уничтожение анализатора
semantic_analyzer_t* semantic_analyzer_create(void);
void semantic_analyzer_destroy(semantic_analyzer_t* analyzer);

// Установка зависимостей
void semantic_analyzer_set_symbol_table(semantic_analyzer_t* analyzer, symbol_table_t* table);

// Анализ программы
bool semantic_analyzer_check_program(semantic_analyzer_t* analyzer, ast_program_t* program);

// Получение информации об ошибках
const char* semantic_analyzer_get_error(const semantic_analyzer_t* analyzer);
source_loc_t semantic_analyzer_get_error_location(const semantic_analyzer_t* analyzer);

#endif // TRIAS_SEMANTIC_ANALYZER_H 