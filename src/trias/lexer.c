#include "lexer.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

// Реальная структура лексера (скрыта от пользователей)
struct lexer_t {
    const char* source;     // Исходный текст
    size_t length;          // Длина текста
    size_t position;        // Текущая позиция
    source_loc_t loc;       // Текущая локация
    char* error_message;    // Сообщение об ошибке (владеем)
    source_loc_t error_loc; // Позиция ошибки
    token_t unget_token;    // Возвращенный токен
    bool has_unget;         // Флаг наличия возвращенного токена
    bool owns_source;       // Флаг владения исходным текстом
};

// Создание лексера
lexer_t* lexer_create(const char* source) {
    if (!source) return NULL;
    
    lexer_t* lexer = calloc(1, sizeof(lexer_t));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->position = 0;
    lexer->loc.line = 1;
    lexer->loc.column = 1;
    lexer->has_unget = false;
    lexer->owns_source = false;  // Не владеем исходным текстом
    
    return lexer;
}

// Создание лексера из строки
lexer_t* lexer_create_from_string(const char* input) {
    if (!input) {
        LOG_ERROR(LOG_LEXER, "Входная строка равна NULL");
        return NULL;
    }
    
    // Создаем копию входной строки
    char* input_copy = strdup(input);
    if (!input_copy) {
        LOG_ERROR(LOG_LEXER, "Не удалось создать копию входной строки");
        return NULL;
    }
    
    // Создаем лексер
    lexer_t* lexer = calloc(1, sizeof(lexer_t));
    if (!lexer) {
        LOG_ERROR(LOG_LEXER, "Не удалось создать лексер");
        free(input_copy);
        return NULL;
    }
    
    // Инициализируем поля
    lexer->source = input_copy;
    lexer->length = strlen(input);
    lexer->position = 0;
    lexer->loc.line = 1;
    lexer->loc.column = 1;
    lexer->owns_source = true;  // Владеем копией исходного текста
    
    LOG_DEBUG(LOG_LEXER, "Создан лексер для строки длиной %zu", lexer->length);
    return lexer;
}

// Освобождение памяти
void lexer_destroy(lexer_t* lexer) {
    if (!lexer) return;
    
    // Освобождаем сообщение об ошибке
    if (lexer->error_message) {
        free(lexer->error_message);
        lexer->error_message = NULL;
    }
    
    // Освобождаем возвращенный токен
    if (lexer->has_unget) {
        token_destroy(&lexer->unget_token);
        lexer->has_unget = false;
    }
    
    // Освобождаем копию исходного текста, только если мы ей владеем
    if (lexer->owns_source && lexer->source) {
        free((void*)lexer->source);
        lexer->source = NULL;
    }
    
    free(lexer);
}

// Освобождение памяти токена
void token_destroy(token_t* token) {
    if (!token) return;
    
    // Освобождаем текст только для токенов, которые его выделяют
    switch (token->type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_DIRECTIVE:
        case TOKEN_ERROR:
        case TOKEN_NUMBER:  // Теперь и для чисел освобождаем текст
            if (token->text) {
                free((void*)token->text);
                token->text = NULL;
            }
            break;
        default:
            // Для остальных типов токенов text не освобождается
            token->text = NULL;
            break;
    }
    
    // Очищаем остальные поля
    token->type = TOKEN_EOF;
    token->loc = (source_loc_t){0, 0};
    token->value.number = 0;
}

// Установка ошибки с позицией
static void lexer_set_error(lexer_t* lexer, const source_loc_t* loc, const char* format, ...) {
    if (!lexer) return;
    
    va_list args;
    va_start(args, format);
    
    // Освобождаем предыдущее сообщение об ошибке
    if (lexer->error_message) {
        free(lexer->error_message);
        lexer->error_message = NULL;
    }
    
    va_list args_copy;
    va_copy(args_copy, args);
    size_t size = vsnprintf(NULL, 0, format, args_copy) + 1;
    va_end(args_copy);
    
    lexer->error_message = malloc(size);
    
    if (lexer->error_message) {
        vsnprintf(lexer->error_message, size, format, args);
    }
    
    // Устанавливаем позицию ошибки
    if (loc) {
        lexer->error_loc = *loc;
        LOG_ERROR(LOG_LEXER, "Ошибка на строке %zu, колонка %zu: %s", 
                 lexer->error_loc.line, lexer->error_loc.column, lexer->error_message);
    } else {
        lexer->error_loc = lexer->loc;
        LOG_ERROR(LOG_LEXER, "Ошибка на строке %zu, колонка %zu: %s", 
                 lexer->error_loc.line, lexer->error_loc.column, lexer->error_message);
    }
    
    va_end(args);
}

