#include "trias_instructions.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Внутренняя функция для поиска информации об инструкции
static const instruction_info_t* find_instruction(const char* name) {
    if (!name) return NULL;
    
    // Проходим по списку базовых инструкций
    #define X(QNAME, VALUE, OPERANDS, DESC, GROUP, HANDLER) \
        if (strcasecmp(name, #QNAME) == 0) { \
            static const instruction_info_t info = { \
                .name = #QNAME, \
                .type = VALUE, \
                .operand_count = OPERANDS, \
                .description = DESC, \
                .group = GROUP \
            }; \
            return &info; \
        }
    BASIC_INSTRUCTION_LIST(X)
    #undef X
    
    return NULL;
}

bool trias_instruction_parse(const char* text, trias_instruction_t* out_instruction) {
    if (!text || !out_instruction) return false;
    
    // Простейшая реализация для начала - парсим только имя инструкции
    char name[32] = {0};
    sscanf(text, "%31s", name);
    
    // Ищем информацию об инструкции
    const instruction_info_t* info = find_instruction(name);
    if (!info) return false;
    
    // Заполняем базовую информацию
    out_instruction->name = info->name;
    out_instruction->opcode = info->type;
    out_instruction->operand_count = 0;
    
    return true;
}

bool trias_instruction_encode(const trias_instruction_t* inst, tryte_t* out_bytes) {
    if (!inst || !out_bytes) return false;
    
    // Находим информацию об инструкции
    const instruction_info_t* info = find_instruction(inst->name);
    if (!info) return false;
    
    // Базовый опкод
    out_bytes[0] = make_opcode(0, inst->opcode);  // Режим адресации 0 для базового опкода
    
    // Кодируем операнды
    for (size_t i = 0; i < inst->operand_count && i < 2; i++) {
        const operand_t* op = &inst->operands[i];
        // Создаем троичное значение для операнда
        out_bytes[i + 1] = make_opcode(op->addressing_mode, op->value);
    }
    
    return true;
}

const char* trias_instruction_get_group(const char* name) {
    const instruction_info_t* info = find_instruction(name);
    return info ? info->group : NULL;
}

const char* trias_instruction_get_description(const char* name) {
    const instruction_info_t* info = find_instruction(name);
    return info ? info->description : NULL;
}