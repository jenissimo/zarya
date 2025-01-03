#include "parser.h"
#include "lexer.h"
#include "logging.h"
#include "ast.h"
#include "zarya_config.h"
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
    
    // Проверяем, что метка начинается с буквы
    if (!isalpha((unsigned char)label_name[0]) && label_name[0] != '_') {
        parser_set_error(parser, "Метка должна начинаться с буквы или подчеркивания", label_loc);
        return NULL;
    }
    
    // Проверяем остальные символы метки
    for (const char* p = label_name + 1; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_') {
            parser_set_error(parser, "Метка может содержать только буквы, цифры и подчеркивания", label_loc);
            return NULL;
        }
    }
    
    // Проверяем, что метка еще не определена
    symbol_t* existing = symbol_table_lookup_local(parser->symbols, label_name);
    if (existing && (existing->flags & SYMBOL_FLAG_DEFINED)) {
        parser_set_error(parser, "Метка уже определена", label_loc);
        return NULL;
    }
    
    // Создаем узел метки
    ast_label_t* label = ast_create_label(parser->ast, label_name, false, label_loc);
    if (!label) {
        parser_set_error(parser, "Не удалось создать узел метки", label_loc);
        return NULL;
    }
    
    // Добавляем метку в таблицу символов
    tryte_t value = TRYTE_FROM_INT(0);  // Значение будет установлено позже
    vm_error_t err = symbol_table_add(parser->symbols, label_name, SYMBOL_LABEL,
                                     value, SYMBOL_FLAG_NONE, *label_loc);
    
    if (err != VM_OK) {
        parser_set_error(parser, "Не удалось добавить метку в таблицу символов", label_loc);
        ast_unref(&label->base);
        return NULL;
    }
    
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

// Разбор списка операндов (общая логика для инструкций и директив)
static bool parser_parse_operand_list(parser_t* parser, void* parent,
                                    void (*add_operand)(void*, ast_operand_t*)) {
    while (!parser_check_token(parser, TOKEN_NEWLINE) && !parser_check_token(parser, TOKEN_EOF)) {
        ast_operand_t* operand = parser_parse_operand(parser);
        if (!operand) {
            return false;
        }
        
        add_operand(parent, operand);
        ast_unref((ast_node_t*)operand);
        
        if (parser_match_token(parser, TOKEN_COMMA)) {
            if (parser_check_token(parser, TOKEN_NEWLINE) || parser_check_token(parser, TOKEN_EOF)) {
                parser_set_error(parser, "Ожидался операнд после запятой", NULL);
                return false;
            }
        } else {
            break;
        }
    }
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
        
        // Определяем тип оператора по первому токену
        switch (parser->current.token.type) {
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
                    stmt = (ast_node_t*)parser_parse_label(parser, name, &start_loc);
                } else {
                    stmt = (ast_node_t*)parser_parse_instruction(parser);
                }
                
                free(name);
                break;
            }
            
            case TOKEN_DIRECTIVE:
                stmt = (ast_node_t*)parser_parse_directive(parser);
                break;
                
            default: {
                char error[256];
                snprintf(error, sizeof(error), "Неожиданный токен типа %d", 
                        parser->current.token.type);
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
    
    return program;
}

// Разбор директивы
ast_directive_t* parser_parse_directive(parser_t* parser) {
    LOG_DEBUG(LOG_PARSER, "Разбор директивы");
    
    source_loc_t start_loc = parser->current.token.loc;
    ast_directive_t* directive = ast_create_directive(parser->ast, 
                                                    parser->current.token.text,
                                                    &start_loc);
    if (!directive) {
        parser_set_error(parser, "Не удалось создать узел директивы", &start_loc);
        return NULL;
    }
    
    parser_advance(parser);
    
    if (!parser_parse_operand_list(parser, directive,
                                  (void(*)(void*,ast_operand_t*))ast_directive_add_arg)) {
        ast_unref((ast_node_t*)directive);
        return NULL;
    }
    
    LOG_DEBUG(LOG_PARSER, "Директива '%s' успешно разобрана", directive->name);
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
    
    ast_operand_t* operand = NULL;
    bool is_indirect = false;
    
    // Проверяем косвенную адресацию
    if (parser->current.token.type == TOKEN_AT) {
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
        }
        parser_advance(parser);
        return operand;
    }
    
    // Разбираем основной операнд
    if (parser->current.token.type == TOKEN_HASH) {
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
        
        operand = ast_create_immediate_operand(parser->ast, parser->current.token.value);
        parser_advance(parser);
        return operand;
    }
    else if (parser->current.token.type == TOKEN_REGISTER) {
        // Регистр
        operand = ast_create_register_operand(parser->ast, parser->current.token.value);
        parser_advance(parser);
        return operand;
    }
    
    parser_set_error(parser, "Ожидался операнд (регистр или непосредственное значение)", NULL);
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