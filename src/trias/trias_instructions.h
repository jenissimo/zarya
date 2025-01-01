#ifndef TRIAS_INSTRUCTIONS_H
#define TRIAS_INSTRUCTIONS_H

#include "zarya_vm.h"
#include "ast.h"
#include "codegen.h"

// Тип обработчика инструкции
typedef vm_error_t (*instruction_handler_t)(codegen_t* gen, ast_node_t** operands);

// Информация об инструкции
typedef struct {
    const char* name;          // Имя инструкции
    int type;                  // Тип инструкции (опкод)
    int operand_count;         // Количество операндов
    const char* description;   // Описание инструкции
    const char* group;         // Группа инструкций
    instruction_handler_t handler; // Обработчик (NULL для базовых инструкций)
} instruction_info_t;

// Получение информации об инструкции по имени
const instruction_info_t* get_instruction_info(const char* name);

// Обработчики псевдоинструкций
vm_error_t handle_mov(codegen_t* gen, ast_node_t** operands);
vm_error_t handle_inc(codegen_t* gen, ast_node_t** operands);
vm_error_t handle_dec(codegen_t* gen, ast_node_t** operands);
vm_error_t handle_pushr(codegen_t* gen, ast_node_t** operands);
vm_error_t handle_popr(codegen_t* gen, ast_node_t** operands);
vm_error_t handle_clear(codegen_t* gen, ast_node_t** operands);
vm_error_t handle_cmp(codegen_t* gen, ast_node_t** operands);
vm_error_t handle_test(codegen_t* gen, ast_node_t** operands);

#endif // TRIAS_INSTRUCTIONS_H 