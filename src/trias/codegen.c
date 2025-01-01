#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "codegen.h"
#include "trias_instructions.h"
#include "lexer.h"
#include "parser.h"

// Инициализация таблицы меток
static void label_table_init(label_table_t* table) {
    table->count = 0;
}

// Освобождение таблицы меток
static void label_table_free(label_table_t* table) {
    for (size_t i = 0; i < table->count; i++) {
        free(table->labels[i].name);
    }
    table->count = 0;
}

// Добавление метки в таблицу
static void label_table_add(label_table_t* table, const char* name, int address) {
    if (table->count >= MAX_LABELS) {
        fprintf(stderr, "Превышено максимальное количество меток\n");
        return;
    }
    
    // Проверяем, нет ли уже такой метки
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->labels[i].name, name) == 0) {
            // Метка уже существует, обновляем адрес
            table->labels[i].address = address;
            return;
        }
    }
    
    // Добавляем новую метку
    table->labels[table->count].name = strdup(name);
    if (!table->labels[table->count].name) {
        fprintf(stderr, "Не удалось выделить память для имени метки\n");
        return;
    }
    table->labels[table->count].address = address;
    table->count++;
}

// Поиск метки в таблице
static int label_table_find(const label_table_t* table, const char* name) {
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->labels[i].name, name) == 0) {
            return table->labels[i].address;
        }
    }
    return -1; // Метка не найдена
}

// Эмиссия машинного слова
static vm_error_t emit_word(codegen_t* gen, word_t word) {
    if (gen->code_size + 3 >= gen->code_capacity) {
        size_t new_capacity = gen->code_capacity == 0 ? 1024 : gen->code_capacity * 2;
        tryte_t* new_code = realloc(gen->code, new_capacity * sizeof(tryte_t));
        if (!new_code) {
            codegen_error(gen, "Не удалось выделить память");
            return VM_ERROR_OUT_OF_MEMORY;
        }
        gen->code = new_code;
        gen->code_capacity = new_capacity;
    }
    
    // Разбиваем слово на трайты
    tryte_t trytes[3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < TRITS_PER_TRYTE; j++) {
            trytes[i].trits[j] = word.trits[i * TRITS_PER_TRYTE + j];
        }
        update_tryte_value(&trytes[i]);
    }
    
    // Копируем трайты в код
    memcpy(&gen->code[gen->code_size], trytes, 3 * sizeof(tryte_t));
    gen->code_size += 3;
    gen->current_address++;

    // Выводим отладочную информацию
    printf("Emitted word: value=%ld, trytes=[%d,%d,%d]\n",
           (long)word.value,
           trytes[0].value,
           trytes[1].value,
           trytes[2].value);

    return VM_OK;
}

// Эмиссия одной инструкции
vm_error_t codegen_emit(codegen_t* gen, const instruction_t* inst) {
    if (!gen || !inst) return VM_ERROR_INVALID_PARAMETER;
    
    // Кодируем инструкцию в машинное слово
    word_t word = encode_instruction(inst);
    return emit_word(gen, word);
}

// Генерация кода для операнда
static vm_error_t generate_operand(codegen_t* gen, ast_node_t* node, tryte_t* result) {
    if (!gen || !node || !result) return VM_ERROR_INVALID_PARAMETER;
    
    switch (node->type) {
        case NODE_NUMBER:
            *result = create_tryte_from_int(node->value.number);
            break;
            
        case NODE_LABEL:
            {
                int addr = label_table_find(&gen->labels, node->value.label.text);
                if (addr < 0) {
                    codegen_error(gen, "Неизвестная метка");
                    return VM_ERROR_INVALID_INSTRUCTION;
                }
                *result = create_tryte_from_int(addr);
            }
            break;
            
        default:
            codegen_error(gen, "Неверный тип операнда");
            return VM_ERROR_INVALID_INSTRUCTION;
    }
    
    return VM_OK;
}

// Генерация кода для инструкции
vm_error_t generate_instruction(codegen_t* gen, ast_node_t* node) {
    if (!gen || !node) return VM_ERROR_INVALID_PARAMETER;
    
    // Проверяем тип узла
    if (node->type != NODE_INSTRUCTION) {
        return VM_ERROR_INVALID_INSTRUCTION;
    }
    
    // Создаем слово для инструкции
    word_t word = {0};
    
    // Получаем базовый опкод
    int base_opcode = node->value.instruction.opcode;
    
    // Получаем режим адресации из первого операнда (если есть)
    trit_t addr_mode = ADDR_MODE_IMMEDIATE;  // По умолчанию непосредственный режим
    if (node->value.instruction.operands[0]) {
        addr_mode = node->value.instruction.operands[0]->addr_mode;
    }
    
    // Устанавливаем опкод с учетом режима адресации
    tryte_t opcode = make_opcode(addr_mode, base_opcode);
    memcpy(word.trits, opcode.trits, TRITS_PER_TRYTE);
    
    // Обрабатываем операнды
    tryte_t operand1 = TRYTE_FROM_INT(0);
    tryte_t operand2 = TRYTE_FROM_INT(0);
    
    // Генерируем код для первого операнда, если он есть
    if (node->value.instruction.operands[0]) {
        vm_error_t err = generate_operand(gen, node->value.instruction.operands[0], &operand1);
        if (err != VM_OK) return err;
        
        // Генерируем код для второго операнда, если он есть
        if (node->value.instruction.operands[1]) {
            err = generate_operand(gen, node->value.instruction.operands[1], &operand2);
            if (err != VM_OK) return err;
        }
    }
    
    // Копируем операнды в слово
    memcpy(&word.trits[TRITS_PER_TRYTE], operand1.trits, TRITS_PER_TRYTE);
    memcpy(&word.trits[2 * TRITS_PER_TRYTE], operand2.trits, TRITS_PER_TRYTE);
    
    // Обновляем значение слова
    int64_t value = 0;
    int64_t power = 1;
    for (int i = 0; i < TRITS_PER_WORD; i++) {
        value += word.trits[i] * power;
        power *= 3;
    }
    word.value = value;
    
    // Эмитируем инструкцию
    return emit_word(gen, word);
}

