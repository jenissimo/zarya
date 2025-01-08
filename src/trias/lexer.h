#ifndef TRIAS_LEXER_H
#define TRIAS_LEXER_H

#include <stddef.h>
#include <stdbool.h>
#include "instruction_defs.h"
#include "zarya_config.h"

// Типы токенов
typedef enum {
    TOKEN_EOF,          // Конец файла
    TOKEN_NEWLINE,      // Перевод строки
    TOKEN_IDENTIFIER,   // Идентификатор (метка, инструкция)
    TOKEN_NUMBER,       // Десятичное число
    TOKEN_BINARY_NUMBER,  // Двоичное число (0b...)
    TOKEN_OCTAL_NUMBER,  // Восьмеричное число (0o...)
    TOKEN_HEX_NUMBER,    // Шестнадцатеричное число (0x...)
    TOKEN_TERNARY_NUMBER, // Троичное число (0t...)
    TOKEN_DIRECTIVE,    // Директива (.org, .align)
    TOKEN_COLON,        // Двоеточие (метка)
    TOKEN_COMMA,        // Запятая
    TOKEN_HASH,         // # (непосредственное значение)
    TOKEN_MINUS,        // - (отрицательное значение)
    TOKEN_AT,          // @ (косвенная адресация)
    TOKEN_DOT,         // . (точка)
    TOKEN_REGISTER,     // R0-R3
    TOKEN_ERROR        // Ошибка лексического анализа
} token_type_t;

// Локация в исходном коде
typedef struct {
    size_t line;       // Номер строки (1-based)
    size_t column;     // Номер колонки (1-based)
} source_loc_t;

// Значение токена
typedef union {
    int number;        // Для чисел
    int reg_num;       // Для регистров
} token_value_t;

// Токен
typedef struct {
    token_type_t type;  // Тип токена
    source_loc_t loc;   // Позиция в исходном коде
    const char* text;   // Текстовое представление (может быть NULL)
    token_value_t value; // Значение токена
} token_t;

// Лексер (скрытая реализация)
typedef struct lexer_t lexer_t;

// Создание и уничтожение
lexer_t* lexer_create(const char* source);
lexer_t* lexer_create_from_string(const char* input);
void lexer_destroy(lexer_t* lexer);

// Работа с токенами
token_t lexer_next_token(lexer_t* lexer);
void lexer_unget_token(lexer_t* lexer, token_t token);
void token_destroy(token_t* token);

// Получение ошибок и позиции
const char* lexer_get_error(const lexer_t* lexer);
source_loc_t lexer_get_location(const lexer_t* lexer);

#endif // TRIAS_LEXER_H 