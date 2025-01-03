#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "zarya_vm.h"
#include "instruction_defs.h"

// Структура инструкции
typedef struct {
    tryte_t opcode;    // Код операции
    tryte_t operand1;  // Первый операнд
    tryte_t operand2;  // Второй операнд
} instruction_t;

// Функции для выполнения инструкций
vm_error_t execute_instruction(vm_state_t* vm, const instruction_t* inst);

#endif // INSTRUCTIONS_H 