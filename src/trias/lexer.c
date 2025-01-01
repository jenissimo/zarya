#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>  // для strtol, free, strndup
#include <stdio.h>   // для printf и fflush
#include "lexer.h"

// Вспомогательные функции
static bool is_at_end(lexer_t* lexer) {
    return *lexer->current == '\0';
}

static char advance(lexer_t* lexer) {
    char c = *lexer->current;
    lexer->current++;
    return c;
}

static char peek(lexer_t* lexer) {
    if (is_at_end(lexer)) return '\0';
    return *lexer->current;
}

static char peek_next(lexer_t* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

// Пропуск пробельных символов
static void skip_whitespace(lexer_t* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case ';':  // Комментарий
                // Пропускаем все символы до конца строки
                while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                    advance(lexer);
                }
                // Не пропускаем символ новой строки
                break;
            default:
                return;
        }
    }
}

// Создание токена ошибки
static token_t error_token(lexer_t* lexer, const char* message) {
    token_t token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    lexer->had_error = true;
    return token;
}

// Создание токена
static token_t make_token(lexer_t* lexer, token_type_t type) {
    token_t token;
    token.type = type;
    token.start = lexer->start;
    token.length = (size_t)(lexer->current - lexer->start);
    token.line = lexer->line;
    return token;
}

// Разбор числа
static token_t number(lexer_t* lexer) {
    printf("DEBUG: number: начало разбора числа\n");
    fflush(stdout);
    
    // Пропускаем все цифры
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }
    
    // Создаем токен и сохраняем значение
    token_t token = make_token(lexer, TOKEN_NUMBER);
    char* temp = strndup(token.start, token.length);
    token.value.number = atoi(temp);
    printf("DEBUG: number: разобрано число %d\n", token.value.number);
    fflush(stdout);
    free(temp);
    
    // Проверяем, есть ли после числа символ новой строки
    char next = peek(lexer);
    printf("DEBUG: number: следующий символ '%c' (код %d)\n", next, next);
    fflush(stdout);
    
    // Не пропускаем символ новой строки, он будет обработан при следующем вызове lexer_next_token
    
    return token;
}

// Разбор строки
static token_t string(lexer_t* lexer) {
    // Пропускаем начальную кавычку
    advance(lexer);
    
    // Читаем содержимое строки
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') lexer->line++;
        advance(lexer);
    }
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "Незакрытая строка");
    }
    
    // Пропускаем закрывающую кавычку
    advance(lexer);
    
    // Создаем токен
    token_t token = make_token(lexer, TOKEN_STRING);
    // Копируем строку без кавычек
    token.value.string = strndup(token.start + 1, token.length - 2);
    return token;
}

// Разбор символа
static token_t character(lexer_t* lexer) {
    // Пропускаем начальную кавычку
    advance(lexer);
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "Незакрытый символьный литерал");
    }
    
    char c = advance(lexer);
    
    if (peek(lexer) != '\'') {
        return error_token(lexer, "Символьный литерал должен содержать один символ");
    }
    
    // Пропускаем закрывающую кавычку
    advance(lexer);
    
    // Создаем токен
    token_t token = make_token(lexer, TOKEN_CHAR);
    token.value.number = c;
    return token;
}

// Инициализация лексера
void lexer_init(lexer_t* lexer, const char* source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->had_error = false;
    lexer->has_peeked = false;
}

