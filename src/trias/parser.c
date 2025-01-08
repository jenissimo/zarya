#include "parser.h"
#include "lexer.h"
#include "logging.h"
#include "ast.h"
#include "zarya_config.h"
#include "instruction_defs.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#define LOG_SUBSYS LOG_PARSER

// Структура для хранения токена с его текстом
struct parser_token {
    token_t token;        // Сам токен
    char* text;           // Копия текста токена
};

// Структура парсера
struct parser {
    lexer_t* lexer;           // Лексический анализатор
    ast_manager_t* ast;       // Менеджер AST
    symbol_table_t* symbols;   // Таблица символов
    struct parser_token current;  // Текущий токен
    struct parser_token previous; // Предыдущий токен
    char* error_message;      // Сообщение об ошибке
    source_loc_t error_loc;   // Позиция ошибки
};

// Сохранение токена с копированием текста
static void parser_save_token(struct parser_token* dest, const token_t* src) {
    if (!dest || !src) return;
    
    LOG_DEBUG(LOG_SUBSYS, "Сохранение токена типа %d, text=%p, src->text=%p", 
              src->type, (void*)dest->text, (void*)src->text);
    
    // Сначала освобождаем старый текст, если он есть
    if (dest->text) {
        LOG_DEBUG(LOG_SUBSYS, "Освобождение старого текста %p", (void*)dest->text);
        free(dest->text);
        dest->text = NULL;
        dest->token.text = NULL;
    }
    
    // Копируем токен (без текста)
    dest->token = *src;
    dest->token.text = NULL;  // Очищаем указатель, так как мы будем его перезаписывать
    
    // Копируем текст только для токенов, которые его имеют
    switch (src->type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_DIRECTIVE:
        case TOKEN_ERROR:
        case TOKEN_NUMBER:
        case TOKEN_BINARY_NUMBER:
        case TOKEN_OCTAL_NUMBER:
        case TOKEN_HEX_NUMBER:
        case TOKEN_TERNARY_NUMBER:
            if (src->text) {
                dest->text = strdup(src->text);
                if (dest->text) {
                    dest->token.text = dest->text;
                    LOG_DEBUG(LOG_SUBSYS, "Скопирован текст для токена типа %d: '%s' (%p)", 
                             src->type, dest->text, (void*)dest->text);
                }
                // Освобождаем исходную строку
                free((void*)src->text);
                ((token_t*)src)->text = NULL;
            }
            break;
        default:
            // Для остальных типов токенов text не копируется
            dest->text = NULL;
            dest->token.text = NULL;
            LOG_DEBUG(LOG_SUBSYS, "Токен типа %d: text установлен в NULL", src->type);
            break;
    }
}

// Освобождение токена
static void parser_free_token(struct parser_token* token) {
    if (!token) return;
    
    LOG_DEBUG(LOG_SUBSYS, "Освобождение токена %p (text=%p, token.text=%p)", 
              (void*)token, (void*)token->text, (void*)token->token.text);
    
    if (token->text) {
        LOG_DEBUG(LOG_SUBSYS, "Освобождение текста токена %p", (void*)token->text);
        free(token->text);
        token->text = NULL;
        token->token.text = NULL;
    }
}

// Создание парсера
parser_t* parser_create(lexer_t* lexer) {
    LOG_DEBUG(LOG_SUBSYS, "Начало создания парсера");
    
    if (!lexer) {
        LOG_ERROR(LOG_SUBSYS, "Лексер равен NULL");
        return NULL;
    }
    
    parser_t* parser = calloc(1, sizeof(parser_t));
    if (!parser) {
        LOG_ERROR(LOG_SUBSYS, "Не удалось создать парсер (calloc вернул NULL)");
        return NULL;
    }
    
    LOG_DEBUG(LOG_SUBSYS, "Выделена память под парсер: %p", (void*)parser);
    
    parser->lexer = lexer;
    // Не создаем ast_manager здесь, он будет установлен через parser_set_ast_manager
    
    // Инициализируем токены нулями через calloc
    // token_init не нужен, так как calloc уже обнулил память
    
    LOG_DEBUG(LOG_SUBSYS, "Создан парсер %p с лексером %p", 
              (void*)parser, (void*)parser->lexer);
    return parser;
}

