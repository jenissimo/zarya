#include "trias_instructions.h"
#include "instructions.h"
#include "instruction_defs.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Обработчики псевдоинструкций

// MOV dst, src -> PUSH src; POP dst
vm_error_t handle_mov(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0] || !operands[1]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнды
    ast_node_t* dst = operands[0];
    ast_node_t* src = operands[1];

    // Генерируем код для MOV
    // MOV dst, src -> PUSH src; POP dst
    instruction_t inst = {0};

    // PUSH src
    inst.opcode = MAKE_OPCODE(src->addr_mode, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(src->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // POP dst
    inst.opcode = MAKE_OPCODE(dst->addr_mode, OP_POP);
    inst.operand1 = TRYTE_FROM_INT(dst->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    return VM_OK;
}

// INC reg -> PUSH reg; PUSH 1; ADD; POP reg
vm_error_t handle_inc(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнд
    ast_node_t* dst = operands[0];

    // Генерируем код для INC
    // INC dst -> PUSH dst; PUSH #1; ADD; POP dst
    instruction_t inst = {0};

    // PUSH dst
    inst.opcode = MAKE_OPCODE(dst->addr_mode, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(dst->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // PUSH #1
    inst.opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(1);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // ADD
    inst.opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_ADD);
    inst.operand1 = TRYTE_FROM_INT(0);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // POP dst
    inst.opcode = MAKE_OPCODE(dst->addr_mode, OP_POP);
    inst.operand1 = TRYTE_FROM_INT(dst->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    return VM_OK;
}

// DEC reg -> PUSH reg; PUSH 1; SUB; POP reg
vm_error_t handle_dec(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнд
    ast_node_t* dst = operands[0];

    // Генерируем код для DEC
    // DEC dst -> PUSH dst; PUSH #1; SUB; POP dst
    instruction_t inst = {0};

    // PUSH dst
    inst.opcode = MAKE_OPCODE(dst->addr_mode, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(dst->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // PUSH #1
    inst.opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(1);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // SUB
    inst.opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_SUB);
    inst.operand1 = TRYTE_FROM_INT(0);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // POP dst
    inst.opcode = MAKE_OPCODE(dst->addr_mode, OP_POP);
    inst.operand1 = TRYTE_FROM_INT(dst->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    return VM_OK;
}

// PUSHR reg -> PUSH reg
vm_error_t handle_pushr(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнд
    ast_node_t* src = operands[0];

    // Генерируем код для PUSHR
    // PUSHR src -> PUSH src
    instruction_t inst = {0};

    // PUSH src
    inst.opcode = MAKE_OPCODE(src->addr_mode, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(src->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    return VM_OK;
}

// POPR reg -> POP reg
vm_error_t handle_popr(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнд
    ast_node_t* dst = operands[0];

    // Генерируем код для POPR
    // POPR dst -> POP dst
    instruction_t inst = {0};

    // POP dst
    inst.opcode = MAKE_OPCODE(dst->addr_mode, OP_POP);
    inst.operand1 = TRYTE_FROM_INT(dst->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    return VM_OK;
}

// CLEAR n -> DROP n раз
vm_error_t handle_clear(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнд
    ast_node_t* count = operands[0];
    if (count->type != NODE_NUMBER || count->addr_mode != ADDR_MODE_IMMEDIATE) {
        return VM_ERROR_INVALID_PARAMETER;
    }

    // Генерируем код для CLEAR
    // CLEAR n -> DROP n раз
    instruction_t inst = {0};

    // DROP n раз
    for (int i = 0; i < count->value.number; i++) {
        inst.opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_DROP);
        inst.operand1 = TRYTE_FROM_INT(0);
        inst.operand2 = TRYTE_FROM_INT(0);
        if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;
    }

    return VM_OK;
}

// CMP a, b -> PUSH a; PUSH b; SUB
vm_error_t handle_cmp(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0] || !operands[1]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнды
    ast_node_t* a = operands[0];
    ast_node_t* b = operands[1];

    // Генерируем код для CMP
    // CMP a, b -> PUSH a; PUSH b; SUB
    instruction_t inst = {0};

    // PUSH a
    inst.opcode = MAKE_OPCODE(a->addr_mode, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(a->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // PUSH b
    inst.opcode = MAKE_OPCODE(b->addr_mode, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(b->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    // SUB
    inst.opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_SUB);
    inst.operand1 = TRYTE_FROM_INT(0);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    return VM_OK;
}

// TEST val -> PUSH val
vm_error_t handle_test(codegen_t* gen, ast_node_t** operands) {
    if (!gen || !operands || !operands[0]) return VM_ERROR_INVALID_PARAMETER;

    // Получаем операнд
    ast_node_t* value = operands[0];

    // Генерируем код для TEST
    // TEST value -> PUSH value
    instruction_t inst = {0};

    // PUSH value
    inst.opcode = MAKE_OPCODE(value->addr_mode, OP_PUSH);
    inst.operand1 = TRYTE_FROM_INT(value->value.number);
    inst.operand2 = TRYTE_FROM_INT(0);
    if (codegen_emit(gen, &inst) != VM_OK) return VM_ERROR_INVALID_PARAMETER;

    return VM_OK;
}

// Макросы для X-macro паттерна
#define X_BASIC(name, value, operands, desc, group, handler) \
    {#name, value, operands, desc, group, NULL},

#define X_PSEUDO(name, value, operands, desc, group, handler) \
    {#name, value, operands, desc, group, handler},

// Таблица инструкций
static const instruction_info_t instructions[] = {
    // Базовые инструкции
    BASIC_INSTRUCTION_LIST(X_BASIC)
    // Псевдоинструкции
    PSEUDO_INSTRUCTION_LIST(X_PSEUDO)
    {NULL, OP_COUNT, 0, NULL, NULL, NULL}  // Маркер конца таблицы
};

// Получение информации об инструкции по имени
const instruction_info_t* get_instruction_info(const char* name) {
    if (!name) return NULL;
    
    printf("DEBUG: Поиск инструкции '%s'\n", name);
    printf("DEBUG: Размер таблицы: %zu элементов\n", 
           sizeof(instructions) / sizeof(instructions[0]));
    
    // Выведем все инструкции в таблице
    printf("DEBUG: Доступные инструкции:\n");
    for (size_t i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
        if (!instructions[i].name) {
            printf("DEBUG: [%zu] NULL (конец таблицы)\n", i);
            break;
        }
        printf("DEBUG: [%zu] '%s' (тип=%d)\n", 
               i, instructions[i].name, instructions[i].type);
    }
    
    // Ищем инструкцию в таблице
    for (size_t i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
        if (!instructions[i].name) break;  // Достигли конца таблицы
        
        printf("DEBUG: Сравниваем с '%s'\n", instructions[i].name);
        if (strcasecmp(instructions[i].name, name) == 0) {
            printf("DEBUG: Нашли инструкцию '%s' с типом %d\n", 
                   instructions[i].name, instructions[i].type);
            return &instructions[i];
        }
    }
    
    printf("DEBUG: Инструкция '%s' не найдена\n", name);
    return NULL;  // Инструкция не найдена
}