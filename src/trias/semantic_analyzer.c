#include "semantic_analyzer.h"
#include "instruction_defs.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_MSG_SIZE 256
#define LOG_SUBSYS LOG_SEMANTIC
#define HASH_SIZE 256

// Простая хэш-функция для строк
static unsigned int hash_string(const char* str) {
    unsigned int hash = 0;
    while (*str) {
        hash = hash * 31 + *str++;
    }
    return hash % HASH_SIZE;
}

// Получение допустимых режимов адресации для инструкции
static int get_allowed_addr_modes(const char* instr_name) {
    const instruction_info_t* info = NULL;
    
    // Ищем инструкцию в таблице
    for (size_t i = 0; i < sizeof(instruction_table) / sizeof(instruction_table[0]); i++) {
        if (strcmp(instruction_table[i].name, instr_name) == 0) {
            info = &instruction_table[i];
            break;
        }
    }
    
    if (info) {
        return info->addr_mode;
    }
    
    return -1;  // Неизвестная инструкция
}

// Структура семантического анализатора
struct semantic_analyzer {
    symbol_table_t* symbols;      // Таблица символов
    char* error_message;          // Сообщение об ошибке
    source_loc_t error_loc;       // Позиция ошибки
};

// Создание анализатора
semantic_analyzer_t* semantic_analyzer_create(void) {
    semantic_analyzer_t* analyzer = calloc(1, sizeof(semantic_analyzer_t));
    if (!analyzer) return NULL;
    
    return analyzer;
}

// Установка таблицы символов
void semantic_analyzer_set_symbol_table(semantic_analyzer_t* analyzer, symbol_table_t* table) {
    if (!analyzer) return;
    analyzer->symbols = table;
}

// Установка сообщения об ошибке
void semantic_analyzer_set_error(semantic_analyzer_t* analyzer, const char* message, const source_loc_t* loc) {
    if (!analyzer || !message) return;
    
    free(analyzer->error_message);
    
    if (loc) {
        char buf[ERROR_MSG_SIZE];
        snprintf(buf, sizeof(buf), "%s (строка %zu, позиция %zu)", 
                message, loc->line, loc->column);
        analyzer->error_message = strdup(buf);
    }
    else {
        analyzer->error_message = strdup(message);
    }
}