// Генерация кода для директивы
static vm_error_t generate_directive(codegen_t* gen, ast_node_t* node) {
    if (!gen || !node || node->type != NODE_DIRECTIVE) {
        return VM_ERROR_INVALID_PARAMETER;
    }
    
    switch (node->value.directive.directive_type) {
        case TOKEN_DIR_ORG:
            // Устанавливаем текущий адрес
            gen->current_address = node->value.directive.value.address;
            break;
            
        case TOKEN_DIR_DB:
            // Записываем один трайт
            {
                word_t word = create_word_from_int(node->value.directive.value.value);
                if (emit_word(gen, word) != VM_OK) {
                    return VM_ERROR_INVALID_ADDRESS;
                }
                gen->current_address++;
            }
            break;
            
        case TOKEN_DIR_DW:
            // Записываем два трайта
            {
                word_t word = create_word_from_int(node->value.directive.value.value);
                if (emit_word(gen, word) != VM_OK) {
                    return VM_ERROR_INVALID_ADDRESS;
                }
                gen->current_address += 2;
            }
            break;
            
        case TOKEN_DIR_DS:
            // Записываем строку
            {
                const char* str = node->value.directive.value.string;
                if (!str) {
                    return VM_ERROR_INVALID_PARAMETER;
                }
                
                size_t len = strlen(str);
                for (size_t i = 0; i < len; i++) {
                    word_t word = create_word_from_int(str[i]);
                    if (emit_word(gen, word) != VM_OK) {
                        return VM_ERROR_INVALID_ADDRESS;
                    }
                }
                gen->current_address += len;
            }
            break;
            
        default:
            return VM_ERROR_INVALID_INSTRUCTION;
    }
    
    return VM_OK;
}

// Инициализация генератора кода
vm_error_t codegen_init(codegen_t* gen) {
    gen->code = NULL;
    gen->code_size = 0;
    gen->code_capacity = 0;
    gen->current_address = 0;
    gen->had_error = false;
    label_table_init(&gen->labels);
    return VM_OK;
}

// Освобождение ресурсов генератора кода
void codegen_free(codegen_t* gen) {
    if (!gen) return;
    free(gen->code);
    label_table_free(&gen->labels);
    gen->code = NULL;
    gen->code_size = 0;
    gen->code_capacity = 0;
    gen->current_address = 0;
    gen->had_error = false;
}

// Генерация кода для программы
vm_error_t codegen_generate(codegen_t* gen, ast_node_t* ast) {
    if (!gen || !ast) return VM_ERROR_INVALID_PARAMETER;
    
    // Первый проход: собираем метки
    ast_node_t* node = ast;
    while (node) {
        if (node->type == NODE_LABEL) {
            label_table_add(&gen->labels, node->value.label.text, gen->current_address);
        }
        node = node->next;
    }
    
    // Второй проход: генерируем код
    node = ast;
    while (node) {
        vm_error_t err;
        
        switch (node->type) {
            case NODE_INSTRUCTION:
                err = generate_instruction(gen, node);
                if (err != VM_OK) return err;
                break;
                
            case NODE_DIRECTIVE:
                err = generate_directive(gen, node);
                if (err != VM_OK) return err;
                break;
                
            case NODE_LABEL:
                // Метки уже обработаны на первом проходе
                break;
                
            default:
                codegen_error(gen, "Неизвестный тип узла AST");
                return VM_ERROR_INVALID_INSTRUCTION;
        }
        
        node = node->next;
    }
    
    // Добавляем HALT в конец программы
    instruction_t halt = {
        .opcode = create_tryte_from_int(OP_HALT),
        .operand1 = create_tryte_from_int(0),
        .operand2 = create_tryte_from_int(0)
    };
    return codegen_emit(gen, &halt);
}

// Получение сгенерированного кода
const word_t* codegen_get_code(const codegen_t* gen, size_t* size) {
    if (size) {
        *size = gen->code_size / 3; // Размер в словах
    }
    return (const word_t*)gen->code;
}

// Проверка наличия ошибок
bool codegen_had_error(const codegen_t* gen) {
    return gen->had_error;
}

// Сборка программы из исходного текста
vm_error_t assemble_program(codegen_t* gen, const char* source) {
    if (!gen || !source) return VM_ERROR_INVALID_PARAMETER;
    
    // Инициализируем лексер
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    // Инициализируем парсер
    parser_t parser;
    parser_init(&parser, &lexer);
    
    // Разбираем программу
    ast_node_t* ast = parse_program(&parser);
    if (!ast) {
        if (parser_had_error(&parser)) {
            codegen_error(gen, "Ошибка при разборе программы");
            return VM_ERROR_INVALID_INSTRUCTION;
        }
        return VM_ERROR_OUT_OF_MEMORY;
    }
    
    // Генерируем код
    vm_error_t err = codegen_generate(gen, ast);
    
    // Освобождаем AST
    free_ast(ast);
    
    return err;
} 