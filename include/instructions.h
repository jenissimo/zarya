#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include "types.h"
#include "zarya_vm.h"
#include "instruction_defs.h"

// Опкоды инструкций
typedef enum {
#define DEFINE_OPCODE(name, value, operands, desc, group, handler) OP_##name = value,
    INSTRUCTION_LIST(DEFINE_OPCODE)
#undef DEFINE_OPCODE
    OP_COUNT       // Количество опкодов
} opcode_t;

// Структура для хранения инструкции
typedef struct {
    tryte_t opcode;    // Код операции
    tryte_t operand1;  // Первый операнд (для инструкций, требующих параметров)
    tryte_t operand2;  // Второй операнд (для инструкций, требующих параметров)
} instruction_t;

// Декодирование инструкции из машинного слова
instruction_t decode_instruction(const word_t* word);

// Кодирование инструкции в машинное слово
word_t encode_instruction(const instruction_t* inst);

// Выполнение инструкции
vm_error_t execute_instruction(vm_state_t* vm, const instruction_t* inst);

#endif // INSTRUCTIONS_H 