// Проверка директивы
static bool semantic_analyzer_check_directive(semantic_analyzer_t* analyzer, ast_directive_t* directive) {
    if (!analyzer || !directive) return false;
    
    LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Проверка директивы '%s' (строка %zu, позиция %zu)", 
              directive->name, directive->base.loc.line, directive->base.loc.column);
    
    // Проверяем, является ли директива локальной
    bool is_local = directive->name && directive->name[0] == '.';
    LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Директива '%s' является %s", 
              directive->name, is_local ? "локальной" : "глобальной");
    
    // Проверяем количество аргументов
    LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Количество аргументов: %zu", directive->arg_count);
    
    // Проверяем каждый аргумент
    for (size_t i = 0; i < directive->arg_count; i++) {
        ast_operand_t* arg = (ast_operand_t*)directive->args[i];
        LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Аргумент %zu: тип %d, текст '%s'", 
                 i, arg->type, arg->source_text ? arg->source_text : "NULL");
    }
    
    // Проверяем количество аргументов
    if (strcmp(directive->name, "org") == 0) {
        if (directive->arg_count != 1) {
            semantic_analyzer_set_error(analyzer, "Директива .org требует один аргумент", &directive->base.loc);
            return false;
        }
        
        // Проверяем тип аргумента
        ast_operand_t* arg = (ast_operand_t*)directive->args[0];
        if (arg->type != OPERAND_IMMEDIATE) {
            semantic_analyzer_set_error(analyzer, "Аргумент директивы .org должен быть числом", &arg->base.loc);
            return false;
        }
        
        // Проверяем значение
        if (arg->immediate < 0) {
            semantic_analyzer_set_error(analyzer, "Адрес не может быть отрицательным", &arg->base.loc);
            return false;
        }
    }
    else if (strcmp(directive->name, "space") == 0) {
        if (directive->arg_count != 1) {
            semantic_analyzer_set_error(analyzer, "Директива .space требует один аргумент", &directive->base.loc);
            return false;
        }
        
        // Проверяем тип аргумента
        ast_operand_t* arg = (ast_operand_t*)directive->args[0];
        if (arg->type != OPERAND_IMMEDIATE) {
            semantic_analyzer_set_error(analyzer, "Аргумент директивы .space должен быть числом", &arg->base.loc);
            return false;
        }
        
        // Проверяем значение
        if (arg->immediate <= 0) {
            semantic_analyzer_set_error(analyzer, "Размер должен быть положительным", &arg->base.loc);
            return false;
        }
    }
    else if (strcmp(directive->name, "align") == 0) {
        if (directive->arg_count != 1) {
            semantic_analyzer_set_error(analyzer, "Директива .align требует один аргумент", &directive->base.loc);
            return false;
        }
        
        // Проверяем тип аргумента
        ast_operand_t* arg = (ast_operand_t*)directive->args[0];
        if (arg->type != OPERAND_IMMEDIATE) {
            semantic_analyzer_set_error(analyzer, "Аргумент директивы .align должен быть числом", &arg->base.loc);
            return false;
        }
        
        // Проверяем значение
        if (arg->immediate <= 0) {
            semantic_analyzer_set_error(analyzer, "Выравнивание должно быть положительным", &arg->base.loc);
            return false;
        }
    }
    else if (strcmp(directive->name, "trit") == 0) {
        if (directive->arg_count == 0) {
            semantic_analyzer_set_error(analyzer, "Директива .trit требует хотя бы один аргумент", &directive->base.loc);
            return false;
        }
        
        // Проверяем каждый аргумент
        for (size_t i = 0; i < directive->arg_count; i++) {
            ast_operand_t* arg = (ast_operand_t*)directive->args[i];
            if (arg->type != OPERAND_IMMEDIATE) {
                semantic_analyzer_set_error(analyzer, "Аргументы директивы .trit должны быть числами", &arg->base.loc);
                return false;
            }
            
            // Проверяем значение трита (-1, 0, +1)
            if (arg->immediate < TRIT_NEGATIVE || arg->immediate > TRIT_POSITIVE) {
                semantic_analyzer_set_error(analyzer, "Значение трита должно быть -1, 0 или +1", &arg->base.loc);
                return false;
            }
            
            // Проверяем троичное представление, если оно есть
            if (arg->source_text && strncmp(arg->source_text, "0t", 2) == 0) {
                size_t trit_count = strlen(arg->source_text) - 2;  // Вычитаем "0t"
                LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Проверка троичного трита: %s (длина %zu)", 
                         arg->source_text, trit_count);
                
                // Для трита должен быть только один символ после 0t
                if (trit_count != 1) {
                    semantic_analyzer_set_error(analyzer, "Троичное представление трита должно содержать 1 трит", &arg->base.loc);
                    return false;
                }
                
                // Проверяем корректность трита
                char trit = arg->source_text[2];
                if (trit != '+' && trit != '0' && trit != '-') {
                    semantic_analyzer_set_error(analyzer, "Некорректный символ в троичном числе (допустимы только +, 0, -)", &arg->base.loc);
                    return false;
                }
            }
        }
    }
    else if (strcmp(directive->name, "tryte") == 0) {
        if (directive->arg_count == 0) {
            semantic_analyzer_set_error(analyzer, "Директива .tryte требует хотя бы один аргумент", &directive->base.loc);
            return false;
        }
        
        // Проверяем каждый аргумент
        for (size_t i = 0; i < directive->arg_count; i++) {
            ast_operand_t* arg = (ast_operand_t*)directive->args[i];
            if (arg->type != OPERAND_IMMEDIATE) {
                semantic_analyzer_set_error(analyzer, "Аргументы директивы .tryte должны быть числами", &arg->base.loc);
                return false;
            }
            
            // Проверяем значение трайта (-364..+364)
            if (arg->immediate < -364 || arg->immediate > 364) {
                semantic_analyzer_set_error(analyzer, "Значение трайта должно быть в диапазоне от -364 до +364", &arg->base.loc);
                return false;
            }
            
            LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Проверка троичного трайта: %s", arg->source_text ? arg->source_text : "NULL");
            
            // Проверяем количество тритов в троичном представлении
            if (!arg->source_text) {
                semantic_analyzer_set_error(analyzer, "Отсутствует исходный текст числа", &arg->base.loc);
                return false;
            }
            
            if (strncmp(arg->source_text, "0t", 2) == 0) {
                size_t trit_count = strlen(arg->source_text) - 2;  // Вычитаем "0t"
                LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Количество тритов: %zu", trit_count);
                if (trit_count != TRITS_PER_TRYTE) {
                    semantic_analyzer_set_error(analyzer, "Троичное представление трайта должно содержать 6 тритов", &arg->base.loc);
                    return false;
                }
                
                // Проверяем корректность каждого трита
                for (size_t j = 2; j < strlen(arg->source_text); j++) {
                    char trit = arg->source_text[j];
                    if (trit != '+' && trit != '0' && trit != '-') {
                        semantic_analyzer_set_error(analyzer, "Некорректный символ в троичном числе (допустимы только +, 0, -)", &arg->base.loc);
                        return false;
                    }
                }
            }
        }
    }
    else if (strcmp(directive->name, "scope") == 0) {
        LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Обработка директивы .scope");
        if (directive->arg_count != 1) {
            semantic_analyzer_set_error(analyzer, "Директива .scope требует один аргумент", &directive->base.loc);
            return false;
        }
        
        // Проверяем тип аргумента
        ast_operand_t* arg = (ast_operand_t*)directive->args[0];
        if (arg->type != OPERAND_LABEL) {
            semantic_analyzer_set_error(analyzer, "Аргумент директивы .scope должен быть именем", &arg->base.loc);
            return false;
        }
        
        LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Создание новой области видимости '%s'", arg->label);
        
        // Создаем новую область видимости
        struct scope* scope = symbol_table_push_scope(analyzer->symbols, arg->label);
        if (!scope) {
            semantic_analyzer_set_error(analyzer, "Не удалось создать область видимости", &directive->base.loc);
            return false;
        }
        
        LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Создана новая область видимости '%s'", arg->label);
    }
    else if (strcmp(directive->name, "endscope") == 0) {
        if (directive->arg_count != 0) {
            semantic_analyzer_set_error(analyzer, "Директива .endscope не принимает аргументов", &directive->base.loc);
            return false;
        }
        
        // Проверяем, что есть текущая область видимости
        struct scope* current_scope = symbol_table_get_current_scope(analyzer->symbols);
        if (!current_scope || current_scope == analyzer->symbols->global_scope) {
            semantic_analyzer_set_error(analyzer, "Нет активной области видимости для закрытия", &directive->base.loc);
            return false;
        }
        
        // Закрываем текущую область видимости
        symbol_table_pop_scope(analyzer->symbols);
        LOG_DEBUG(LOG_SUBSYS, "Закрыта область видимости");
    }
    
    return true;
}