// Получение текущего символа
static char lexer_current(const lexer_t* lexer) {
    return lexer->position < lexer->length ? lexer->source[lexer->position] : '\0';
}

// Следующий символ
static void lexer_advance(lexer_t* lexer) {
    if (lexer->position < lexer->length) {
        char c = lexer_current(lexer);
        lexer->position++;
        
        if (c == '\n') {
            lexer->loc.line++;
            lexer->loc.column = 1;
        } else {
            lexer->loc.column++;
        }
    }
}

// Пропуск однострочного комментария
static void lexer_skip_line_comment(lexer_t* lexer) {
    // Используем lexer_advance для корректного обновления колонки
    while (lexer_current(lexer) != '\n' && lexer_current(lexer) != '\0') {
        lexer_advance(lexer);
    }
}

// Пропуск многострочного комментария с поддержкой вложенности
// Пропуск многострочного комментария с поддержкой вложенности
static bool lexer_skip_block_comment(lexer_t* lexer) {
    source_loc_t start_loc = lexer->loc;  // Запоминаем позицию начала комментария

    // Пропускаем начальные /*
    lexer_advance(lexer); // skip '/'
    lexer_advance(lexer); // skip '*'

    int nested = 1;  // Уровень вложенности

    while (lexer_current(lexer) != '\0') {
        if (lexer_current(lexer) == '/' && 
            lexer->position + 1 < lexer->length && 
            lexer->source[lexer->position + 1] == '*') {
            // Найдено вложенное /*
            lexer_advance(lexer);
            lexer_advance(lexer);
            nested++;
            LOG_TRACE(LOG_LEXER, "Вложенный комментарий найден. Уровень вложенности: %d", nested);
        }
        else if (lexer_current(lexer) == '*' && 
                 lexer->position + 1 < lexer->length && 
                 lexer->source[lexer->position + 1] == '/') {
            // Найдено закрывающее */
            lexer_advance(lexer);
            lexer_advance(lexer);
            nested--;
            LOG_TRACE(LOG_LEXER, "Закрывающий символ комментария найден. Уровень вложенности: %d", nested);
            if (nested == 0) {
                return true;
            }
        }
        else {
            lexer_advance(lexer);
        }
    }

    // Комментарий не закрыт
    source_loc_t error_loc = start_loc;
    error_loc.column += 1; // Устанавливаем колонку на 6 (после '*')
    lexer_set_error(lexer, &error_loc, "Незакрытый многострочный комментарий начатый на строке %zu, колонка %zu", start_loc.line, start_loc.column);
    return false;
}

// Пропуск пробельных символов и комментариев
static void lexer_skip_whitespace(lexer_t* lexer) {
    while (true) {
        char c = lexer_current(lexer);
        
        if (c == '\0') {
            break;
        } else if (c == '\n') {
            break;  // Перевод строки обрабатываем как токен
        } else if (isspace(c)) {
            // Пропускаем пробельные символы
            lexer_advance(lexer);
        } else if (c == ';') {
            // Однострочный комментарий
            LOG_TRACE(LOG_LEXER, "Обнаружен однострочный комментарий на строке %zu, колонка %zu", 
                     lexer->loc.line, lexer->loc.column);
            lexer_skip_line_comment(lexer);
        } else if (c == '/' && lexer->position + 1 < lexer->length && 
                  lexer->source[lexer->position + 1] == '*') {
            // Многострочный комментарий
            LOG_TRACE(LOG_LEXER, "Обнаружен многострочный комментарий на строке %zu, колонка %zu", 
                     lexer->loc.line, lexer->loc.column);
            if (!lexer_skip_block_comment(lexer)) {
                break;  // Ошибка в комментарии
            }
        } else {
            break;
        }
    }
}

// Создание токена с текущей локацией
static token_t lexer_make_token(lexer_t* lexer, token_type_t type) {
    token_t token = {
        .type = type,
        .loc = lexer->loc
    };
    return token;
}

