#ifndef TRIAS_PARSER_H
#define TRIAS_PARSER_H

#include <stdlib.h>
#include "lexer.h"
#include "ast.h"
#include "trias_instructions.h"

#define MAX_LABEL_LENGTH 256

// Таблица символов
typedef struct {
    label_info_t* labels;    // Список меток
    int label_count;         // Количество меток
    int current_address;     // Текущий адрес для генерации кода
} symbol_table_t;

// Парсер
typedef struct {
    lexer_t* lexer;          // Лексический анализатор
    token_t current;         // Текущий токен
    token_t previous;        // Предыдущий токен
    bool had_error;          // Флаг наличия ошибки
    bool panic_mode;         // Режим паники
    const char* error_message; // Сообщение об ошибке
    symbol_table_t symbols;  // Таблица символов
} parser_t;

// Публичный API

// Инициализация и освобождение парсера
void parser_init(parser_t* parser, lexer_t* lexer);
void parser_free(parser_t* parser);

// Основная функция разбора
ast_node_t* parse_program(parser_t* parser);

// Проверка состояния
bool parser_had_error(const parser_t* parser);

#endif // TRIAS_PARSER_H 