// Проверка режима адресации операнда
static bool check_operand_addressing_mode(semantic_analyzer_t* analyzer,
                                        ast_operand_t* operand,
                                        int allowed_modes,
                                        char* error) {
    (void)analyzer; // Подавляем предупреждение о неиспользуемом параметре
    
    // Определяем режим адресации операнда
    int operand_mode;
    
    printf("[DEBUG] Проверка режима адресации для операнда типа %d\n", operand->type);
    printf("[DEBUG] Разрешенные режимы: %d\n", allowed_modes);
    
    if (operand->type == OPERAND_IMMEDIATE) {
        operand_mode = 1 << 0;  // ADDR_MODE_IMM
        printf("[DEBUG] Определен непосредственный режим (маска %d)\n", operand_mode);
    }
    else if (operand->type == OPERAND_REGISTER) {
        operand_mode = 1 << 1;  // ADDR_MODE_REG
        printf("[DEBUG] Определен регистровый режим (маска %d)\n", operand_mode);
    }
    else if (operand->type == OPERAND_INDIRECT) {
        operand_mode = 1 << 2;  // ADDR_MODE_IND
        printf("[DEBUG] Определен косвенный режим (маска %d)\n", operand_mode);
    }
    else if (operand->type == OPERAND_LABEL) {
        operand_mode = 1 << 0;  // ADDR_MODE_IMM - метки обрабатываются как непосредственные значения
        printf("[DEBUG] Определен режим метки (маска %d)\n", operand_mode);
    }
    else {
        snprintf(error, ERROR_MSG_SIZE, "Неподдерживаемый тип операнда");
        printf("[DEBUG] Неподдерживаемый тип операнда: %d\n", operand->type);
        return false;
    }
    
    // Проверяем, что режим адресации разрешен
    printf("[DEBUG] Проверка режима %d против разрешенных %d (побитовое И: %d)\n", 
           operand_mode, allowed_modes, allowed_modes & operand_mode);
    if (!(allowed_modes & operand_mode)) {
        snprintf(error, ERROR_MSG_SIZE, "Недопустимый режим адресации для операнда");
        printf("[DEBUG] Режим адресации не разрешен\n");
        return false;
    }
    
    printf("[DEBUG] Режим адресации допустим\n");
    return true;
}