// Проверка, является ли строка регистром
static bool is_register_token(const char* text, token_value_t* value) {
    if (!text || !value) return false;
    
    // Регистр должен начинаться с 'R'
    if (text[0] != 'R' && text[0] != 'r') return false;
    
    // После R должна быть цифра
    if (!isdigit((unsigned char)text[1])) {
        LOG_DEBUG(LOG_LEXER, "Некорректный регистр: после R должна быть цифра");
        return false;
    }
    
    // После цифры не должно быть других символов
    if (text[2] != '\0') {
        LOG_DEBUG(LOG_LEXER, "Некорректный регистр: лишние символы после номера");
        return false;
    }
    
    // Преобразуем номер регистра
    int reg_num = text[1] - '0';
    
    // Проверяем диапазон
    if (reg_num < 0 || reg_num >= NUM_REGISTERS) {
        LOG_DEBUG(LOG_LEXER, "Некорректный регистр: номер %d вне диапазона [0-%d]", 
                 reg_num, NUM_REGISTERS - 1);
        return false;
    }
    
    value->reg_num = reg_num;
    return true;
}

// Чтение идентификатора или ключевого слова
static token_t lexer_read_identifier(lexer_t* lexer) {
    token_t token = {0};
    token.type = TOKEN_IDENTIFIER;
    token.loc = lexer->loc;
    
    // Читаем символы идентификатора
    size_t capacity = 16;
    size_t length = 0;
    char* buffer = malloc(capacity);
    if (!buffer) {
        token.type = TOKEN_ERROR;
        token.text = strdup("Ошибка выделения памяти");
        return token;
    }
    
    // Читаем все символы идентификатора
    while (isalnum(lexer_current(lexer)) || lexer_current(lexer) == '_') {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char* new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                token.type = TOKEN_ERROR;
                token.text = strdup("Ошибка выделения памяти");
                return token;
            }
            buffer = new_buffer;
        }
        buffer[length++] = lexer_current(lexer);
        lexer_advance(lexer);
    }
    buffer[length] = '\0';
    
    token.text = buffer;
    token.value.number = 0;  // Это не локальная метка
    
    LOG_DEBUG(LOG_LEXER, "[ЛЕКСЕР] Создан идентификатор: '%s' (локальный: 0)", buffer);
    return token;
}

// Проверка цифры для разных систем счисления
static bool is_binary_digit(char c) {
    return c == '0' || c == '1';
}

static bool is_octal_digit(char c) {
    return c >= '0' && c <= '7';
}

static bool is_decimal_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}

// Получение значения шестнадцатеричной цифры
static int hex_digit_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool is_ternary_digit(char c) {
    return c == '+' || c == '0' || c == '-';
}

// Получение значения троичной цифры в сбалансированной системе
static int ternary_digit_value(char c) {
    switch (c) {
        case '+': return 1;
        case '0': return 0;
        case '-': return -1;
        default: return -2;  // Ошибка
    }
}

// Чтение числа в указанной системе счисления
static bool read_number_base(lexer_t* lexer, int base,
                          bool (*is_valid_digit)(char),
                          int (*digit_value_fn)(char),
                          int* result) {
    LOG_DEBUG(LOG_LEXER, "Начало разбора числа в системе счисления %d", base);
    
    *result = 0;
    LOG_DEBUG(LOG_LEXER, "Инициализация result = %d", *result);
    
    bool has_digits = false;
    const char* start = lexer->source + lexer->position;
    
    while (true) {
        char c = lexer_current(lexer);
        
        // Если встретили не-цифру или конец числа
        if (isspace(c) || c == '\0' || c == ',') {
            break;
        }
        
        // Для троичных чисел разрешаем любые символы, проверка будет в семантическом анализаторе
        if (base == 3) {
            has_digits = true;
            lexer_advance(lexer);
            continue;
        }
        
        // Для остальных систем проверяем корректность цифр
        if (!is_valid_digit(c)) {
            LOG_DEBUG(LOG_LEXER, "Некорректный символ '%c' в числе", c);
            return false;
        }
        
        int digit;
        int prev_result = *result;
        
        if (digit_value_fn) {
            digit = digit_value_fn(c);
            if (digit == -2 || (base != 3 && digit < 0)) {
                LOG_DEBUG(LOG_LEXER, "Некорректная цифра '%c' для системы счисления %d", c, base);
                return false;
            }
        } else {
            digit = c - '0';
            if (digit >= base) {
                LOG_DEBUG(LOG_LEXER, "Цифра %d больше основания системы счисления %d", digit, base);
                return false;
            }
        }
        
        // Для троичной системы используем сбалансированное представление
        if (base == 3) {
            *result = *result * 3 + digit;
        } else {
            // Проверка на переполнение для других систем счисления
            if (*result > INT_MAX / base || 
                (*result == INT_MAX / base && digit > INT_MAX % base)) {
                lexer_set_error(lexer, NULL, "Переполнение числа");
                LOG_DEBUG(LOG_LEXER, "Переполнение при разборе числа");
                return false;
            }
            *result = *result * base + digit;
        }
        
        LOG_DEBUG(LOG_LEXER, "Операция: %d * %d + %d = %d", prev_result, base, digit, *result);
        
        has_digits = true;
        lexer_advance(lexer);
    }
    
    return has_digits;
}

