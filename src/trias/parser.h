#ifndef TRIAS_PARSER_H
#define TRIAS_PARSER_H

#include "lexer.h"
#include "ast.h"
#include "symbol_table.h"

// Тип парсера
typedef struct parser parser_t;

// Создание парсера
parser_t* parser_create(lexer_t* lexer);

// Установка зависимостей
void parser_set_symbol_table(parser_t* parser, symbol_table_t* table);
void parser_set_ast_manager(parser_t* parser, ast_manager_t* ast);

// Разбор программы
ast_program_t* parser_parse_program(parser_t* parser);

// Разбор метки
ast_label_t* parser_parse_label(parser_t* parser, const char* label_name, const source_loc_t* label_loc);

// Разбор директивы
ast_directive_t* parser_parse_directive(parser_t* parser);

// Разбор инструкции
ast_instruction_t* parser_parse_instruction(parser_t* parser);

// Разбор операнда
ast_operand_t* parser_parse_operand(parser_t* parser);

// Проверка текущего токена
bool parser_match_token(parser_t* parser, token_type_t type);

// Ожидание определенного токена
bool parser_expect_token(parser_t* parser, token_type_t type);

// Пропуск текущего токена
void parser_consume_token(parser_t* parser);

// Получение текста ошибки
const char* parser_get_error(const parser_t* parser);

// Получение позиции ошибки
source_loc_t parser_get_error_location(const parser_t* parser);

// Уничтожение парсера
void parser_destroy(parser_t* parser);

#endif // TRIAS_PARSER_H