// Установка таблицы символов
void parser_set_symbol_table(parser_t* parser, symbol_table_t* table) {
    if (!parser) return;
    parser->symbols = table;
}

// Установка ошибки
static void parser_set_error(parser_t* parser, const char* message, const source_loc_t* loc) {
    char* old_message = parser->error_message;
    parser->error_message = strdup(message);
    free(old_message);  // Освобождаем старое сообщение
    
    if (loc) {
        parser->error_loc = *loc;
    } else {
        parser->error_loc = parser->current.token.loc;
    }
    LOG_ERROR(LOG_SUBSYS, "Ошибка на строке %zu, колонка %zu: %s",
              parser->error_loc.line, parser->error_loc.column, message);
}

// Получение следующего токена
static void parser_advance(parser_t* parser) {
    if (!parser) return;
    
    LOG_DEBUG(LOG_SUBSYS, "Начало parser_advance");
    
    // Сохраняем текущий токен как предыдущий
    LOG_DEBUG(LOG_SUBSYS, "Освобождение предыдущего токена");
    parser_free_token(&parser->previous);  // Освобождаем старый предыдущий токен
    parser->previous = parser->current;
    LOG_DEBUG(LOG_SUBSYS, "Текущий токен сохранен как предыдущий");
    memset(&parser->current, 0, sizeof(struct parser_token));  // Очищаем текущий токен
    
    // Читаем следующий токен
    token_t next = lexer_next_token(parser->lexer);
    LOG_DEBUG(LOG_SUBSYS, "Получен новый токен типа %d", next.type);
    parser_save_token(&parser->current, &next);
    
    // Проверяем ошибки лексера
    const char* lex_error = lexer_get_error(parser->lexer);
    if (lex_error) {
        parser_set_error(parser, lex_error, &parser->current.token.loc);
    }
    
    LOG_DEBUG(LOG_SUBSYS, "Завершение parser_advance");
}

// Проверка текущего токена
bool parser_match_token(parser_t* parser, token_type_t type) {
    if (!parser) return false;
    
    if (parser->current.token.type == type) {
        parser_advance(parser);
        return true;
    }
    return false;
}

// Ожидание определенного токена
bool parser_expect_token(parser_t* parser, token_type_t type) {
    if (!parser) return false;
    
    if (parser_match_token(parser, type)) {
        return true;
    }
    
    char error[256];
    snprintf(error, sizeof(error), "Ожидался токен типа %d, получен %d",
             type, parser->current.token.type);
    parser_set_error(parser, error, NULL);
    return false;
}

// Разбор метки
ast_label_t* parser_parse_label(parser_t* parser, const char* label_name, const source_loc_t* label_loc) {
    if (!parser || !label_name || !label_loc) {
        return NULL;
    }
    
    // Проверяем валидность имени метки
    if (!*label_name) {
        parser_set_error(parser, "Пустое имя метки", label_loc);
        return NULL;
    }
    
    // Определяем локальность метки по значению из токена
    bool is_local = parser->previous.token.value.number == 1;
    LOG_DEBUG(LOG_PARSER, "[ПАРСЕР] Разбор метки '%s' (локальная: %d)", label_name, is_local);
    
    // Проверяем соответствие имени и флага локальности
    if ((label_name[0] == '.') != is_local) {
        parser_set_error(parser, "Несоответствие имени метки и её локальности", label_loc);
        return NULL;
    }
    
    // Проверяем символы метки
    const char* start = is_local ? label_name + 1 : label_name;
    if (!isalpha((unsigned char)*start) && *start != '_') {
        parser_set_error(parser, 
            is_local ? "Локальная метка должна начинаться с буквы или подчеркивания после точки" 
                    : "Метка должна начинаться с буквы или подчеркивания", 
            label_loc);
        return NULL;
    }
    
    for (const char* p = start + 1; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_') {
            parser_set_error(parser, 
                "Метка может содержать только буквы, цифры и подчеркивания", 
                label_loc);
            return NULL;
        }
    }
    
    // Создаем узел метки с правильным флагом локальности
    ast_label_t* label = ast_create_label(parser->ast, label_name, is_local, label_loc);
    if (!label) {
        parser_set_error(parser, "Не удалось создать метку", label_loc);
        return NULL;
    }
    
    LOG_DEBUG(LOG_PARSER, "[ПАРСЕР] Создан узел метки '%s' (локальная: %d)", 
              label_name, label->is_local);
    
    return label;
}

