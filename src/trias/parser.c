#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "parser.h"
#include "trias_instructions.h"

// Определение таблицы инструкций
#define X(name, value, operands, desc, group, handler) \
    { #name, value, operands, desc, group, handler },

static const instruction_info_t instruction_table[] = {
    INSTRUCTION_LIST(X)
    { NULL, 0, 0, NULL, NULL, NULL }  // Терминатор
};

static const int instruction_count = sizeof(instruction_table) / sizeof(instruction_table[0]) - 1;

#undef X

// Вспомогательные функции для работы с парсером
static void advance(parser_t* parser);
static bool check(parser_t* parser, token_type_t type);
static bool match(parser_t* parser, token_type_t type);

// Функции обработки ошибок
static void error(parser_t* parser, const char* message);
static void error_at_current(parser_t* parser, const char* message);
static void error_at(parser_t* parser, const token_t* token, const char* message);

// Функции разбора
static ast_node_t* parse_instruction_or_directive(parser_t* parser);
static ast_node_t* parse_instruction(parser_t* parser, const instruction_info_t* instr);
static ast_node_t* parse_directive(parser_t* parser);
static ast_node_t* parse_label(parser_t* parser);
static ast_node_t* parse_operand(parser_t* parser);
static ast_node_t* parse_number(parser_t* parser);
static ast_node_t* parse_string(parser_t* parser);

// Функции для работы с метками
static bool add_label(symbol_table_t* table, const char* name, int address, int line);
static const label_info_t* find_label(const symbol_table_t* table, const char* name);
static bool is_valid_label_name(const char* name, size_t len);
static void free_labels(symbol_table_t* table);

// Функции поиска и отладки
static const instruction_info_t* find_instruction(const char* name, size_t length);
static void print_indent(int level);
static void print_ast(const ast_node_t* node, int level);

// Объявления вспомогательных функций
static void error(parser_t* parser, const char* message);
static void error_at_current(parser_t* parser, const char* message);
static ast_node_t* parse_number(parser_t* parser);
static ast_node_t* parse_string(parser_t* parser);
static ast_node_t* parse_operand(parser_t* parser);

// Прототипы функций
static ast_node_t* parse_instruction_or_directive(parser_t* parser);

// Разбор операнда
static ast_node_t* parse_operand(parser_t* parser) {
    printf("DEBUG: parse_operand: начало разбора операнда\n");
    fflush(stdout);
    
    printf("DEBUG: parse_operand: текущий токен: тип=%d, значение='%.*s'\n",
           parser->current.type, (int)parser->current.length, parser->current.start);
    fflush(stdout);
    
    // Определяем тип операнда и режим адресации
    addr_mode_t addr_mode = ADDR_MODE_REGISTER;  // По умолчанию регистровый режим
    bool has_prefix = false;
    
    // Проверяем префиксы режимов адресации
    if (match(parser, TOKEN_HASH)) {
        printf("DEBUG: parse_operand: обнаружен префикс #\n");
        fflush(stdout);
        addr_mode = ADDR_MODE_IMMEDIATE;
        has_prefix = true;
    } else if (match(parser, TOKEN_AT)) {
        printf("DEBUG: parse_operand: обнаружен префикс @\n");
        fflush(stdout);
        addr_mode = ADDR_MODE_INDIRECT;
        has_prefix = true;
    }
    
    // Создаем узел AST после всех проверок
    ast_node_t* operand = NULL;
    
    // Разбираем значение операнда
    if (check(parser, TOKEN_NUMBER)) {
        // Числа могут быть только в непосредственном режиме
        if (has_prefix && addr_mode != ADDR_MODE_IMMEDIATE) {
            error_at_current(parser, "Число может использоваться только в непосредственном режиме");
            return NULL;
        }
        addr_mode = ADDR_MODE_IMMEDIATE;  // Числа всегда в непосредственном режиме
        
        operand = create_ast_node(NODE_NUMBER);
        if (!operand) {
            error(parser, "Не удалось выделить память");
            return NULL;
        }
        operand->value.number = parser->current.value.number;
        operand->addr_mode = addr_mode;
        advance(parser);
    } else if (check(parser, TOKEN_IDENTIFIER)) {
        const char* id = parser->current.start;
        size_t len = parser->current.length;
        
        // Проверяем, регистр ли это
        if (len >= 2 && id[0] == 'R' && isdigit(id[1])) {
            // Регистры не могут быть в непосредственном режиме
            if (has_prefix && addr_mode == ADDR_MODE_IMMEDIATE) {
                error_at_current(parser, "Регистр не может быть непосредственным значением");
                return NULL;
            }
            // Если нет префикса или префикс @, используем соответствующий режим
            if (!has_prefix) {
                addr_mode = ADDR_MODE_REGISTER;
            }
            
            operand = create_ast_node(NODE_REGISTER);
            if (!operand) {
                error(parser, "Не удалось выделить память");
                return NULL;
            }
            operand->value.number = atoi(id + 1);
            if (operand->value.number < 0 || operand->value.number > 7) {
                error_at_current(parser, "Недопустимый номер регистра");
                free_ast(operand);
                return NULL;
            }
        } else {
            // Метки по умолчанию в непосредственном режиме
            if (!has_prefix) {
                addr_mode = ADDR_MODE_IMMEDIATE;
            }
            
            operand = create_ast_node(NODE_IDENTIFIER);
            if (!operand) {
                error(parser, "Не удалось выделить память");
                return NULL;
            }
            operand->value.identifier.text = strndup(id, len);
            if (!operand->value.identifier.text) {
                error(parser, "Не удалось выделить память");
                free_ast(operand);
                return NULL;
            }
            
            // Проверяем существование метки
            if (!find_label(&parser->symbols, operand->value.identifier.text)) {
                printf("DEBUG: parse_operand: метка '%s' не найдена\n", operand->value.identifier.text);
                fflush(stdout);
            }
        }
        operand->addr_mode = addr_mode;
        advance(parser);
    } else {
        error_at_current(parser, "Ожидалось число, регистр или метка");
        return NULL;
    }
    
    printf("DEBUG: parse_operand: успешно разобран операнд\n");
    fflush(stdout);
    return operand;
}

// Проверка корректности имени метки
bool is_valid_label_name(const char* name, size_t len) {
    if (!name || len == 0 || len >= MAX_LABEL_LENGTH) return false;
    
    // Первый символ должен быть буквой
    if (!isalpha(name[0])) return false;
    
    // Остальные символы - буквы, цифры или подчеркивание
    for (size_t i = 1; i < len; i++) {
        if (!isalnum(name[i]) && name[i] != '_') return false;
    }
    
    return true;
}

// Поиск метки в таблице символов
const label_info_t* find_label(const symbol_table_t* table, const char* name) {
    if (!table || !name) return NULL;
    
    const label_info_t* current = table->labels;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Добавление метки в таблицу символов
bool add_label(symbol_table_t* table, const char* name, int address, int line) {
    if (!table || !name) return false;
    
    // Проверяем, нет ли уже такой метки
    if (find_label(table, name)) {
        return false;
    }
    
    // Создаем новую метку
    label_info_t* label = (label_info_t*)malloc(sizeof(label_info_t));
    if (!label) return false;
    
    // Копируем имя метки
    label->name = strdup(name);
    if (!label->name) {
        free(label);
        return false;
    }
    
    label->address = address;
    label->line = line;
    label->next = table->labels;
    table->labels = label;
    
    return true;
}

// Освобождение памяти таблицы символов
void free_labels(symbol_table_t* table) {
    if (!table) return;
    
    label_info_t* current = table->labels;
    while (current) {
        label_info_t* next = current->next;
        if (current->name) {
            free(current->name);
            current->name = NULL;
        }
        free(current);
        current = next;
    }
    table->labels = NULL;
    table->label_count = 0;
    table->current_address = 0;
}

// Инициализация парсера
void parser_init(parser_t* parser, lexer_t* lexer) {
    if (!parser || !lexer) return;
    
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->error_message = NULL;
    
    // Инициализируем таблицу символов
    parser->symbols.labels = NULL;
    parser->symbols.label_count = 0;
    parser->symbols.current_address = 0;
    
    // Инициализируем токены
    memset(&parser->current, 0, sizeof(token_t));
    memset(&parser->previous, 0, sizeof(token_t));
    
    // Получаем первый токен
    advance(parser);
}

// Вспомогательные функции
void advance(parser_t* parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
}

bool check(parser_t* parser, token_type_t type) {
    return parser->current.type == type;
}

bool match(parser_t* parser, token_type_t type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

// Обработка ошибок
static void error_at(parser_t* parser, const token_t* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    
    // Освобождаем старое сообщение об ошибке
    if (parser->error_message) {
        free((void*)parser->error_message);
        parser->error_message = NULL;
    }
    
    // Создаем копию нового сообщения об ошибке
    parser->error_message = strdup(message);
    if (!parser->error_message) {
        fprintf(stderr, "Не удалось выделить память для сообщения об ошибке\n");
        return;
    }
    
    fprintf(stderr, "[строка %d] Ошибка", token->line);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " в конце файла");
    } else if (token->type == TOKEN_ERROR) {
        // Ничего
    } else {
        fprintf(stderr, " в '%.*s'", (int)token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
}

static void error(parser_t* parser, const char* message) {
    error_at(parser, &parser->previous, message);
}

static void error_at_current(parser_t* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

void parser_error(parser_t* parser, const char* message) {
    error_at(parser, &parser->previous, message);
}

void parser_synchronize(parser_t* parser) {
    parser->panic_mode = false;
    
    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;
        
        switch (parser->current.type) {
            case TOKEN_DIR_ORG:
            case TOKEN_DIR_DB:
            case TOKEN_DIR_DW:
            case TOKEN_DIR_DS:
                return;
            default:
                advance(parser);
        }
    }
}

// Разбор числа
static ast_node_t* parse_number(parser_t* parser) {
    ast_node_t* node = create_ast_node(NODE_NUMBER);
    if (!node) {
        error(parser, "Не удалось выделить память");
        return NULL;
    }
    node->line = parser->previous.line;
    node->value.number = parser->previous.value.number;
    return node;
}

// Разбор строки
static ast_node_t* parse_string(parser_t* parser) {
    ast_node_t* node = create_ast_node(NODE_STRING);
    if (!node) {
        error(parser, "Не удалось выделить память");
        return NULL;
    }
    node->line = parser->previous.line;
    node->value.string.text = strdup(parser->previous.value.string);
    if (!node->value.string.text) {
        error(parser, "Не удалось выделить память");
        free(node);
        return NULL;
    }
    return node;
}

// Разбор инструкции
ast_node_t* parse_instruction(parser_t* parser, const instruction_info_t* instr) {
    printf("DEBUG: parse_instruction: начало разбора инструкции '%s'\n", instr->name);
    fflush(stdout);
    
    ast_node_t* node = create_ast_node(NODE_INSTRUCTION);
    if (!node) {
        error(parser, "Не удалось выделить память");
        return NULL;
    }
    
    node->line = parser->previous.line;
    node->value.instruction.opcode = instr->type;
    node->value.instruction.name = strdup(instr->name);
    if (!node->value.instruction.name) {
        error(parser, "Не удалось выделить память");
        free_ast(node);
        return NULL;
    }
    
    // Разбираем операнды
    for (int i = 0; i < instr->operand_count; i++) {
        printf("DEBUG: parse_instruction: разбор операнда %d\n", i);
        fflush(stdout);
        
        if (i > 0) {
            if (!match(parser, TOKEN_COMMA)) {
                error_at_current(parser, "Ожидалась запятая");
                free_ast(node);
                return NULL;
            }
            
            // Пропускаем пробелы после запятой
            while (check(parser, TOKEN_NEWLINE)) {
                advance(parser);
            }
        }
        
        // Проверяем наличие операнда
        if (check(parser, TOKEN_NEWLINE) || check(parser, TOKEN_EOF)) {
            printf("DEBUG: parse_instruction: обнаружен конец строки или файла\n");
            fflush(stdout);
            error_at_current(parser, "Ожидался операнд");
            free_ast(node);
            return NULL;
        }
        
        printf("DEBUG: parse_instruction: текущий токен перед разбором операнда: тип=%d\n", parser->current.type);
        fflush(stdout);
        
        ast_node_t* operand = parse_operand(parser);
        if (!operand) {
            printf("DEBUG: parse_instruction: операнд вернул NULL, освобождаем узел инструкции\n");
            fflush(stdout);
            free_ast(node);
            return NULL;
        }
        
        node->value.instruction.operands[i] = operand;
    }
    
    // Проверяем конец строки
    printf("DEBUG: parse_instruction: проверка конца строки, текущий токен: тип=%d\n", parser->current.type);
    fflush(stdout);
    
    if (!match(parser, TOKEN_NEWLINE) && !check(parser, TOKEN_EOF)) {
        error_at_current(parser, "Ожидался перевод строки");
        free_ast(node);
        return NULL;
    }
    
    printf("DEBUG: parse_instruction: успешное завершение разбора\n");
    fflush(stdout);
    return node;
}

// Разбор директивы
ast_node_t* parse_directive(parser_t* parser) {
    ast_node_t* node = create_ast_node(NODE_DIRECTIVE);
    if (!node) {
        error(parser, "Не удалось выделить память");
        return NULL;
    }
    
    node->line = parser->previous.line;
    node->value.directive.directive_type = parser->previous.type;
    
    switch (parser->previous.type) {
        case TOKEN_DIR_ORG:
            if (!match(parser, TOKEN_NUMBER)) {
                error_at_current(parser, "Ожидалось число после .org");
                free_ast(node);
                return NULL;
            }
            node->value.directive.value.address = parser->previous.value.number;
            break;
            
        case TOKEN_DIR_DB:
        case TOKEN_DIR_DW:
            if (!match(parser, TOKEN_NUMBER)) {
                error_at_current(parser, "Ожидалось число после директивы");
                free_ast(node);
                return NULL;
            }
            node->value.directive.value.value = parser->previous.value.number;
            break;
            
        case TOKEN_DIR_DS:
            if (!match(parser, TOKEN_STRING)) {
                error_at_current(parser, "Ожидалась строка после .ds");
                free_ast(node);
                return NULL;
            }
            node->value.directive.value.string = strdup(parser->previous.value.string);
            if (!node->value.directive.value.string) {
                error(parser, "Не удалось выделить память");
                free_ast(node);
                return NULL;
            }
            break;
            
        default:
            error_at_current(parser, "Неизвестная директива");
            free_ast(node);
            return NULL;
    }
    
    // Проверяем конец строки
    if (!match(parser, TOKEN_NEWLINE)) {
        error_at_current(parser, "Ожидался перевод строки");
        free_ast(node);
        return NULL;
    }
    
    return node;
}

// Разбор метки
ast_node_t* parse_label(parser_t* parser) {
    printf("DEBUG: parse_label: начало разбора метки\n");
    fflush(stdout);
    
    // Сохраняем имя метки из previous токена
    const char* label_name = parser->previous.start;
    size_t label_length = parser->previous.length;
    
    printf("DEBUG: parse_label: метка '%.*s'\n", (int)label_length, label_name);
    fflush(stdout);
    
    // Проверяем корректность имени метки
    if (!is_valid_label_name(label_name, label_length)) {
        error_at_current(parser, "Некорректное имя метки");
        return NULL;
    }
    
    // Создаем узел метки
    ast_node_t* node = create_ast_node(NODE_LABEL);
    if (!node) {
        error(parser, "Не удалось выделить память");
        return NULL;
    }
    
    node->line = parser->previous.line;
    node->value.label.text = strndup(label_name, label_length);
    if (!node->value.label.text) {
        error(parser, "Не удалось выделить память");
        free_ast(node);
        return NULL;
    }
    
    // Проверяем двоеточие (текущий токен должен быть ':')
    if (!match(parser, TOKEN_COLON)) {
        error_at_current(parser, "Ожидалось ':'");
        free_ast(node);
        return NULL;
    }
    
    // Добавляем метку в таблицу символов
    if (!add_label(&parser->symbols, node->value.label.text, parser->symbols.current_address, node->line)) {
        error(parser, "Метка уже определена");
        free_ast(node);
        return NULL;
    }
    
    printf("DEBUG: parse_label: метка добавлена в таблицу символов\n");
    fflush(stdout);
    
    // Пропускаем пробелы и переводы строк после метки
    while (match(parser, TOKEN_NEWLINE)) {
        continue;
    }
    
    // После метки может быть инструкция, директива или конец файла
    if (!check(parser, TOKEN_EOF)) {
        ast_node_t* instruction = parse_instruction_or_directive(parser);
        if (instruction) {
            node->next = instruction;
        } else if (parser->had_error) {
            // Если произошла ошибка при разборе инструкции,
            // освобождаем узел метки
            free_ast(node);
            return NULL;
        }
    }
    
    return node;
}

// Разбор инструкции или директивы
static ast_node_t* parse_instruction_or_directive(parser_t* parser) {
    printf("DEBUG: parse_instruction_or_directive: начало\n");
    fflush(stdout);
    
    // Проверяем, не директива ли это
    if (parser->current.type == TOKEN_DIR_ORG ||
        parser->current.type == TOKEN_DIR_DB ||
        parser->current.type == TOKEN_DIR_DW ||
        parser->current.type == TOKEN_DIR_DS) {
        advance(parser);
        return parse_directive(parser);
    }
    
    // Если не директива, то это должна быть инструкция
    if (parser->current.type != TOKEN_IDENTIFIER) {
        error_at_current(parser, "Ожидалась инструкция или директива");
        return NULL;
    }
    
    // Поиск инструкции
    printf("DEBUG: parse_instruction_or_directive: поиск инструкции '%.*s'\n",
           (int)parser->current.length, parser->current.start);
    fflush(stdout);
    
    const instruction_info_t* instr = find_instruction(parser->current.start, parser->current.length);
    if (!instr) {
        error_at_current(parser, "Неизвестная инструкция");
        return NULL;
    }
    
    printf("DEBUG: parse_instruction_or_directive: найдена инструкция '%s'\n", instr->name);
    fflush(stdout);
    
    advance(parser);
    ast_node_t* node = parse_instruction(parser, instr);
    if (!node && parser->had_error) {
        // Если произошла ошибка при разборе инструкции, память уже освобождена в parse_instruction
        return NULL;
    }
    return node;
}

// Разбор программы
ast_node_t* parse_program(parser_t* parser) {
    printf("DEBUG: parse_program: начало разбора программы\n");
    fflush(stdout);
    
    ast_node_t* first = NULL;
    ast_node_t* last = NULL;
    
    while (!check(parser, TOKEN_EOF)) {
        printf("DEBUG: parse_program: разбор следующего токена\n");
        fflush(stdout);
        
        // Пропускаем пустые строки
        if (match(parser, TOKEN_NEWLINE)) {
            printf("DEBUG: parse_program: пропущена пустая строка\n");
            fflush(stdout);
            continue;
        }
        
        // Разбираем инструкцию, директиву или метку
        ast_node_t* node = NULL;
        
        if (check(parser, TOKEN_IDENTIFIER)) {
            printf("DEBUG: parse_program: обнаружен идентификатор '%.*s'\n", 
                   (int)parser->current.length, parser->current.start);
            fflush(stdout);
            
            // Сохраняем текущий токен и делаем его previous
            token_t identifier = parser->current;
            advance(parser);  // Переходим к следующему токену
            
            // Проверяем, является ли это меткой
            if (check(parser, TOKEN_COLON)) {
                printf("DEBUG: parse_program: обнаружена метка\n");
                fflush(stdout);
                
                // Восстанавливаем состояние для parse_label
                parser->previous = identifier;
                node = parse_label(parser);
            } else {
                // Возвращаем токен обратно для parse_instruction_or_directive
                parser->current = identifier;
                node = parse_instruction_or_directive(parser);
                if (!node && parser->had_error) {
                    free_ast(first);
                    return NULL;
                }
            }
        } else if (check(parser, TOKEN_DIR_ORG) || 
                  check(parser, TOKEN_DIR_DB) || 
                  check(parser, TOKEN_DIR_DW) || 
                  check(parser, TOKEN_DIR_DS)) {
            advance(parser);
            node = parse_directive(parser);
        } else {
            error_at_current(parser, "Ожидалась инструкция, метка или директива");
            free_ast(first);
            return NULL;
        }
        
        if (parser->had_error) {
            free_ast(first);
            return NULL;
        }
        
        if (node) {
            if (!first) {
                first = node;
                last = node;
            } else {
                // Находим последний узел в цепочке
                ast_node_t* current = node;
                while (current->next) {
                    current = current->next;
                }
                last->next = node;
                last = current;
            }
        }
    }
    
    printf("DEBUG: parse_program: успешное завершение разбора\n");
    fflush(stdout);
    
    return first;
}

// Проверка наличия ошибок
bool parser_had_error(const parser_t* parser) {
    return parser->had_error;
}

// Освобождение ресурсов парсера
void parser_free(parser_t* parser) {
    if (!parser) return;
    
    // Освобождаем таблицу символов
    free_labels(&parser->symbols);
    
    // Освобождаем сообщение об ошибке
    if (parser->error_message) {
        free((void*)parser->error_message);
        parser->error_message = NULL;
    }
    
    // Очищаем все поля
    parser->lexer = NULL;  // Не освобождаем lexer, он управляется извне
    parser->had_error = false;
    parser->panic_mode = false;
    
    // Очищаем токены
    if (parser->current.type == TOKEN_STRING && parser->current.value.string) {
        free(parser->current.value.string);
        parser->current.value.string = NULL;
    }
    if (parser->previous.type == TOKEN_STRING && parser->previous.value.string) {
        free(parser->previous.value.string);
        parser->previous.value.string = NULL;
    }
    
    // Очищаем все поля токенов
    memset(&parser->current, 0, sizeof(token_t));
    memset(&parser->previous, 0, sizeof(token_t));
}

// Функция для вывода пробелов для визуализации уровней вложенности
static void print_indent(int level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
}

// Вывод AST
void print_ast(const ast_node_t* node, int level) {
    if (!node) return;
    
    // Отступ
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    
    // Вывод узла
    switch (node->type) {
        case NODE_INSTRUCTION:
            printf("Инструкция '%s' (опкод=%d)\n", 
                   node->value.instruction.name,
                   node->value.instruction.opcode);
            // Вывод операндов
            for (int i = 0; i < 2; i++) {
                if (node->value.instruction.operands[i]) {
                    print_ast(node->value.instruction.operands[i], level + 1);
                }
            }
            break;
            
        case NODE_LABEL:
            printf("Метка '%s'\n", node->value.label.text);
            break;
            
        case NODE_DIRECTIVE:
            printf("Директива: ");
            switch (node->value.directive.directive_type) {
                case TOKEN_DIR_ORG:
                    printf(".org %d\n", node->value.directive.value.address);
                    break;
                case TOKEN_DIR_DB:
                    printf(".db %d\n", node->value.directive.value.value);
                    break;
                case TOKEN_DIR_DW:
                    printf(".dw %d\n", node->value.directive.value.value);
                    break;
                case TOKEN_DIR_DS:
                    printf(".ds \"%s\"\n", node->value.directive.value.string);
                    break;
                default:
                    printf("неизвестная директива\n");
                    break;
            }
            break;
            
        case NODE_NUMBER:
            printf("Число: %d\n", node->value.number);
            break;
            
        case NODE_STRING:
            printf("Строка: \"%s\"\n", node->value.string.text);
            break;
            
        case NODE_IDENTIFIER:
            printf("Идентификатор: %s\n", node->value.identifier.text);
            break;
            
        case NODE_REGISTER:
            printf("Регистр: R%d\n", node->value.number);
            break;
            
        default:
            printf("Неизвестный тип узла\n");
            break;
    }
    
    // Вывод следующего узла
    if (node->next) {
        print_ast(node->next, level);
    }
}

// Поиск инструкции по имени
static const instruction_info_t* find_instruction(const char* name, size_t length) {
    printf("DEBUG: Поиск инструкции '%.*s'\n", (int)length, name);
    fflush(stdout);
    
    printf("DEBUG: Размер таблицы: %d элементов\n", instruction_count);
    fflush(stdout);
    
    printf("DEBUG: Доступные инструкции:\n");
    for (int i = 0; instruction_table[i].name; i++) {
        printf("DEBUG: [%d] '%s' (тип=%d)\n", i, instruction_table[i].name, instruction_table[i].type);
        fflush(stdout);
    }
    
    for (int i = 0; instruction_table[i].name; i++) {
        printf("DEBUG: Сравниваем с '%s'\n", instruction_table[i].name);
        fflush(stdout);
        if (strlen(instruction_table[i].name) == length &&
            strncmp(instruction_table[i].name, name, length) == 0) {
            printf("DEBUG: Нашли инструкцию '%s' с типом %d\n", 
                   instruction_table[i].name, instruction_table[i].type);
            fflush(stdout);
            return &instruction_table[i];
        }
    }
    
    printf("DEBUG: Инструкция '%.*s' не найдена\n", (int)length, name);
    fflush(stdout);
    return NULL;
}