// Проверка инструкции
bool semantic_analyzer_check_instruction(semantic_analyzer_t* analyzer,
                                      ast_instruction_t* instruction,
                                      char* error) {
    // Получаем информацию об инструкции
    const instruction_info_t* info = NULL;
    
    // Ищем инструкцию в таблице
    for (size_t i = 0; i < sizeof(instruction_table) / sizeof(instruction_table[0]); i++) {
        if (strcmp(instruction_table[i].name, instruction->mnemonic) == 0) {
            info = &instruction_table[i];
            break;
        }
    }
    
    if (!info) {
        snprintf(error, ERROR_MSG_SIZE, "Неизвестная инструкция: %s", instruction->mnemonic);
        return false;
    }
    
    // Проверяем количество операндов
    if ((int)instruction->operand_count != info->operands) {
        snprintf(error, ERROR_MSG_SIZE, "Неверное количество операндов для %s: ожидается %d, получено %zu",
                instruction->mnemonic, info->operands, instruction->operand_count);
        return false;
    }
    
    // Получаем допустимые режимы адресации
    int allowed_modes = get_allowed_addr_modes(instruction->mnemonic);
    if (allowed_modes == -1) {
        snprintf(error, ERROR_MSG_SIZE, "Не удалось определить допустимые режимы адресации для %s",
                instruction->mnemonic);
        return false;
    }
    
    // Проверяем режимы адресации операндов
    for (size_t i = 0; i < instruction->operand_count; i++) {
        if (!check_operand_addressing_mode(analyzer, instruction->operands[i],
                                         allowed_modes, error)) {
            return false;
        }
    }
    
    return true;
}