// Проверка текущего токена без его потребления
static bool parser_check_token(parser_t* parser, token_type_t type) {
    if (!parser) return false;
    return parser->current.token.type == type;
}

// Вспомогательные функции для обработки ошибок
static void* parser_set_error_and_cleanup(parser_t* parser, const char* message, 
                                        const source_loc_t* loc, void* node) {
    parser_set_error(parser, message, loc);
    if (node) ast_unref((ast_node_t*)node);
    return NULL;
}

// Безопасное освобождение AST узла
static void parser_safe_unref(ast_node_t* node) {
    if (node) ast_unref(node);
}

// Функция-обертка для добавления операнда к директиве
static void parser_add_directive_operand(void* directive, ast_operand_t* operand) {
    LOG_DEBUG(LOG_PARSER, "Добавление операнда к директиве");
    ast_directive_add_arg((ast_directive_t*)directive, (ast_node_t*)operand);
    LOG_DEBUG(LOG_PARSER, "Операнд успешно добавлен к директиве");
}

// Проверка, является ли текущий токен возможным началом операнда
static bool parser_check_operand_start(parser_t* parser) {
    if (!parser) return false;
    
    token_type_t type = parser->current.token.type;
    return type == TOKEN_MINUS ||
           type == TOKEN_AT ||
           type == TOKEN_HASH ||
           type == TOKEN_REGISTER ||
           type == TOKEN_NUMBER ||
           type == TOKEN_BINARY_NUMBER ||
           type == TOKEN_OCTAL_NUMBER ||
           type == TOKEN_HEX_NUMBER ||
           type == TOKEN_TERNARY_NUMBER ||
           type == TOKEN_IDENTIFIER;
}

// Разбор списка операндов (общая логика для инструкций и директив)
static bool parser_parse_operand_list(parser_t* parser, void* parent,
                                    void (*add_operand)(void*, ast_operand_t*)) {
    LOG_DEBUG(LOG_PARSER, "Начало разбора списка операндов");
    
    // Проверяем, есть ли операнды
    if (!parser_check_operand_start(parser)) {
        LOG_DEBUG(LOG_PARSER, "Список операндов пуст");
        return true;
    }
    
    // Разбираем первый операнд
    LOG_DEBUG(LOG_PARSER, "Разбор операнда, текущий токен: тип %d, текст '%s'",
             parser->current.token.type,
             parser->current.token.text ? parser->current.token.text : "NULL");
    
    ast_operand_t* operand = parser_parse_operand(parser);
    if (!operand) {
        LOG_DEBUG(LOG_PARSER, "Не удалось разобрать операнд");
        return false;
    }
    
    add_operand(parent, operand);
    ast_unref((ast_node_t*)operand);
    
    // Разбираем остальные операнды после запятой
    while (parser_match_token(parser, TOKEN_COMMA)) {
        if (!parser_check_operand_start(parser)) {
            parser_set_error(parser, "Ожидался операнд после запятой", NULL);
            return false;
        }
        
        operand = parser_parse_operand(parser);
        if (!operand) {
            LOG_DEBUG(LOG_PARSER, "Не удалось разобрать операнд");
            return false;
        }
        
        add_operand(parent, operand);
        ast_unref((ast_node_t*)operand);
    }
    
    LOG_DEBUG(LOG_PARSER, "Завершение разбора списка операндов");
    return true;
}

