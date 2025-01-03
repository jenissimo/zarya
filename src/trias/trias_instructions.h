#ifndef TRIAS_INSTRUCTIONS_H
#define TRIAS_INSTRUCTIONS_H

#include "instruction_defs.h"
#include "types.h"
#include <stdbool.h>

// Структура для информации об инструкции
typedef struct {
    const char* name;          // Имя инструкции
    int type;                  // Тип инструкции
    int operand_count;         // Количество операндов
    const char* description;   // Описание
    const char* group;         // Группа инструкций
} instruction_info_t;

// Структура для хранения информации об операнде
typedef struct {
    int value;              // Значение операнда
    trit_t addressing_mode; // Режим адресации
} operand_t;

// Структура для представления инструкции в ассемблере
typedef struct {
    const char* name;       // Имя инструкции
    int opcode;            // Базовый опкод
    operand_t operands[2]; // Операнды (максимум 2)
    size_t operand_count;  // Фактическое количество операндов
} trias_instruction_t;

// API для работы с инструкциями
bool trias_instruction_parse(const char* text, trias_instruction_t* out_instruction);
bool trias_instruction_encode(const trias_instruction_t* inst, tryte_t* out_bytes);
const char* trias_instruction_get_group(const char* name);
const char* trias_instruction_get_description(const char* name);

#endif // TRIAS_INSTRUCTIONS_H 