// Проверка метки
static bool semantic_analyzer_check_label(semantic_analyzer_t* analyzer, ast_label_t* label) {
    if (!analyzer || !label) return false;

    LOG_DEBUG(LOG_SUBSYS, "=== Начало проверки метки '%s' ===", label->name);
    LOG_DEBUG(LOG_SUBSYS, "Локальная: %d, Позиция: %zu:%zu", 
              label->is_local, label->base.loc.line, label->base.loc.column);

    // Определяем, является ли метка локальной по префиксу
    bool is_local = label->name[0] == '.';
    
    // Проверяем соответствие флага is_local с префиксом
    if (is_local != label->is_local) {
        LOG_ERROR(LOG_SUBSYS, "Несоответствие типа метки '%s' её префиксу", label->name);
        semantic_analyzer_set_error(analyzer,
            "Несоответствие типа метки её префиксу", &label->base.loc);
        return false;
    }

    // Получаем текущую область видимости
    struct scope* current_scope = symbol_table_get_current_scope(analyzer->symbols);
    
    // Для локальных меток проверяем наличие области видимости
    if (is_local && !current_scope) {
        LOG_ERROR(LOG_SUBSYS, "Локальная метка '%s' определена вне области видимости", label->name);
        semantic_analyzer_set_error(analyzer,
            "Локальная метка определена вне области видимости", &label->base.loc);
        return false;
    }

    // Ищем метку в соответствующей области видимости
    symbol_t* symbol = NULL;
    if (is_local) {
        // Для локальных меток ищем только в текущей области
        symbol = symbol_table_lookup_local(analyzer->symbols, label->name);
    } else {
        // Для глобальных меток ищем в глобальной области
        symbol = symbol_table_lookup(analyzer->symbols, label->name);
    }

    if (!symbol) {
        LOG_ERROR(LOG_SUBSYS, "Метка '%s' не найдена в таблице символов", label->name);
        semantic_analyzer_set_error(analyzer,
            "Метка не найдена в таблице символов", &label->base.loc);
        return false;
    }

    // Проверяем соответствие области видимости
    bool symbol_is_local = (symbol->flags & SYMBOL_FLAG_LOCAL) != 0;
    if (is_local != symbol_is_local) {
        LOG_ERROR(LOG_SUBSYS, "Несоответствие области видимости метки '%s'", label->name);
        semantic_analyzer_set_error(analyzer,
            "Несоответствие области видимости метки", &label->base.loc);
        return false;
    }

    // Привязываем метку к текущей области
    symbol->scope = current_scope;
    LOG_DEBUG(LOG_SUBSYS, "Метка '%s' привязана к области '%s'", 
              label->name, current_scope ? current_scope->name : "global");

    return true;
}