// Разбор программы
ast_program_t* parser_parse_program(parser_t* parser) {
    LOG_DEBUG(LOG_SUBSYS, "Начало разбора программы");
    
    // Проверяем наличие таблицы символов
    if (!parser->symbols) {
        parser_set_error(parser, "Не установлена таблица символов", NULL);
        return NULL;
    }
    
    // Создаем узел программы
    ast_program_t* program = ast_create_program(parser->ast);
    if (!program) {
        parser_set_error(parser, "Не удалось создать узел программы", NULL);
        return NULL;
    }
    
    // Читаем первый токен
    parser_advance(parser);
    
    // Если есть ошибка лексера, прерываем разбор
    if (parser->error_message) {
        ast_unref((ast_node_t*)program);
        return NULL;
    }
    
    // Разбираем операторы до конца файла
    while (parser->current.token.type != TOKEN_EOF) {
        ast_node_t* stmt = NULL;
        source_loc_t stmt_loc = parser->current.token.loc;
        
        LOG_DEBUG(LOG_PARSER, "Текущий токен: тип %d, текст '%s'", 
                 parser->current.token.type,
                 parser->current.token.text ? parser->current.token.text : "NULL");
        
        // Определяем тип оператора по первому токену
        switch (parser->current.token.type) {
            case TOKEN_DIRECTIVE:
                LOG_DEBUG(LOG_PARSER, "Обнаружена директива '%s'", parser->current.token.text);
                stmt = (ast_node_t*)parser_parse_directive(parser);
                if (stmt) {
                    LOG_DEBUG(LOG_PARSER, "Директива успешно разобрана и добавлена в программу");
                    ast_program_add_statement(program, stmt);
                } else {
                    LOG_ERROR(LOG_PARSER, "Ошибка разбора директивы");
                }
                break;
                
            case TOKEN_IDENTIFIER: {
                // Сохраняем текущую позицию и копируем текст
                source_loc_t start_loc = parser->current.token.loc;
                char* name = strdup(parser->current.token.text);
                if (!name) {
                    parser_set_error(parser, "Ошибка выделения памяти", &start_loc);
                    ast_unref((ast_node_t*)program);
                    return NULL;
                }
                
                parser_advance(parser);
                
                // Проверяем, является ли это меткой
                if (parser_match_token(parser, TOKEN_COLON)) {
                    LOG_DEBUG(LOG_PARSER, "Обнаружена метка '%s'", name);
                    stmt = (ast_node_t*)parser_parse_label(parser, name, &start_loc);
                } else {
                    // Проверяем, не является ли это директивой
                    if (is_known_directive(name)) {
                        LOG_DEBUG(LOG_PARSER, "Обнаружена директива '%s' (без точки)", name);
                        stmt = (ast_node_t*)parser_parse_directive(parser);
                    } else {
                        LOG_DEBUG(LOG_PARSER, "Обнаружена инструкция '%s'", name);
                        stmt = (ast_node_t*)parser_parse_instruction(parser);
                    }
                }
                
                free(name);
                break;
            }
            
            case TOKEN_NEWLINE:
                LOG_DEBUG(LOG_PARSER, "Пропуск перевода строки");
                parser_advance(parser);
                continue;
                
            default: {
                char error[256];
                snprintf(error, sizeof(error), "Неожиданный токен типа %d", 
                        parser->current.token.type);
                LOG_ERROR(LOG_PARSER, "%s", error);
                parser_set_error(parser, error, &stmt_loc);
                ast_unref((ast_node_t*)program);
                return NULL;
            }
        }
        
        // Проверяем успешность разбора
        if (!stmt) {
            if (!parser->error_message) {
                parser_set_error(parser, "Не удалось разобрать оператор", &stmt_loc);
            }
            ast_unref((ast_node_t*)program);
            return NULL;
        }
        
        // Добавляем оператор в программу
        ast_program_add_statement(program, stmt);
        ast_unref(stmt);
        
        // Пропускаем необязательный перевод строки
        parser_match_token(parser, TOKEN_NEWLINE);
    }
    
    LOG_DEBUG(LOG_PARSER, "Программа успешно разобрана, всего операторов: %zu", program->statement_count);
    return program;
}

