#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include "types.h"
#include "zarya_vm.h"
#include "instruction_defs.h"

// Структура для хранения инструкции
typedef struct {
    tryte_t opcode;    // Код операции (включая режим адресации в старшем трите)
    tryte_t operand1;  // Первый операнд (для инструкций, требующих параметров)
    tryte_t operand2;  // Второй операнд (для инструкций, требующих параметров)
} instruction_t;

// Получение режима адресации
static inline trit_t get_addressing_mode(const instruction_t* inst) {
    return GET_ADDR_MODE(inst->opcode.value);
}

// Создание инструкции с заданным режимом адресации
static inline instruction_t make_instruction(int opcode, trit_t mode) {
    instruction_t inst = {0};
    inst.opcode = MAKE_OPCODE(mode, opcode);
    return inst;
}

// Декодирование инструкции из машинного слова
instruction_t decode_instruction(const word_t* word);

// Кодирование инструкции в машинное слово
word_t encode_instruction(const instruction_t* inst);

// Выполнение инструкции
vm_error_t execute_instruction(vm_state_t* vm, const instruction_t* inst);

#endif // INSTRUCTIONS_H 