// Проверка ссылок на метки
static bool semantic_analyzer_check_label_references(semantic_analyzer_t* analyzer, ast_program_t* program) {
    if (!analyzer || !program) return false;

    LOG_DEBUG(LOG_SUBSYS, "Начало проверки ссылок на метки (всего операторов: %zu)", 
              program->statement_count);

    // Первый проход: собираем все определения меток
    LOG_DEBUG(LOG_SUBSYS, "Первый проход: сбор определений меток");
    for (size_t i = 0; i < program->statement_count; i++) {
        ast_node_t* stmt = program->statements[i];
        if (stmt->type == AST_LABEL) {
            ast_label_t* label = (ast_label_t*)stmt;
            LOG_DEBUG(LOG_SUBSYS, "Обработка метки '%s' (строка %zu)", 
                      label->name, label->base.loc.line);
            
            // Определяем, является ли метка локальной
            bool is_local = label->name[0] == '.';
            
            // Ищем метку в соответствующей области
            symbol_t* symbol = NULL;
            if (is_local) {
                symbol = symbol_table_lookup_local(analyzer->symbols, label->name);
            } else {
                symbol = symbol_table_lookup(analyzer->symbols, label->name);
            }

            if (!symbol) {
                LOG_ERROR(LOG_SUBSYS, "Метка '%s' не найдена в таблице символов", label->name);
                semantic_analyzer_set_error(analyzer,
                    "Метка не найдена в таблице символов", &label->base.loc);
                return false;
            }

            // Помечаем метку как определенную
            symbol->flags |= SYMBOL_FLAG_DEFINED;
        }
    }

    // Второй проход: проверяем все ссылки на метки
    LOG_DEBUG(LOG_SUBSYS, "Второй проход: проверка ссылок на метки");
    for (size_t i = 0; i < program->statement_count; i++) {
        ast_node_t* stmt = program->statements[i];
        if (stmt->type != AST_INSTRUCTION) continue;

        ast_instruction_t* instr = (ast_instruction_t*)stmt;
        LOG_DEBUG(LOG_SUBSYS, "Проверка инструкции '%s' (строка %zu)", 
                  instr->mnemonic, instr->base.loc.line);

        for (size_t j = 0; j < instr->operand_count; j++) {
            ast_operand_t* op = instr->operands[j];
            if (op->type == OPERAND_LABEL) {
                LOG_DEBUG(LOG_SUBSYS, "Проверка ссылки на метку '%s'", op->label);
                
                // Определяем, является ли ссылка локальной
                bool is_local_ref = op->label[0] == '.';
                symbol_t* symbol = NULL;

                if (is_local_ref) {
                    // Для локальных меток ищем в текущей и родительских областях
                    struct scope* scope = symbol_table_get_current_scope(analyzer->symbols);
                    while (scope && !symbol) {
                        // Ищем метку в текущей области
                        struct symbol_node* node = scope->buckets[hash_string(op->label)];
                        while (node) {
                            if (strcmp(node->symbol.name, op->label) == 0) {
                                symbol = &node->symbol;
                                break;
                            }
                            node = node->next;
                        }
                        scope = scope->parent;
                    }
                } else {
                    // Для глобальных меток ищем только в глобальной области
                    symbol = symbol_table_lookup(analyzer->symbols, op->label);
                }

                if (!symbol) {
                    LOG_ERROR(LOG_SUBSYS, "Ссылка на неопределенную метку '%s'", op->label);
                    semantic_analyzer_set_error(analyzer,
                        "Ссылка на неопределенную метку", &op->base.loc);
                    return false;
                }

                // Проверяем соответствие области видимости
                bool is_local_symbol = (symbol->flags & SYMBOL_FLAG_LOCAL) != 0;
                if (is_local_symbol != is_local_ref) {
                    LOG_ERROR(LOG_SUBSYS, "Несоответствие области видимости при использовании метки '%s'", op->label);
                    semantic_analyzer_set_error(analyzer,
                        "Несоответствие области видимости метки", &op->base.loc);
                    return false;
                }

                // Проверяем доступность локальной метки
                if (is_local_ref) {
                    struct scope* current_scope = symbol_table_get_current_scope(analyzer->symbols);
                    struct scope* symbol_scope = symbol->scope;
                    
                    // Проверяем, что метка определена в текущей или родительской области
                    bool found = false;
                    bool is_child_scope = false;
                    
                    // Сначала проверяем, не является ли область метки дочерней
                    struct scope* check_scope = current_scope;
                    while (check_scope) {
                        if (check_scope == symbol_scope) {
                            found = true;
                            break;
                        }
                        check_scope = check_scope->parent;
                    }
                    
                    // Если метка не найдена в текущей или родительских областях,
                    // проверяем, не находится ли она в дочерней области
                    if (!found) {
                        check_scope = symbol_scope;
                        while (check_scope) {
                            if (check_scope == current_scope) {
                                is_child_scope = true;
                                break;
                            }
                            check_scope = check_scope->parent;
                        }
                    }
                    
                    // Ошибка, если метка в дочерней области или не найдена вообще
                    if (!found || is_child_scope) {
                        LOG_ERROR(LOG_SUBSYS, "Локальная метка '%s' недоступна в текущей области видимости (текущая область: %s, область метки: %s)", 
                                  op->label, current_scope->name, symbol_scope->name);
                        semantic_analyzer_set_error(analyzer,
                            "Локальная метка недоступна в текущей области видимости", &op->base.loc);
                        return false;
                    }
                    
                    LOG_DEBUG(LOG_SUBSYS, "Метка '%s' доступна в области '%s'", 
                              op->label, current_scope->name);
                }
                
                LOG_DEBUG(LOG_SUBSYS, "Ссылка на метку '%s' корректна", op->label);
            }
        }
    }

    LOG_DEBUG(LOG_SUBSYS, "Проверка ссылок на метки завершена успешно");
    return true;
}

