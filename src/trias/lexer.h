#ifndef TRIAS_LEXER_H
#define TRIAS_LEXER_H

#include <stddef.h>
#include <stdbool.h>
#include "errors.h"

// Типы токенов
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    TOKEN_NEWLINE,
    
    // Однозначные токены
    TOKEN_COLON,      // :
    TOKEN_COMMA,      // ,
    TOKEN_SEMICOLON,  // ;
    TOKEN_HASH,       // # (непосредственный режим)
    TOKEN_AT,         // @ (регистровый режим)
    TOKEN_STAR,       // * (косвенный режим)
    
    // Литералы
    TOKEN_IDENTIFIER, // имя
    TOKEN_NUMBER,     // число
    TOKEN_STRING,     // строка
    TOKEN_CHAR,       // символ
    
    // Директивы
    TOKEN_DIR_ORG,    // .org
    TOKEN_DIR_DB,     // .db
    TOKEN_DIR_DW,     // .dw
    TOKEN_DIR_DS      // .ds
} token_type_t;

// Структура токена
typedef struct {
    token_type_t type;     // Тип токена
    const char* start;     // Начало токена в исходном коде
    size_t length;         // Длина токена
    int line;             // Номер строки
    union {
        int number;       // Для TOKEN_NUMBER и TOKEN_CHAR
        char* string;     // Для TOKEN_STRING
    } value;
} token_t;

// Структура лексического анализатора
typedef struct {
    const char* source;    // Исходный код
    const char* start;     // Начало текущего токена
    const char* current;   // Текущая позиция
    int line;             // Текущая строка
    bool had_error;       // Флаг ошибки
    token_t peeked;       // Следующий токен (для предпросмотра)
    bool has_peeked;      // Флаг наличия предпросмотренного токена
} lexer_t;

// Инициализация лексера
void lexer_init(lexer_t* lexer, const char* source);

// Получение следующего токена
token_t lexer_next_token(lexer_t* lexer);

// Предпросмотр следующего токена без его потребления
token_t lexer_peek(lexer_t* lexer);

// Проверка наличия ошибок
bool lexer_had_error(const lexer_t* lexer);

// Освобождение ресурсов лексера
void lexer_free(lexer_t* lexer);

#endif // TRIAS_LEXER_H 