// Разбор директивы
ast_directive_t* parser_parse_directive(parser_t* parser) {
    LOG_DEBUG(LOG_PARSER, "Разбор директивы");
    
    source_loc_t start_loc = parser->previous.token.loc;  // Используем позицию из предыдущего токена
    const char* name = parser->previous.token.text;  // Используем имя из предыдущего токена
    LOG_DEBUG(LOG_PARSER, "Директива: текст '%s', позиция %zu:%zu", 
             name ? name : "NULL", start_loc.line, start_loc.column);
    
    // Проверяем, является ли директива локальной
    bool is_local = false;
    if (name && name[0] == '.') {
        is_local = true;
        LOG_DEBUG(LOG_PARSER, "Обнаружена локальная директива");
    } else {
        LOG_DEBUG(LOG_PARSER, "Обнаружена глобальная директива");
    }
             
    ast_directive_t* directive = ast_create_directive(parser->ast, name, &start_loc);
    if (!directive) {
        parser_set_error(parser, "Не удалось создать узел директивы", &start_loc);
        return NULL;
    }
    
    if (!parser_parse_operand_list(parser, directive,
                                  (void(*)(void*,ast_operand_t*))parser_add_directive_operand)) {
        ast_unref((ast_node_t*)directive);
        return NULL;
    }
    
    LOG_DEBUG(LOG_PARSER, "Директива '%s' успешно разобрана (%s)", 
             name, is_local ? "локальная" : "глобальная");
    return directive;
}

// Разбор инструкции
ast_instruction_t* parser_parse_instruction(parser_t* parser) {
    LOG_DEBUG(LOG_PARSER, "Разбор инструкции");
    
    // Мнемоника должна быть в предыдущем токене
    if (parser->previous.token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Ожидалась мнемоника инструкции", NULL);
        return NULL;
    }
    
    LOG_DEBUG(LOG_PARSER, "Разбор инструкции с мнемоникой '%s'", parser->previous.token.text);
    
    ast_instruction_t* instruction = ast_create_instruction(parser->ast, 
                                                          parser->previous.token.text,
                                                          &parser->previous.token.loc);
    if (!instruction) {
        parser_set_error(parser, "Не удалось создать узел инструкции", NULL);
        return NULL;
    }
    
    // Разбираем операнды
    if (!parser_parse_operand_list(parser, instruction, 
                                  (void(*)(void*,ast_operand_t*))ast_instruction_add_operand)) {
        ast_unref((ast_node_t*)instruction);
        return NULL;
    }
    
    LOG_DEBUG(LOG_PARSER, "Инструкция '%s' успешно разобрана с %zu операндами",
              instruction->mnemonic, instruction->operand_count);
    return instruction;
}