// Модифицированная функция чтения числа
static token_t lexer_read_number(lexer_t* lexer) {
    LOG_DEBUG(LOG_LEXER, "Начало разбора числа на позиции %zu", lexer->position);
    
    token_t token = {
        .type = TOKEN_NUMBER,
        .loc = lexer->loc,
        .text = NULL,
        .value = {.number = 0}
    };
    
    const char* start = lexer->source + lexer->position;
    size_t start_pos = lexer->position;
    
    // Обработка знака числа
    int sign = 1;
    if (lexer_current(lexer) == '-' || lexer_current(lexer) == '+') {
        sign = (lexer_current(lexer) == '-') ? -1 : 1;
        lexer_advance(lexer);
    }
    
    if (lexer_current(lexer) == '0') {
        LOG_DEBUG(LOG_LEXER, "Обнаружен ведущий ноль на позиции %zu", lexer->position);
        lexer_advance(lexer);
        char prefix = lexer_current(lexer);
        LOG_DEBUG(LOG_LEXER, "Следующий символ после нуля: '%c'", prefix);
        
        switch (prefix) {
            case 'x': case 'X':
                LOG_DEBUG(LOG_LEXER, "Обнаружен префикс шестнадцатеричного числа");
                lexer_advance(lexer);
                token.type = TOKEN_HEX_NUMBER;
                LOG_DEBUG(LOG_LEXER, "Установлен тип TOKEN_HEX_NUMBER (%d)", token.type);
                if (!read_number_base(lexer, 16, is_hex_digit, hex_digit_value, &token.value.number)) {
                    LOG_DEBUG(LOG_LEXER, "Ошибка при разборе шестнадцатеричного числа");
                    token.type = TOKEN_ERROR;
                    lexer_set_error(lexer, &token.loc, "Некорректное шестнадцатеричное число");
                }
                break;
                
            case 'b': case 'B':
                LOG_DEBUG(LOG_LEXER, "Обнаружен префикс двоичного числа");
                lexer_advance(lexer);
                token.type = TOKEN_BINARY_NUMBER;
                LOG_DEBUG(LOG_LEXER, "Установлен тип TOKEN_BINARY_NUMBER (%d)", token.type);
                if (!read_number_base(lexer, 2, is_binary_digit, NULL, &token.value.number)) {
                    LOG_DEBUG(LOG_LEXER, "Ошибка при разборе двоичного числа");
                    token.type = TOKEN_ERROR;
                    lexer_set_error(lexer, &token.loc, "Некорректное двоичное число");
                }
                break;
                
            case 'o': case 'O':
                LOG_DEBUG(LOG_LEXER, "Обнаружен префикс восьмеричного числа");
                lexer_advance(lexer);
                token.type = TOKEN_OCTAL_NUMBER;
                LOG_DEBUG(LOG_LEXER, "Установлен тип TOKEN_OCTAL_NUMBER (%d)", token.type);
                if (!read_number_base(lexer, 8, is_octal_digit, NULL, &token.value.number)) {
                    LOG_DEBUG(LOG_LEXER, "Ошибка при разборе восьмеричного числа");
                    token.type = TOKEN_ERROR;
                    lexer_set_error(lexer, &token.loc, "Некорректное восьмеричное число");
                }
                break;
                
            case 't': case 'T':
                LOG_DEBUG(LOG_LEXER, "Обнаружен префикс троичного числа");
                lexer_advance(lexer);
                token.type = TOKEN_TERNARY_NUMBER;
                LOG_DEBUG(LOG_LEXER, "Установлен тип TOKEN_TERNARY_NUMBER (%d)", token.type);
                if (!read_number_base(lexer, 3, is_ternary_digit, ternary_digit_value, &token.value.number)) {
                    LOG_DEBUG(LOG_LEXER, "Ошибка при разборе троичного числа");
                    token.type = TOKEN_ERROR;
                    lexer_set_error(lexer, &token.loc, "Некорректное троичное число");
                }
                break;
                
            default:
                if (is_octal_digit(prefix)) {
                    LOG_DEBUG(LOG_LEXER, "Обнаружено восьмеричное число без префикса");
                    // Возвращаемся к началу числа для правильного разбора
                    lexer->position = start_pos;
                    if (!read_number_base(lexer, 10, is_decimal_digit, NULL, &token.value.number)) {
                        LOG_DEBUG(LOG_LEXER, "Ошибка при разборе десятичного числа");
                        token.type = TOKEN_ERROR;
                        lexer_set_error(lexer, &token.loc, "Некорректное десятичное число");
                    }
                } else {
                    LOG_DEBUG(LOG_LEXER, "Обнаружено десятичное число, начинающееся с нуля");
                    // Возвращаемся к началу числа для правильного разбора
                    lexer->position = start_pos;
                    if (!read_number_base(lexer, 10, is_decimal_digit, NULL, &token.value.number)) {
                        LOG_DEBUG(LOG_LEXER, "Ошибка при разборе десятичного числа");
                        token.type = TOKEN_ERROR;
                        lexer_set_error(lexer, &token.loc, "Некорректное десятичное число");
                    }
                }
                break;
        }
    } else {
        LOG_DEBUG(LOG_LEXER, "Обнаружено десятичное число без ведущего нуля");
        if (!read_number_base(lexer, 10, is_decimal_digit, NULL, &token.value.number)) {
            LOG_DEBUG(LOG_LEXER, "Ошибка при разборе десятичного числа");
            token.type = TOKEN_ERROR;
            lexer_set_error(lexer, &token.loc, "Некорректное десятичное число");
        }
    }
    
    // Применяем знак к значению
    token.value.number *= sign;
    
    // Сохраняем исходный текст числа
    size_t length = lexer->source + lexer->position - start;
    char* text_copy = malloc(length + 1);
    if (text_copy) {
        memcpy(text_copy, start, length);
        text_copy[length] = '\0';
        token.text = text_copy;
        LOG_DEBUG(LOG_LEXER, "Сохранен исходный текст числа: '%s'", token.text);
    }
    
    return token;
}

