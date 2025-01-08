#include <string.h>
#include <stdio.h>
#include "instruction_defs.h"
#include "instructions.h"
#include "trit_ops.h"
#include "stack.h"
#include "zarya_config.h"

// Вспомогательные функции для работы с тритами в слове
static tryte_t get_opcode(const word_t* word) {
    tryte_t opcode = {0};
    if (word) {
        // Копируем первые TRITS_PER_TRYTE тритов
        memcpy(opcode.trits, word->trits, TRITS_PER_TRYTE);
        update_tryte_value(&opcode);
    }
    return opcode;
}

static tryte_t get_operand1(const word_t* word) {
    tryte_t operand = {0};
    if (word) {
        // Копируем следующие TRITS_PER_TRYTE тритов
        memcpy(operand.trits, &word->trits[TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        update_tryte_value(&operand);
    }
    return operand;
}

static tryte_t get_operand2(const word_t* word) {
    tryte_t operand = {0};
    if (word) {
        // Копируем последние TRITS_PER_TRYTE тритов
        memcpy(operand.trits, &word->trits[2 * TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        update_tryte_value(&operand);
    }
    return operand;
}

// Получение значения операнда с учетом режима адресации
static vm_error_t get_operand_value(vm_state_t* vm, const tryte_t* operand, trit_t addr_mode, tryte_t* value) {
    if (!vm || !operand || !value) {
        return VM_ERROR_INVALID_ADDRESS;
    }

    switch (addr_mode) {
        case ADDR_MODE_IMMEDIATE:
            *value = *operand;  // Непосредственное значение
            break;
            
        case ADDR_MODE_REGISTER:
            // Проверяем, что индекс регистра в допустимом диапазоне
            if (operand->value < 0 || (size_t)operand->value >= NUM_REGISTERS) {
                printf("Invalid register index: %d\n", operand->value);
                return VM_ERROR_INVALID_REGISTER;
            }
            *value = vm->registers[operand->value];  // Значение из регистра
            break;
            
        case ADDR_MODE_INDIRECT:
            if (operand->value < 0 || (size_t)operand->value >= NUM_REGISTERS) {
                return VM_ERROR_INVALID_REGISTER;
            }
            // Получаем адрес из регистра
            int addr = vm->registers[operand->value].value;
            if (addr < 0 || (size_t)addr >= vm->memory_size) {
                return VM_ERROR_INVALID_ADDRESS;
            }
            *value = vm->memory[addr];  // Значение по адресу из регистра
            break;
            
        default:
            return VM_ERROR_INVALID_ADDRESSING_MODE;
    }
    
    return VM_OK;
}

instruction_t decode_instruction(const word_t* word) {
    instruction_t inst = {0};
    if (word) {
        // Получаем каждую часть инструкции
        inst.opcode = get_opcode(word);
        inst.operand1 = get_operand1(word);
        inst.operand2 = get_operand2(word);
        
        // Получаем режим адресации и базовый опкод для отладки
        trit_t addr_mode = GET_ADDR_MODE(inst.opcode.value);
        int base_opcode = GET_BASE_OPCODE(inst.opcode.value);
        
        printf("Decoded instruction: op=%d (base=%d, mode=%d), op1=%d, op2=%d\n", 
               inst.opcode.value, base_opcode, addr_mode,
               inst.operand1.value, inst.operand2.value);
    }
    return inst;
}

word_t encode_instruction(const instruction_t* inst) {
    word_t word = {0};
    if (inst) {
        // Копируем опкод
        memcpy(word.trits, inst->opcode.trits, TRITS_PER_TRYTE);
        
        // Копируем первый операнд
        memcpy(&word.trits[TRITS_PER_TRYTE], inst->operand1.trits, TRITS_PER_TRYTE);
        
        // Копируем второй операнд
        memcpy(&word.trits[2 * TRITS_PER_TRYTE], inst->operand2.trits, TRITS_PER_TRYTE);
        
        // Обновляем значение слова
        int64_t value = 0;
        int64_t power = 1;
        
        // Вычисляем значение, начиная с младших тритов
        for (int i = 0; i < TRITS_PER_WORD; i++) {
            value += word.trits[i] * power;
            power *= 3;
        }
        
        word.value = value;
    }
    return word;
}

// Выполнение арифметических операций
static vm_error_t execute_arithmetic(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Получаем операнды из стека
    tryte_t op1, op2;
    vm_error_t err;
    
    // Снимаем операнды в правильном порядке
    err = stack_pop(vm, &op1);  // Первый операнд сверху
    if (err != VM_OK) return err;
    
    err = stack_pop(vm, &op2);  // Второй операнд под ним
    if (err != VM_OK) {
        // Возвращаем первый операнд обратно в стек
        stack_push(vm, op1);
        return err;
    }
    
    tryte_t result = {0};
    
    // Выполняем операцию
    switch (base_opcode) {
        case OP_ADD:
            result = tryte_add(&op2, &op1);  // op2 + op1
            printf("ADD: %d + %d = %d\n", op2.value, op1.value, result.value);
            break;
        case OP_SUB:
            result = tryte_sub(&op2, &op1);  // op2 - op1
            break;
        case OP_MUL:
            result = tryte_mul(&op2, &op1);  // op2 * op1
            break;
        case OP_DIV:
            if (op1.value == 0) {
                // В случае деления на ноль возвращаем операнды в стек
                stack_push(vm, op2);
                stack_push(vm, op1);
                return VM_ERROR_DIVISION_BY_ZERO;
            }
            result = tryte_div(&op2, &op1);  // op2 / op1
            break;
        default:
            // В случае неизвестной операции возвращаем операнды в стек
            stack_push(vm, op2);
            stack_push(vm, op1);
            return VM_ERROR_INVALID_OPCODE;
    }
    
    // Кладем результат в стек
    return stack_push(vm, result);
}

// Выполнение логических операций
static vm_error_t execute_logical(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Получаем операнды из стека
    tryte_t op1, op2;
    vm_error_t err;
    
    switch (base_opcode) {
        case OP_AND: {
            // Снимаем операнды в правильном порядке
            err = stack_pop(vm, &op2);
            if (err != VM_OK) return err;
            
            err = stack_pop(vm, &op1);
            if (err != VM_OK) {
                stack_push(vm, op2);
                return err;
            }
            
            // В троичной логике AND:
            // -1 AND -1 = -1
            // -1 AND 0 = -1
            // -1 AND 1 = -1
            // 0 AND x = 0
            // 1 AND -1 = -1
            // 1 AND 0 = 0
            // 1 AND 1 = 1
            tryte_t result = {0};
            if (op1.value == 0 || op2.value == 0) {
                TRYTE_SET_VALUE(result, 0);
            } else if (op1.value == 1 && op2.value == 1) {
                TRYTE_SET_VALUE(result, 1);
            } else {
                TRYTE_SET_VALUE(result, -1);
            }
            
            return stack_push(vm, result);
        }
        
        case OP_OR: {
            // Снимаем операнды в правильном порядке
            err = stack_pop(vm, &op2);
            if (err != VM_OK) return err;
            
            err = stack_pop(vm, &op1);
            if (err != VM_OK) {
                stack_push(vm, op2);
                return err;
            }
            
            // В троичной логике OR:
            // -1 OR -1 = -1
            // -1 OR 0 = -1
            // -1 OR 1 = 1
            // 0 OR x = x
            // 1 OR x = 1
            tryte_t result = {0};
            if (op1.value == 1 || op2.value == 1) {
                TRYTE_SET_VALUE(result, 1);
            } else if (op1.value == 0) {
                TRYTE_SET_VALUE(result, op2.value);
            } else if (op2.value == 0) {
                TRYTE_SET_VALUE(result, op1.value);
            } else {
                TRYTE_SET_VALUE(result, -1);
            }
            
            return stack_push(vm, result);
        }
        
        case OP_NOT: {
            // Снимаем операнд
            err = stack_pop(vm, &op1);
            if (err != VM_OK) return err;
            
            // В троичной логике NOT:
            // NOT -1 = 1
            // NOT 0 = 0
            // NOT 1 = -1
            tryte_t result = {0};
            TRYTE_SET_VALUE(result, -op1.value);
            
            return stack_push(vm, result);
        }
        
        default:
            return VM_ERROR_INVALID_OPCODE;
    }
}

// Выполнение стековых операций
static vm_error_t execute_stack_operation(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод и режим адресации
    trit_t addr_mode = get_addressing_mode(inst);
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    printf("Stack operation: opcode=%d, base_opcode=%d, addr_mode=%d\n", 
           inst->opcode.value, base_opcode, addr_mode);
    
    tryte_t value;
    vm_error_t err;
    
    switch (base_opcode) {
        case OP_PUSH: {
            // Проверяем режим адресации
            if (addr_mode != ADDR_MODE_IMMEDIATE && 
                addr_mode != ADDR_MODE_REGISTER && 
                addr_mode != ADDR_MODE_INDIRECT) {
                printf("DEBUG: PUSH: invalid addressing mode %d\n", addr_mode);
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
            
            // Получаем значение операнда с учетом режима адресации
            printf("DEBUG: PUSH: operand=%d, sp=%d, addr_mode=%d, memory_size=%zu\n", 
                   inst->operand1.value, vm->sp.value, addr_mode, vm->memory_size);
            
            if (addr_mode == ADDR_MODE_IMMEDIATE) {
                value = inst->operand1;  // Для непосредственной адресации берем значение как есть
                printf("DEBUG: PUSH: immediate value=%d\n", value.value);
            } else {
                err = get_operand_value(vm, &inst->operand1, addr_mode, &value);
                if (err != VM_OK) {
                    printf("DEBUG: PUSH: error getting operand value: %d\n", err);
                    return err;
                }
                printf("DEBUG: PUSH: operand value=%d\n", value.value);
            }
            
            // Помещаем значение в стек
            err = stack_push(vm, value);
            if (err != VM_OK) {
                printf("DEBUG: PUSH: error pushing value: %d\n", err);
                return err;
            }
            
            // Выводим информацию о выполненной операции
            printf("DEBUG: PUSH completed: sp=%d, value=%d\n", vm->sp.value, value.value);
            return VM_OK;
        }
        
        case OP_POP: {
            // Проверяем режим адресации
            if (addr_mode != ADDR_MODE_REGISTER && addr_mode != ADDR_MODE_INDIRECT) {
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
            
            // Снимаем значение со стека
            err = stack_pop(vm, &value);
            if (err != VM_OK) return err;
            
            // Если указан регистр, сохраняем значение в него
            if (addr_mode == ADDR_MODE_REGISTER) {
                if (inst->operand1.value < 0 || inst->operand1.value >= NUM_REGISTERS) {
                    return VM_ERROR_INVALID_REGISTER;
                }
                vm->registers[inst->operand1.value] = value;
            }
            // Если указан косвенный адрес, сохраняем значение по адресу из регистра
            else if (addr_mode == ADDR_MODE_INDIRECT) {
                if (inst->operand1.value < 0 || inst->operand1.value >= NUM_REGISTERS) {
                    return VM_ERROR_INVALID_REGISTER;
                }
                int addr = vm->registers[inst->operand1.value].value;
                if (addr < 0 || addr >= vm->memory_size) {
                    return VM_ERROR_INVALID_ADDRESS;
                }
                vm->memory[addr] = value;
            }
            return VM_OK;
            break;
        }
        
        case OP_DUP: {
            // Проверяем режим адресации
            if (addr_mode != ADDR_MODE_IMMEDIATE) {
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
            
            // Дублируем значение на вершине стека
            err = stack_dup(vm);
            if (err != VM_OK) return err;
            
            // Выводим информацию о состоянии стека
            printf("DUP: sp=%d, top=%d, next=%d\n", 
                   vm->sp.value, 
                   vm->memory[vm->sp.value].value,
                   vm->memory[vm->sp.value - 1].value);
            break;
        }
        
        case OP_SWAP: {
            // Проверяем режим адресации
            if (addr_mode != ADDR_MODE_IMMEDIATE) {
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
            
            // Меняем местами два верхних значения стека
            err = stack_swap(vm);
            if (err != VM_OK) return err;
            
            // Выводим информацию о состоянии стека
            printf("SWAP: sp=%d, top=%d, next=%d\n", 
                   vm->sp.value, 
                   vm->memory[vm->sp.value].value,
                   vm->memory[vm->sp.value - 1].value);
            break;
        }
        
        case OP_DROP: {
            // Проверяем режим адресации
            if (addr_mode != ADDR_MODE_IMMEDIATE) {
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
            
            // Удаляем значение с вершины стека
            err = stack_pop(vm, &value);
            if (err != VM_OK) return err;
            
            printf("DROP: sp=%d, top=%d\n", 
                   vm->sp.value, value.value);
            break;
        }
        
        case OP_OVER: {
            // Проверяем режим адресации
            if (addr_mode != ADDR_MODE_IMMEDIATE) {
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
            
            // Копируем предпоследний элемент на вершину
            tryte_t value1, value2;
            err = stack_pop(vm, &value1);
            if (err != VM_OK) return err;
            
            err = stack_pop(vm, &value2);
            if (err != VM_OK) {
                stack_push(vm, value1);
                return err;
            }
            
            err = stack_push(vm, value2);
            if (err != VM_OK) return err;
            
            err = stack_push(vm, value1);
            if (err != VM_OK) return err;
            
            err = stack_push(vm, value2);
            if (err != VM_OK) return err;
            
            printf("OVER: sp=%d, top=%d, next=%d, third=%d\n", 
                   vm->sp.value, value2.value, value1.value, value2.value);
            break;
        }
        
        default:
            return VM_ERROR_INVALID_OPCODE;
    }
    
    return VM_OK;
}

// Выполнение команд управления потоком
static vm_error_t execute_control(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    tryte_t addr, value;
    vm_error_t err;
    
    switch (base_opcode) {
        case OP_JMP: {
            // Безусловный переход по адресу из операнда
            addr = inst->operand1;
            if (addr.value < 0 || (size_t)addr.value >= vm->memory_size) {
                return VM_ERROR_INVALID_ADDRESS;
            }
            vm->pc.value = addr.value;
            printf("JMP: jumping to %d\n", addr.value);
            return VM_OK;
        }
        
        case OP_JZ: {
            // Условный переход, если на вершине стека 0
            err = stack_pop(vm, &value);
            if (err != VM_OK) return err;
            
            if (value.value == 0) {
                addr = inst->operand1;
                if (addr.value < 0 || (size_t)addr.value >= vm->memory_size) {
                    return VM_ERROR_INVALID_ADDRESS;
                }
                vm->pc.value = addr.value;
                printf("JZ: jumping to %d\n", addr.value);
            } else {
                vm->pc.value += 6;  // Пропускаем текущую и следующую инструкцию
                printf("JZ: not jumping, moving to %d\n", vm->pc.value);
            }
            return VM_OK;
        }
        
        case OP_JNZ: {
            // Условный переход, если на вершине стека не 0
            err = stack_pop(vm, &value);
            if (err != VM_OK) return err;
            
            if (value.value != 0) {
                addr = inst->operand1;
                if (addr.value < 0 || (size_t)addr.value >= vm->memory_size) {
                    return VM_ERROR_INVALID_ADDRESS;
                }
                vm->pc.value = addr.value;
                printf("JNZ: value=%d (not zero), jumping to %d\n", value.value, addr.value);
            } else {
                vm->pc.value += 6;  // Пропускаем текущую и следующую инструкцию
                printf("JNZ: value=%d (zero), moving to %d\n", value.value, vm->pc.value);
            }
            return VM_OK;
        }
        
        case OP_CALL: {
            // Вызов подпрограммы
            addr = inst->operand1;
            if (addr.value < 0 || (size_t)addr.value >= vm->memory_size) {
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // Сохраняем адрес возврата (следующая инструкция)
            tryte_t return_addr_tryte = TRYTE_FROM_INT(vm->pc.value + 6);  // Указываем на следующую инструкцию после CALL
            
            // Кладем адрес возврата в стек
            err = stack_push(vm, return_addr_tryte);
            if (err != VM_OK) return err;
            
            // Выполняем переход
            vm->pc.value = addr.value;
            printf("CALL: jumping to %d, return addr=%d\n", addr.value, return_addr_tryte.value);
            return VM_OK;
        }
        
        case OP_RET: {
            // Возврат из подпрограммы
            // Сначала снимаем значение результата
            err = stack_pop(vm, &value);
            if (err != VM_OK) return err;
            
            // Затем снимаем адрес возврата
            err = stack_pop(vm, &addr);
            if (err != VM_OK) {
                stack_push(vm, value);  // Возвращаем значение обратно
                return err;
            }
            
            if (addr.value < 0 || (size_t)addr.value >= vm->memory_size) {
                stack_push(vm, value);  // Возвращаем значение обратно
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // Кладем результат обратно на стек
            err = stack_push(vm, value);
            if (err != VM_OK) return err;
            
            vm->pc.value = addr.value;
            printf("RET: returning to %d with value %d\n", addr.value, value.value);
            return VM_OK;
        }
        
        case OP_HALT: {
            printf("HALT: stopping execution\n");
            SET_FLAG(vm, FLAG_HALT_TRIT, TRIT_NEGATIVE);  // Устанавливаем флаг остановки
            return VM_OK;  // Возвращаем успешное выполнение
        }
        
        default:
            return VM_ERROR_INVALID_OPCODE;
    }
}

// Выполнение операций с памятью
static vm_error_t execute_memory(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод и режим адресации
    trit_t addr_mode = get_addressing_mode(inst);
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Получаем операнды из стека
    tryte_t value;
    vm_error_t err;
    
    switch (base_opcode) {
        case OP_LOAD: {
            if (addr_mode == ADDR_MODE_IMMEDIATE) {
                // Для непосредственной адресации просто кладем значение в стек
                value = inst->operand1;
                return stack_push(vm, value);
            } else if (addr_mode == ADDR_MODE_REGISTER) {
                // Для регистровой адресации берем значение из регистра
                if (inst->operand1.value < 0 || inst->operand1.value >= NUM_REGISTERS) {
                    return VM_ERROR_INVALID_REGISTER;
                }
                value = vm->registers[inst->operand1.value];
                printf("LOAD: addr=%d, value=%d, mode=%d\n", inst->operand1.value, value.value, addr_mode);
                return stack_push(vm, value);
            } else if (addr_mode == ADDR_MODE_INDIRECT) {
                // Для косвенной адресации берем адрес из регистра и загружаем значение по этому адресу
                if (inst->operand1.value < 0 || inst->operand1.value >= NUM_REGISTERS) {
                    return VM_ERROR_INVALID_REGISTER;
                }
                int addr = vm->registers[inst->operand1.value].value;
                if (addr < 0 || (size_t)addr >= vm->memory_size) {
                    return VM_ERROR_INVALID_ADDRESS;
                }
                value = vm->memory[addr];
                printf("LOAD: addr=%d, value=%d, mode=%d\n", addr, value.value, addr_mode);
                return stack_push(vm, value);
            } else {
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
        }
        
        case OP_STORE: {
            // Снимаем значение со стека
            err = stack_pop(vm, &value);
            if (err != VM_OK) return err;
            
            int addr;
            if (addr_mode == ADDR_MODE_IMMEDIATE) {
                // Для непосредственной адресации используем адрес из операнда
                addr = inst->operand1.value;
            } else if (addr_mode == ADDR_MODE_REGISTER) {
                // Для регистровой адресации берем адрес из регистра
                if (inst->operand1.value < 0 || inst->operand1.value >= NUM_REGISTERS) {
                    stack_push(vm, value);  // Возвращаем значение в стек
                    return VM_ERROR_INVALID_REGISTER;
                }
                addr = vm->registers[inst->operand1.value].value;
            } else if (addr_mode == ADDR_MODE_INDIRECT) {
                // Для косвенной адресации берем адрес из памяти по адресу из регистра
                if (inst->operand1.value < 0 || inst->operand1.value >= NUM_REGISTERS) {
                    stack_push(vm, value);  // Возвращаем значение в стек
                    return VM_ERROR_INVALID_REGISTER;
                }
                int reg_addr = vm->registers[inst->operand1.value].value;
                if (reg_addr < 0 || (size_t)reg_addr >= vm->memory_size) {
                    stack_push(vm, value);  // Возвращаем значение в стек
                    return VM_ERROR_INVALID_ADDRESS;
                }
                addr = vm->memory[reg_addr].value;
            } else {
                stack_push(vm, value);  // Возвращаем значение в стек
                return VM_ERROR_INVALID_ADDRESSING_MODE;
            }
            
            if (addr < 0 || (size_t)addr >= vm->memory_size) {
                stack_push(vm, value);  // Возвращаем значение в стек
                return VM_ERROR_INVALID_ADDRESS;
            }
            vm->memory[addr] = value;
            
            printf("STORE: addr=%d, value=%d, mode=%d\n", addr, value.value, addr_mode);
            return VM_OK;
        }
        
        default:
            return VM_ERROR_INVALID_OPCODE;
    }
}

// Выполнение операций сравнения
static vm_error_t execute_comparison(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Получаем операнды из стека
    tryte_t op1, op2;
    vm_error_t err;
    
    // Снимаем операнды в правильном порядке
    err = stack_pop(vm, &op1);  // Первый операнд сверху
    if (err != VM_OK) return err;
    
    err = stack_pop(vm, &op2);  // Второй операнд под ним
    if (err != VM_OK) {
        stack_push(vm, op1);
        return err;
    }
    
    printf("Comparison operands: op1=%d, op2=%d\n", op1.value, op2.value);
    
    tryte_t result = {0};
    
    // Выполняем сравнение
    switch (base_opcode) {
        case OP_EQ:
            // Равенство: 1 если равны, -1 если не равны
            result = TRYTE_FROM_INT(op2.value == op1.value ? 1 : -1);
            printf("EQ: %d == %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_NEQ:
            // Неравенство: 1 если не равны, -1 если равны
            result = TRYTE_FROM_INT(op2.value != op1.value ? 1 : -1);
            printf("NEQ: %d != %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_LT:
            // Меньше: 1 если op2 < op1, -1 если op2 >= op1
            result = TRYTE_FROM_INT(op2.value < op1.value ? 1 : -1);
            printf("LT: %d < %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_GT:
            // Больше: 1 если op2 > op1, -1 если op2 <= op1
            result = TRYTE_FROM_INT(op2.value > op1.value ? 1 : -1);
            printf("GT: %d > %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_LE:
            // Меньше или равно: 1 если op2 <= op1, -1 если op2 > op1
            result = TRYTE_FROM_INT(op2.value <= op1.value ? 1 : -1);
            printf("LE: %d <= %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_GE:
            // Больше или равно: 1 если op2 >= op1, -1 если op2 < op1
            result = TRYTE_FROM_INT(op2.value >= op1.value ? 1 : -1);
            printf("GE: %d >= %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        default:
            // Возвращаем операнды в стек
            stack_push(vm, op2);
            stack_push(vm, op1);
            return VM_ERROR_INVALID_OPCODE;
    }
    
    // Кладем результат в стек
    return stack_push(vm, result);
}

// Выполнение инструкций прерываний
static vm_error_t execute_interrupt(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    switch (base_opcode) {
        case OP_INT: {
            // Проверяем, разрешены ли прерывания
            if (GET_FLAG(vm, FLAG_INTERRUPT_TRIT) != TRIT_POSITIVE) {
                return VM_ERROR_INTERRUPTS_DISABLED;
            }
            
            // Получаем номер прерывания из стека
            tryte_t int_num;
            vm_error_t err = stack_pop(vm, &int_num);
            if (err != VM_OK) return err;
            
            // Вызываем обработчик прерывания
            if (vm->interrupt_callback) {
                return vm->interrupt_callback(vm->interrupt_context, int_num.value);
            }
            return VM_OK;
        }
        
        case OP_CLI: {
            // Запрещаем прерывания
            SET_FLAG(vm, FLAG_INTERRUPT_TRIT, TRIT_NEGATIVE);
            return VM_OK;
        }
        
        case OP_STI: {
            // Разрешаем прерывания
            SET_FLAG(vm, FLAG_INTERRUPT_TRIT, TRIT_POSITIVE);
            return VM_OK;
        }
        
        default:
            return VM_ERROR_INVALID_OPCODE;
    }
}

// Выполнение инструкции
vm_error_t execute_instruction(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Выводим отладочную информацию
    printf("DEBUG: execute_instruction: pc=%d, opcode=%d (base=%d, mode=%d), op1=%d, op2=%d\n",
           vm->pc.value, inst->opcode.value, base_opcode, GET_ADDR_MODE(inst->opcode.value),
           inst->operand1.value, inst->operand2.value);
    
    printf("DEBUG: execute_instruction: memory state at pc: [%d, %d, %d]\n",
           vm->memory[vm->pc.value].value,
           vm->memory[vm->pc.value + 1].value,
           vm->memory[vm->pc.value + 2].value);
    
    printf("DEBUG: execute_instruction: stack pointer=%d, registers=[%d, %d, %d, %d]\n",
           vm->sp.value,
           vm->registers[0].value,
           vm->registers[1].value,
           vm->registers[2].value,
           vm->registers[3].value);
    
    vm_error_t err = VM_OK;
    
    // Выполняем инструкцию в зависимости от группы опкодов
    if (IS_STACK_OPCODE(base_opcode)) {
        printf("DEBUG: execute_instruction: executing stack operation\n");
        err = execute_stack_operation(vm, inst);
    } else if (IS_ARITHMETIC_OPCODE(base_opcode)) {
        printf("DEBUG: execute_instruction: executing arithmetic operation\n");
        err = execute_arithmetic(vm, inst);
    } else if (IS_LOGICAL_OPCODE(base_opcode)) {
        printf("DEBUG: execute_instruction: executing logical operation\n");
        err = execute_logical(vm, inst);
    } else if (IS_COMPARISON_OPCODE(base_opcode)) {
        printf("DEBUG: execute_instruction: executing comparison operation\n");
        err = execute_comparison(vm, inst);
    } else if (IS_CONTROL_OPCODE(base_opcode)) {
        printf("DEBUG: execute_instruction: executing control operation\n");
        err = execute_control(vm, inst);
    } else if (IS_MEMORY_OPCODE(base_opcode)) {
        printf("DEBUG: execute_instruction: executing memory operation\n");
        err = execute_memory(vm, inst);
    } else if (IS_INTERRUPT_OPCODE(base_opcode)) {
        printf("DEBUG: execute_instruction: executing interrupt operation\n");
        err = execute_interrupt(vm, inst);
    } else {
        printf("ERROR: Unknown opcode %d (full opcode: %d)\n", base_opcode, inst->opcode.value);
        err = VM_ERROR_INVALID_OPCODE;
    }
    
    printf("DEBUG: execute_instruction: operation result=%d\n", err);
    printf("DEBUG: execute_instruction: new stack pointer=%d\n", vm->sp.value);
    if (vm->sp.value >= 0) {
        printf("DEBUG: execute_instruction: top of stack=%d\n", vm->memory[vm->sp.value].value);
    }
    
    return err;
} 