// Разбор операнда
ast_operand_t* parser_parse_operand(parser_t* parser) {
    if (!parser) return NULL;
    
    LOG_DEBUG(LOG_PARSER, "Начало разбора операнда");
    ast_operand_t* operand = NULL;
    bool is_indirect = false;
    bool is_negative = false;
    
    // Проверяем знак минус
    if (parser->current.token.type == TOKEN_MINUS) {
        LOG_DEBUG(LOG_PARSER, "Обнаружен знак минус");
        is_negative = true;
        parser_advance(parser);
    }
    
    // Проверяем косвенную адресацию
    if (parser->current.token.type == TOKEN_AT) {
        LOG_DEBUG(LOG_PARSER, "Обнаружена косвенная адресация");
        if (is_negative) {
            parser_set_error(parser, "Знак минус не может использоваться с косвенной адресацией", NULL);
            return NULL;
        }
        
        is_indirect = true;
        parser_advance(parser);
        
        // После @ должен быть регистр
        if (parser->current.token.type != TOKEN_REGISTER) {
            parser_set_error(parser, "После @ ожидался регистр", NULL);
            return NULL;
        }
        
        // Создаем операнд с регистром
        operand = ast_create_register_operand(parser->ast, parser->current.token.value);
        if (operand) {
            operand->is_indirect = true;
            operand->type = OPERAND_INDIRECT;  // Устанавливаем правильный тип операнда
            LOG_DEBUG(LOG_PARSER, "Создан операнд косвенной адресации через регистр");
        }
        parser_advance(parser);
        return operand;
    }
    
    // Разбираем основной операнд
    if (parser->current.token.type == TOKEN_HASH) {
        LOG_DEBUG(LOG_PARSER, "Обнаружено непосредственное значение");
        parser_advance(parser);
        
        // Непосредственное значение
        if (parser->current.token.type != TOKEN_NUMBER &&
            parser->current.token.type != TOKEN_BINARY_NUMBER &&
            parser->current.token.type != TOKEN_OCTAL_NUMBER &&
            parser->current.token.type != TOKEN_HEX_NUMBER &&
            parser->current.token.type != TOKEN_TERNARY_NUMBER) {
            parser_set_error(parser, "После # ожидалось число", NULL);
            return NULL;
        }
        
        operand = ast_create_immediate_operand(parser->ast, parser->current.token.value, parser->current.token.text);
        if (operand && is_negative) {
            operand->immediate = -operand->immediate;
            LOG_DEBUG(LOG_PARSER, "Создан операнд с отрицательным непосредственным значением");
        } else {
            LOG_DEBUG(LOG_PARSER, "Создан операнд с непосредственным значением");
        }
        parser_advance(parser);
        return operand;
    }
    else if (parser->current.token.type == TOKEN_REGISTER) {
        LOG_DEBUG(LOG_PARSER, "Обнаружен регистр");
        if (is_negative) {
            parser_set_error(parser, "Знак минус не может использоваться с регистром", NULL);
            return NULL;
        }
        
        operand = ast_create_register_operand(parser->ast, parser->current.token.value);
        if (operand) {
            LOG_DEBUG(LOG_PARSER, "Создан операнд с регистром");
        }
        parser_advance(parser);
        return operand;
    }
    else if (parser->current.token.type == TOKEN_NUMBER ||
             parser->current.token.type == TOKEN_BINARY_NUMBER ||
             parser->current.token.type == TOKEN_OCTAL_NUMBER ||
             parser->current.token.type == TOKEN_HEX_NUMBER ||
             parser->current.token.type == TOKEN_TERNARY_NUMBER) {
        LOG_DEBUG(LOG_PARSER, "Обнаружено числовое значение");
        operand = ast_create_immediate_operand(parser->ast, parser->current.token.value, parser->current.token.text);
        if (operand && is_negative) {
            operand->immediate = -operand->immediate;
            LOG_DEBUG(LOG_PARSER, "Создан операнд с отрицательным числовым значением");
        } else {
            LOG_DEBUG(LOG_PARSER, "Создан операнд с числовым значением");
        }
        parser_advance(parser);
        return operand;
    }
    else if (parser->current.token.type == TOKEN_IDENTIFIER) {
        // Метка
        LOG_DEBUG(LOG_PARSER, "Обнаружена метка");
        if (is_negative) {
            parser_set_error(parser, "Знак минус не может использоваться с меткой", NULL);
            return NULL;
        }
        operand = ast_create_label_operand(parser->ast, parser->current.token.text);
        if (operand) {
            LOG_DEBUG(LOG_PARSER, "Создан операнд с меткой");
        }
        parser_advance(parser);
        return operand;
    }
    
    LOG_DEBUG(LOG_PARSER, "Не удалось разобрать операнд");
    parser_set_error(parser, "Ожидался операнд", NULL);
    return NULL;
}

// Получение текста ошибки
const char* parser_get_error(const parser_t* parser) {
    return parser ? parser->error_message : NULL;
}

// Получение позиции ошибки
source_loc_t parser_get_error_location(const parser_t* parser) {
    return parser ? parser->error_loc : (source_loc_t){0, 0};
}

// Уничтожение парсера
void parser_destroy(parser_t* parser) {
    if (!parser) return;
    
    LOG_DEBUG(LOG_SUBSYS, "Начало уничтожения парсера %p", (void*)parser);
    
    // Освобождаем токены
    parser_free_token(&parser->current);
    parser_free_token(&parser->previous);
    
    // Освобождаем сообщение об ошибке
    free(parser->error_message);
    
    // Очищаем указатели на внешние зависимости
    parser->symbols = NULL;
    parser->ast = NULL;
    
    // Освобождаем сам парсер
    free(parser);
    
    LOG_DEBUG(LOG_SUBSYS, "Парсер уничтожен");
}

// Установка менеджера AST
void parser_set_ast_manager(parser_t* parser, ast_manager_t* ast) {
    if (!parser) return;
    parser->ast = ast;
} 