// Проверка программы
bool semantic_analyzer_check_program(semantic_analyzer_t* analyzer, ast_program_t* program) {
    if (!analyzer || !program) return false;

    LOG_INFO(LOG_SUBSYS, "Начало семантического анализа программы (операторов: %zu)", 
             program->statement_count);

    // Первый проход: собираем все метки без проверки определений
    LOG_DEBUG(LOG_SUBSYS, "Первый проход: сбор всех меток");
    for (size_t i = 0; i < program->statement_count; i++) {
        ast_node_t* stmt = program->statements[i];
        LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Обработка оператора типа %d", stmt->type);
        
        if (stmt->type == AST_LABEL) {
            ast_label_t* label = (ast_label_t*)stmt;
            LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Метка '%s' (is_local в AST: %d, первый символ: '%c')", 
                      label->name, label->is_local, label->name[0]);
            
            bool is_local = label->name[0] == '.';
            
            // Проверяем, существует ли уже метка
            symbol_t* existing = NULL;
            if (is_local) {
                // Для локальных меток проверяем только в текущей области видимости
                existing = symbol_table_lookup_local(analyzer->symbols, label->name);
                LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Поиск локальной метки '%s' в текущей области: %s", 
                          label->name, existing ? "найдена" : "не найдена");
                
                // Если метка уже существует в текущей области и определена, это ошибка
                if (existing && (existing->flags & SYMBOL_FLAG_DEFINED) && 
                    existing->scope == symbol_table_get_current_scope(analyzer->symbols)) {
                    LOG_ERROR(LOG_SUBSYS, "Повторное определение локальной метки '%s' в области '%s'", 
                             label->name, existing->scope ? existing->scope->name : "global");
                    semantic_analyzer_set_error(analyzer,
                        "Повторное определение метки", &label->base.loc);
                    return false;
                }
            } else {
                // Для глобальных меток проверяем во всех областях
                existing = symbol_table_lookup(analyzer->symbols, label->name);
                LOG_DEBUG(LOG_SUBSYS, "[СЕМАНТИЧЕСКИЙ АНАЛИЗ] Поиск глобальной метки '%s': %s", 
                          label->name, existing ? "найдена" : "не найдена");
                
                if (existing && (existing->flags & SYMBOL_FLAG_DEFINED)) {
                    LOG_ERROR(LOG_SUBSYS, "Повторное определение глобальной метки '%s'", label->name);
                    semantic_analyzer_set_error(analyzer,
                        "Повторное определение метки", &label->base.loc);
                    return false;
                }
            }
            
            // Помечаем метку как определенную
            if (existing) {
                existing->flags |= SYMBOL_FLAG_DEFINED;
                LOG_DEBUG(LOG_SUBSYS, "Метка '%s' помечена как определенная", label->name);
            } else {
                // Добавляем новую метку с флагом DEFINED
                symbol_flags_t flags = SYMBOL_FLAG_DEFINED;
                if (is_local) {
                    flags |= SYMBOL_FLAG_LOCAL;
                    // Проверяем наличие текущей области видимости для локальной метки
                    struct scope* current_scope = symbol_table_get_current_scope(analyzer->symbols);
                    if (!current_scope || current_scope == analyzer->symbols->global_scope) {
                        LOG_ERROR(LOG_SUBSYS, "Попытка определить локальную метку '%s' вне области видимости", 
                                  label->name);
                        semantic_analyzer_set_error(analyzer,
                            "Локальная метка определена вне области видимости", &label->base.loc);
                        return false;
                    }
                }

                vm_error_t err = symbol_table_add(analyzer->symbols, label->name,
                                                SYMBOL_LABEL,
                                                create_tryte_from_int(0),
                                                flags,
                                                label->base.loc);
                if (err != VM_OK) {
                    LOG_ERROR(LOG_SUBSYS, "Ошибка добавления метки '%s' в таблицу символов", 
                              label->name);
                    semantic_analyzer_set_error(analyzer,
                        "Ошибка добавления метки", &label->base.loc);
                    return false;
                }
                LOG_DEBUG(LOG_SUBSYS, "Добавлена новая метка '%s'", label->name);
            }
        }
    }

    // Второй проход: проверяем все определения меток
    LOG_DEBUG(LOG_SUBSYS, "Второй проход: проверка определений меток");
    for (size_t i = 0; i < program->statement_count; i++) {
        ast_node_t* stmt = program->statements[i];
        if (stmt->type == AST_LABEL) {
            ast_label_t* label = (ast_label_t*)stmt;
            if (!semantic_analyzer_check_label(analyzer, label)) {
                LOG_ERROR(LOG_SUBSYS, "Ошибка при обработке метки в строке %zu", 
                          label->base.loc.line);
                return false;
            }
        }
    }

    // Третий проход: проверяем все остальные операторы
    LOG_DEBUG(LOG_SUBSYS, "Третий проход: проверка операторов");
    for (size_t i = 0; i < program->statement_count; i++) {
        ast_node_t* stmt = program->statements[i];
        
        switch (stmt->type) {
            case AST_LABEL:
                // Метки уже проверены
                break;
                
            case AST_DIRECTIVE:
                LOG_DEBUG(LOG_SUBSYS, "Проверка директивы в строке %zu", stmt->loc.line);
                if (!semantic_analyzer_check_directive(analyzer, (ast_directive_t*)stmt)) {
                    LOG_ERROR(LOG_SUBSYS, "Ошибка при проверке директивы");
                    return false;
                }
                break;
                
            case AST_INSTRUCTION: {
                LOG_DEBUG(LOG_SUBSYS, "Проверка инструкции в строке %zu", stmt->loc.line);
                char error[ERROR_MSG_SIZE];
                if (!semantic_analyzer_check_instruction(analyzer, (ast_instruction_t*)stmt, error)) {
                    LOG_ERROR(LOG_SUBSYS, "Ошибка при проверке инструкции: %s", error);
                    semantic_analyzer_set_error(analyzer, error, &stmt->loc);
                    return false;
                }
                break;
            }
                
            default:
                LOG_WARN(LOG_SUBSYS, "Пропуск неизвестного типа оператора %d в строке %zu", 
                         stmt->type, stmt->loc.line);
                break;
        }
    }

    // В конце проверяем все ссылки на метки
    LOG_DEBUG(LOG_SUBSYS, "Проверка ссылок на метки");
    if (!semantic_analyzer_check_label_references(analyzer, program)) {
        LOG_ERROR(LOG_SUBSYS, "Ошибка при проверке ссылок на метки");
        return false;
    }

    LOG_INFO(LOG_SUBSYS, "Семантический анализ программы завершен успешно");
    return true;
}

// Получение текста ошибки
const char* semantic_analyzer_get_error(const semantic_analyzer_t* analyzer) {
    return analyzer ? analyzer->error_message : NULL;
}

// Получение позиции ошибки
source_loc_t semantic_analyzer_get_error_location(const semantic_analyzer_t* analyzer) {
    return analyzer ? analyzer->error_loc : (source_loc_t){0, 0};
}

// Освобождение анализатора
void semantic_analyzer_destroy(semantic_analyzer_t* analyzer) {
    if (!analyzer) return;
    
    free(analyzer->error_message);
    free(analyzer);
} 