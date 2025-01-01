#include <string.h>
#include <stdio.h>
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
    trit_t addr_mode = GET_ADDR_MODE(inst->opcode.value);
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
    
    // Получаем базовый опкод и режим адресации
    trit_t addr_mode = GET_ADDR_MODE(inst->opcode.value);
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Получаем операнды из стека
    tryte_t addr, cond;
    vm_error_t err;
    
    switch (base_opcode) {
        case OP_JMP: {
            // Снимаем адрес со стека
            err = stack_pop(vm, &addr);
            if (err != VM_OK) return err;
            
            // Проверяем адрес
            if (addr.value < 0 || addr.value >= vm->memory_size) {
                stack_push(vm, addr);  // Возвращаем адрес в стек
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // Выполняем безусловный переход
            vm->pc.value = addr.value;
            printf("JMP: jumping to %d\n", addr.value);
            return VM_OK;
        }
        
        case OP_JZ: {
            // Снимаем условие
            err = stack_pop(vm, &cond);
            if (err != VM_OK) return err;
            
            // Снимаем адрес
            err = stack_pop(vm, &addr);
            if (err != VM_OK) {
                stack_push(vm, cond);
                return err;
            }
            
            // Проверяем адрес
            if (addr.value < 0 || addr.value >= vm->memory_size) {
                stack_push(vm, addr);
                stack_push(vm, cond);
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // В троичной логике:
            // -1 (ложь) -> не переходим
            //  0 (неопределенность) -> переходим
            //  1 (истина) -> не переходим
            if (cond.value == 0) {
                vm->pc.value = addr.value;
                printf("JZ: value=%d (zero), jumping to %d\n", cond.value, addr.value);
            } else {
                vm->pc.value += INSTRUCTION_SIZE;
                printf("JZ: value=%d (not zero), not jumping\n", cond.value);
            }
            return VM_OK;
        }
        
        case OP_JNZ: {
            // Снимаем условие
            err = stack_pop(vm, &cond);
            if (err != VM_OK) return err;
            
            // Снимаем адрес
            err = stack_pop(vm, &addr);
            if (err != VM_OK) {
                stack_push(vm, cond);
                return err;
            }
            
            // Проверяем адрес
            if (addr.value < 0 || addr.value >= vm->memory_size) {
                stack_push(vm, addr);
                stack_push(vm, cond);
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // В троичной логике:
            // -1 (ложь) -> переходим
            //  0 (неопределенность) -> не переходим
            //  1 (истина) -> переходим
            if (cond.value != 0) {
                vm->pc.value = addr.value;
                printf("JNZ: value=%d (not zero), jumping to %d\n", cond.value, addr.value);
            } else {
                vm->pc.value += INSTRUCTION_SIZE;
                printf("JNZ: value=%d (zero), not jumping\n", cond.value);
            }
            return VM_OK;
        }

        case OP_CALL: {
            // Снимаем адрес со стека
            err = stack_pop(vm, &addr);
            if (err != VM_OK) return err;
            
            // Проверяем адрес
            if (addr.value < 0 || addr.value >= vm->memory_size) {
                stack_push(vm, addr);
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // Сохраняем адрес возврата (следующая инструкция)
            int return_addr_value = vm->pc.value + INSTRUCTION_SIZE;
            tryte_t return_addr = create_tryte_from_int(return_addr_value);
            
            // Выполняем переход к подпрограмме
            vm->pc.value = addr.value;
            
            // Кладем адрес возврата в стек
            err = stack_push(vm, return_addr);
            if (err != VM_OK) {
                vm->pc.value = return_addr_value - INSTRUCTION_SIZE;  // Восстанавливаем PC
                return err;
            }
            
            printf("CALL: jumping to %d, return addr=%d\n", addr.value, return_addr.value);
            return VM_OK;
        }
        
        case OP_RET: {
            // Снимаем адрес возврата со стека
            err = stack_pop(vm, &addr);
            if (err != VM_OK) return err;
            
            // Проверяем адрес
            if (addr.value < 0 || addr.value >= vm->memory_size) {
                stack_push(vm, addr);
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // Возвращаемся по сохраненному адресу
            vm->pc.value = addr.value;
            printf("RET: returning to %d\n", addr.value);
            return VM_OK;
        }
        
        case OP_HALT:
            return VM_ERROR_HALT;
            
        default:
            return VM_ERROR_INVALID_OPCODE;
    }
}

// Выполнение операций с памятью
static vm_error_t execute_memory(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод и режим адресации
    trit_t addr_mode = GET_ADDR_MODE(inst->opcode.value);
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Получаем операнды из стека
    tryte_t addr, value;
    vm_error_t err;
    
    switch (base_opcode) {
        case OP_LOAD: {
            if (addr_mode == ADDR_MODE_IMMEDIATE) {
                // Для непосредственной адресации снимаем адрес со стека
                err = stack_pop(vm, &addr);
                if (err != VM_OK) return err;
            } else {
                // Для других режимов получаем адрес из операнда
                err = get_operand_value(vm, &inst->operand1, addr_mode, &addr);
                if (err != VM_OK) return err;
            }
            
            // Проверяем адрес
            if (addr.value < 0 || addr.value >= vm->memory_size) {
                if (addr_mode == ADDR_MODE_IMMEDIATE) {
                    stack_push(vm, addr);
                }
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // Загружаем значение из памяти
            value = vm->memory[addr.value];
            printf("LOAD: addr=%d, value=%d, mode=%d\n", 
                   addr.value, value.value, addr_mode);
            
            // Кладем значение в стек
            return stack_push(vm, value);
        }
        
        case OP_STORE: {
            // Снимаем значение и адрес со стека
            err = stack_pop(vm, &value);  // Сначала значение
            if (err != VM_OK) return err;
            
            if (addr_mode == ADDR_MODE_IMMEDIATE) {
                // Для непосредственной адресации снимаем адрес со стека
                err = stack_pop(vm, &addr);
                if (err != VM_OK) {
                    stack_push(vm, value);
                    return err;
                }
            } else {
                // Для других режимов получаем адрес из операнда
                err = get_operand_value(vm, &inst->operand1, addr_mode, &addr);
                if (err != VM_OK) {
                    stack_push(vm, value);
                    return err;
                }
            }
            
            // Проверяем адрес
            if (addr.value < 0 || addr.value >= vm->memory_size) {
                stack_push(vm, value);
                return VM_ERROR_INVALID_ADDRESS;
            }
            
            // Сохраняем значение в память
            vm->memory[addr.value] = value;
            printf("STORE: addr=%d, value=%d, mode=%d\n", 
                   addr.value, value.value, addr_mode);
            return VM_OK;
        }
        
        default:
            return VM_ERROR_INVALID_OPCODE;
    }
}

// Выполнение операций сравнения
static vm_error_t execute_comparison(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод и режим адресации
    trit_t addr_mode = GET_ADDR_MODE(inst->opcode.value);
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
            result = create_tryte_from_int(op2.value == op1.value ? 1 : -1);
            printf("EQ: %d == %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_NEQ:
            // Неравенство: 1 если не равны, -1 если равны
            result = create_tryte_from_int(op2.value != op1.value ? 1 : -1);
            printf("NEQ: %d != %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_LT:
            // Меньше: 1 если op2 < op1, -1 если op2 >= op1
            result = create_tryte_from_int(op2.value < op1.value ? 1 : -1);
            printf("LT: %d < %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_GT:
            // Больше: 1 если op2 > op1, -1 если op2 <= op1
            result = create_tryte_from_int(op2.value > op1.value ? 1 : -1);
            printf("GT: %d > %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_LE:
            // Меньше или равно: 1 если op2 <= op1, -1 если op2 > op1
            result = create_tryte_from_int(op2.value <= op1.value ? 1 : -1);
            printf("LE: %d <= %d = %d\n", op2.value, op1.value, result.value);
            break;
            
        case OP_GE:
            // Больше или равно: 1 если op2 >= op1, -1 если op2 < op1
            result = create_tryte_from_int(op2.value >= op1.value ? 1 : -1);
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

// Выполнение операций прерывания
static vm_error_t execute_interrupt(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод и режим адресации
    trit_t addr_mode = GET_ADDR_MODE(inst->opcode.value);
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    // Получаем значение операнда
    tryte_t value;
    vm_error_t err = get_operand_value(vm, &inst->operand1, addr_mode, &value);
    if (err != VM_OK) return err;
    
    // Проверяем, что это операция прерывания
    if (base_opcode != OP_INT) {
        return VM_ERROR_INVALID_OPCODE;
    }
    
    // Обрабатываем операцию прерывания
    switch (value.value) {
        case 1:  // Включить прерывания
            vm->flags.value |= FLAG_INTERRUPTS_ENABLED;
            break;
            
        case -1:  // Выключить прерывания
            vm->flags.value &= ~FLAG_INTERRUPTS_ENABLED;
            break;
            
        case 0:  // Нет операции
            break;
            
        default:  // Вызов прерывания
            if (!(vm->flags.value & FLAG_INTERRUPTS_ENABLED)) {
                return VM_ERROR_INTERRUPTS_DISABLED;
            }
            
            if (vm->interrupt_callback) {
                vm->interrupt_callback(vm, value.value);
            }
            break;
    }
    
    return VM_OK;
}

// Выполнение инструкции
vm_error_t execute_instruction(vm_state_t* vm, const instruction_t* inst) {
    if (!vm || !inst) return VM_ERROR_INVALID_ADDRESS;
    
    // Получаем базовый опкод и режим адресации
    trit_t addr_mode = GET_ADDR_MODE(inst->opcode.value);
    int base_opcode = GET_BASE_OPCODE(inst->opcode.value);
    
    printf("DEBUG: execute_instruction: pc=%d, opcode=%d (base=%d, mode=%d), op1=%d, op2=%d\n",
           vm->pc.value, inst->opcode.value, base_opcode, addr_mode,
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
    
    // Выполняем инструкцию в зависимости от базового опкода
    vm_error_t result;
    switch (base_opcode) {
        // Операции со стеком
        case OP_PUSH:
        case OP_POP:
        case OP_DUP:
        case OP_SWAP:
        case OP_DROP:
        case OP_OVER:
            printf("DEBUG: execute_instruction: executing stack operation\n");
            result = execute_stack_operation(vm, inst);
            break;
            
        // Арифметические операции
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            printf("DEBUG: execute_instruction: executing arithmetic operation\n");
            result = execute_arithmetic(vm, inst);
            break;
            
        // Логические операции
        case OP_AND:
        case OP_OR:
        case OP_NOT:
            printf("DEBUG: execute_instruction: executing logical operation\n");
            result = execute_logical(vm, inst);
            break;
            
        // Операции сравнения
        case OP_EQ:
        case OP_NEQ:
        case OP_LT:
        case OP_GT:
        case OP_LE:
        case OP_GE:
            printf("DEBUG: execute_instruction: executing comparison operation\n");
            result = execute_comparison(vm, inst);
            break;
            
        // Операции с памятью
        case OP_LOAD:
        case OP_STORE:
            printf("DEBUG: execute_instruction: executing memory operation\n");
            result = execute_memory(vm, inst);
            break;
            
        // Операции управления
        case OP_JMP:
        case OP_JZ:
        case OP_JNZ:
        case OP_CALL:
        case OP_RET:
        case OP_HALT:
            printf("DEBUG: execute_instruction: executing control operation\n");
            result = execute_control(vm, inst);
            break;
            
        // Операции с прерываниями
        case OP_INT:
            printf("DEBUG: execute_instruction: executing interrupt operation\n");
            result = execute_interrupt(vm, inst);
            break;
            
        default:
            printf("ERROR: Unknown opcode %d (full opcode: %d)\n", base_opcode, inst->opcode.value);
            return VM_ERROR_INVALID_OPCODE;
    }
    
    printf("DEBUG: execute_instruction: operation result=%d\n", result);
    if (result == VM_OK) {
        printf("DEBUG: execute_instruction: new stack pointer=%d\n", vm->sp.value);
        if (vm->sp.value >= 0) {
            printf("DEBUG: execute_instruction: top of stack=%d\n", 
                   vm->memory[vm->sp.value].value);
        }
    }
    
    return result;
} 