// Чтение директивы
static token_t lexer_read_directive(lexer_t* lexer) {
    // Сохраняем начальную позицию (включая точку)
    source_loc_t start_loc = lexer->loc;
    
    // Пропускаем точку
    lexer_advance(lexer);
    
    // Читаем идентификатор
    token_t token = lexer_read_identifier(lexer);
    token.type = TOKEN_DIRECTIVE;
    token.loc = start_loc;  // Восстанавливаем начальную позицию
    
    printf("[DEBUG] Создана директива: '%s' на строке %zu, колонка %zu\n", token.text, token.loc.line, token.loc.column);
    
    return token;
}

// Реализация возврата токена
void lexer_unget_token(lexer_t* lexer, token_t token) {
    if (!lexer) return;
    
    // Если уже есть возвращенный токен, освобождаем его
    if (lexer->has_unget) {
        token_destroy(&lexer->unget_token);
    }
    
    // Сохраняем новый токен
    lexer->unget_token = token;
    lexer->has_unget = true;
}

// Изменяем lexer_next_token для поддержки возвращенных токенов
token_t lexer_next_token(lexer_t* lexer) {
    if (!lexer) {
        token_t error = {0};
        error.type = TOKEN_ERROR;
        return error;
    }
    
    LOG_DEBUG(LOG_LEXER, "Начало lexer_next_token, позиция %zu", lexer->position);
    
    // Если есть возвращенный токен, возвращаем его
    if (lexer->has_unget) {
        token_t token = lexer->unget_token;
        LOG_DEBUG(LOG_LEXER, "Возвращен отложенный токен типа %d", token.type);
        lexer->has_unget = false;
        lexer->unget_token.text = NULL;  // Предотвращаем двойное освобождение
        return token;
    }
    
    // Сначала пропускаем пробельные символы и комментарии
    lexer_skip_whitespace(lexer);
    
    token_t token = lexer_make_token(lexer, TOKEN_ERROR);
    token.loc = lexer->loc;  // Используем текущую позицию
    
    LOG_DEBUG(LOG_LEXER, "После пропуска пробелов, текущий символ: '%c'", lexer_current(lexer));
    
    // Проверяем наличие ошибки после пропуска пробелов/комментариев
    if (lexer_get_error(lexer)) {
        token.loc = lexer->error_loc;  // Используем позицию ошибки
        printf("[DEBUG] Ошибка лексера на строке %zu, колонка %zu: %s\n", token.loc.line, token.loc.column, lexer->error_message);
        return token;
    }
    
    char c = lexer_current(lexer);
    LOG_DEBUG(LOG_LEXER, "Разбор символа '%c' на позиции %zu", c, lexer->position);
    
    if (c == '\0') {
        token.type = TOKEN_EOF;
        printf("[DEBUG] Создан EOF на строке %zu, колонка %zu\n", token.loc.line, token.loc.column);
        return token;
    }
    
    // Сохраняем позицию начала токена
    source_loc_t token_start = lexer->loc;
    
    // Проверяем, является ли это числом или идентификатором, начинающимся с цифры
    if (isdigit(c) || ((c == '+' || c == '-') && lexer->position > 0 && 
        lexer->source[lexer->position - 1] == '#')) {
        // Если это 0, за которым следует b, o, x или t, то это точно число
        if (c == '0' && lexer->position + 1 < lexer->length) {
            char next = lexer->source[lexer->position + 1];
            if (next == 'b' || next == 'B' || 
                next == 'o' || next == 'O' || 
                next == 'x' || next == 'X' || 
                next == 't' || next == 'T' || 
                isdigit(next)) {
                LOG_DEBUG(LOG_LEXER, "Обнаружено число с префиксом или ведущим нулем");
                token = lexer_read_number(lexer);
                return token;
            }
        }
        
        // Проверяем, не является ли это идентификатором, начинающимся с цифры
        size_t peek_pos = lexer->position;
        if (c == '+' || c == '-') peek_pos++;  // Пропускаем знак
        while (peek_pos < lexer->length && isdigit(lexer->source[peek_pos])) {
            peek_pos++;
        }
        if (peek_pos < lexer->length && (isalpha(lexer->source[peek_pos]) || lexer->source[peek_pos] == '_')) {
            // Это идентификатор, начинающийся с цифры
            LOG_DEBUG(LOG_LEXER, "Обнаружен идентификатор, начинающийся с цифры");
            token = lexer_read_identifier(lexer);
            return token;
        }
        
        // Это обычное число
        LOG_DEBUG(LOG_LEXER, "Обнаружено обычное число");
        token = lexer_read_number(lexer);
        return token;
    }
    
    lexer_advance(lexer);
    
    switch (c) {
        case '\n':
            token.type = TOKEN_NEWLINE;
            token.loc = token_start;  // Используем позицию начала токена
            printf("[DEBUG] Создан NEWLINE на строке %zu, колонка %zu\n", token.loc.line, token.loc.column);
            break;
            
        case ':':
            token.type = TOKEN_COLON;
            printf("[DEBUG] Создан COLON на строке %zu, колонка %zu\n", token.loc.line, token.loc.column);
            break;
            
        case ',':
            token.type = TOKEN_COMMA;
            printf("[DEBUG] Создан COMMA на строке %zu, колонка %zu\n", token.loc.line, token.loc.column);
            break;
            
        case '#':
            token.type = TOKEN_HASH;
            printf("[DEBUG] Создан HASH на строке %zu, колонка %zu\n", token.loc.line, token.loc.column);
            break;
            
        case '-':
            // Если предыдущий токен был #, возвращаемся назад для чтения числа
            if (lexer->position > 1 && lexer->source[lexer->position - 2] == '#') {
                lexer->position--;  // Возвращаемся к минусу
                lexer->loc = token_start;  // Восстанавливаем позицию
                token = lexer_read_number(lexer);
            } else {
                token.type = TOKEN_MINUS;
                printf("[DEBUG] Создан MINUS на строке %zu, колонка %zu\n", token.loc.line, token.loc.column);
            }
            break;
            
        case '@':
            token.type = TOKEN_AT;
            printf("[DEBUG] Создан AT на строке %zu, колонка %zu\n", token.loc.line, token.loc.column);
            break;
            
        case '.': {
            LOG_DEBUG(LOG_LEXER, "[ЛЕКСЕР] Обнаружена точка, следующий символ: '%c'", 
                     lexer->position + 1 < lexer->length ? lexer->source[lexer->position + 1] : '\0');
            
            // Проверяем следующий символ без его потребления
            char next_char = (lexer->position + 1 < lexer->length) ? 
                           lexer->source[lexer->position + 1] : '\0';
            
            if (isalpha((unsigned char)next_char)) {
                // Сохраняем позицию точки
                source_loc_t dot_loc = lexer->loc;
                
                // Создаем токен с точкой
                token.type = TOKEN_IDENTIFIER;
                token.loc = dot_loc;
                token.value.number = 1; // Это локальная метка
                
                // Читаем идентификатор после точки
                size_t capacity = 16;
                char* buffer = malloc(capacity);
                size_t length = 0;
                
                // Сначала добавляем точку
                buffer[length++] = lexer_current(lexer);
                lexer_advance(lexer); // Пропускаем точку
                
                // Теперь читаем идентификатор
                while (isalnum(lexer_current(lexer)) || lexer_current(lexer) == '_') {
                    if (length + 1 >= capacity) {
                        capacity *= 2;
                        char* new_buffer = realloc(buffer, capacity);
                        if (!new_buffer) {
                            free(buffer);
                            token.type = TOKEN_ERROR;
                            token.text = strdup("Ошибка выделения памяти");
                            return token;
                        }
                        buffer = new_buffer;
                    }
                    buffer[length++] = lexer_current(lexer);
                    lexer_advance(lexer);
                }
                buffer[length] = '\0';
                
                // Проверяем, является ли это директивой
                if (is_known_directive(buffer + 1)) { // +1 чтобы пропустить точку
                    // Это директива - используем имя без точки
                    token.type = TOKEN_DIRECTIVE;
                    char* name = strdup(buffer + 1);
                    free(buffer);
                    token.text = name;
                    LOG_DEBUG(LOG_LEXER, "[ЛЕКСЕР] Создана директива: '%s'", token.text);
                } else {
                    // Это локальная метка - используем полное имя с точкой
                    token.text = buffer;
                    LOG_DEBUG(LOG_LEXER, "[ЛЕКСЕР] Создана локальная метка: '%s' (локальная: 1)", token.text);
                }
                
                return token;
            } else {
                token.type = TOKEN_DOT;
                LOG_DEBUG(LOG_LEXER, "[ЛЕКСЕР] Создана точка");
                lexer_advance(lexer);
                return token;
            }
            break;
        }
            
        case 'R':
        case 'r':
            // Возвращаемся назад для чтения всего идентификатора
            lexer->position--;  // Возвращаемся назад для чтения идентификатора
            lexer->loc = token_start;  // Восстанавливаем позицию
            token = lexer_read_identifier(lexer);
            
            // Если это не регистр, оставляем как идентификатор
            if (token.type != TOKEN_REGISTER) {
                // Если это начинается с R, проверяем, не должен ли это быть регистр
                if (c == 'R' || c == 'r') {
                    size_t peek_pos = lexer->position;
                    while (peek_pos < lexer->length && 
                           (isalnum((unsigned char)lexer->source[peek_pos]) || 
                            lexer->source[peek_pos] == '_')) {
                        peek_pos++;
                    }
                    
                    // Если после R идет цифра, это должен был быть регистр
                    if (lexer->position < lexer->length && 
                        isdigit((unsigned char)lexer->source[lexer->position])) {
                        lexer_set_error(lexer, &token_start, "Некорректный регистр");
                        token.type = TOKEN_ERROR;
                        break;
                    }
                }
                LOG_DEBUG(LOG_LEXER, "Идентификатор, начинающийся с R: %s", token.text);
            }
            break;
            
        default:
            if (isalpha(c) || c == '_') {
                lexer->position--;  // Возвращаемся назад для чтения идентификатора
                lexer->loc = token_start;  // Восстанавливаем позицию
                token = lexer_read_identifier(lexer);
            } else {
                lexer_set_error(lexer, &token_start, "Неожиданный символ '%c'", c);
                printf("[DEBUG] Неожиданный символ '%c' на строке %zu, колонка %zu\n", c, token_start.line, token_start.column);
            }
    }
    
    return token;
}

// Получение текста ошибки
const char* lexer_get_error(const lexer_t* lexer) {
    return lexer ? lexer->error_message : NULL;
}

// Получение текущей позиции
source_loc_t lexer_get_location(const lexer_t* lexer) {
    return lexer ? lexer->loc : (source_loc_t){0, 0};
} 