// Получение следующего токена
token_t lexer_next_token(lexer_t* lexer) {
    printf("DEBUG: lexer_next_token: начало\n");
    fflush(stdout);
    
    // Если есть предпросмотренный токен, возвращаем его
    if (lexer->has_peeked) {
        printf("DEBUG: lexer_next_token: возвращаем предпросмотренный токен типа %d\n", lexer->peeked.type);
        fflush(stdout);
        lexer->has_peeked = false;
        return lexer->peeked;
    }
    
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    if (is_at_end(lexer)) {
        printf("DEBUG: lexer_next_token: конец файла\n");
        fflush(stdout);
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = peek(lexer);  // Сначала смотрим на текущий символ
    printf("DEBUG: lexer_next_token: текущий символ '%c' (код %d)\n", c, c);
    fflush(stdout);
    
    // Проверяем символ новой строки
    if (c == '\n') {
        advance(lexer);  // Пропускаем символ новой строки
        lexer->line++;
        printf("DEBUG: lexer_next_token: новая строка (номер %d)\n", lexer->line);
        fflush(stdout);
        return make_token(lexer, TOKEN_NEWLINE);
    }
    
    // Продолжаем обычный разбор
    c = advance(lexer);
    
    // Проверяем, является ли символ #
    if (c == '#') {
        printf("DEBUG: lexer_next_token: токен #\n");
        fflush(stdout);
        return make_token(lexer, TOKEN_HASH);
    }
    
    switch (c) {
        case '@': 
            printf("DEBUG: lexer_next_token: токен @\n");
            fflush(stdout);
            return make_token(lexer, TOKEN_AT);
        case ',': 
            printf("DEBUG: lexer_next_token: токен ,\n");
            fflush(stdout);
            return make_token(lexer, TOKEN_COMMA);
        case ':': 
            printf("DEBUG: lexer_next_token: токен :\n");
            fflush(stdout);
            return make_token(lexer, TOKEN_COLON);
        case '"': 
            printf("DEBUG: lexer_next_token: начало строки\n");
            fflush(stdout);
            return string(lexer);
        case '\'': 
            printf("DEBUG: lexer_next_token: начало символа\n");
            fflush(stdout);
            return character(lexer);
    }
    
    // Числа
    if (isdigit(c)) {
        printf("DEBUG: lexer_next_token: обнаружено число\n");
        fflush(stdout);
        token_t token = number(lexer);
        printf("DEBUG: lexer_next_token: разобрано число %d\n", token.value.number);
        fflush(stdout);
        return token;
    }
    
    // Идентификаторы и ключевые слова
    if (isalpha(c) || c == '_' || c == '.') {
        while (isalnum(peek(lexer)) || peek(lexer) == '_' || peek(lexer) == '.') {
            advance(lexer);
        }
        
        // Проверяем, не директива ли это
        size_t length = lexer->current - lexer->start;
        const char* start = lexer->start;
        
        if (length > 0 && start[0] == '.') {
            if (length == 4 && strncmp(start, ".org", 4) == 0) {
                return make_token(lexer, TOKEN_DIR_ORG);
            } else if (length == 3 && strncmp(start, ".db", 3) == 0) {
                return make_token(lexer, TOKEN_DIR_DB);
            } else if (length == 3 && strncmp(start, ".dw", 3) == 0) {
                return make_token(lexer, TOKEN_DIR_DW);
            } else if (length == 3 && strncmp(start, ".ds", 3) == 0) {
                return make_token(lexer, TOKEN_DIR_DS);
            }
        }
        
        token_t token = make_token(lexer, TOKEN_IDENTIFIER);
        printf("DEBUG: lexer_next_token: идентификатор '%.*s'\n", 
               (int)token.length, token.start);
        fflush(stdout);
        return token;
    }
    
    printf("DEBUG: lexer_next_token: неожиданный символ\n");
    fflush(stdout);
    return error_token(lexer, "Неожиданный символ");
}

// Предпросмотр следующего токена
token_t lexer_peek(lexer_t* lexer) {
    if (lexer->has_peeked) {
        return lexer->peeked;
    }
    
    // Сохраняем текущее состояние
    const char* old_start = lexer->start;
    const char* old_current = lexer->current;
    int old_line = lexer->line;
    
    // Получаем следующий токен
    lexer->peeked = lexer_next_token(lexer);
    lexer->has_peeked = true;
    
    // Восстанавливаем состояние
    lexer->start = old_start;
    lexer->current = old_current;
    lexer->line = old_line;
    
    return lexer->peeked;
}

// Проверка наличия ошибок
bool lexer_had_error(const lexer_t* lexer) {
    return lexer->had_error;
}

// Освобождение ресурсов лексера
void lexer_free(lexer_t* lexer) {
    if (lexer == NULL) return;
    
    // Освобождаем предпросмотренный токен, если он есть
    if (lexer->has_peeked && lexer->peeked.type == TOKEN_STRING) {
        free(lexer->peeked.value.string);
    }
    
    // Сбрасываем все указатели
    lexer->source = NULL;
    lexer->start = NULL;
    lexer->current = NULL;
    lexer->has_peeked = false;
} 