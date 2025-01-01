#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "zarya_vm.h"
#include "instructions.h"

vm_error_t vm_init(vm_state_t* vm, size_t memory_size) {
    if (!vm || memory_size == 0) {
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    // Проверяем, что размер памяти не превышает максимально допустимый
    if (memory_size > MEMORY_SIZE_TRYTES) {
        memory_size = MEMORY_SIZE_TRYTES;
    }
    
    // Выделяем память
    vm->memory = (tryte_t*)calloc(memory_size, sizeof(tryte_t));
    if (!vm->memory) {
        return VM_ERROR_OUT_OF_MEMORY;
    }
    
    vm->memory_size = memory_size;
    
    // Инициализируем каждый трайт в памяти
    for (size_t i = 0; i < memory_size; i++) {
        vm->memory[i] = TRYTE_FROM_INT(0);
    }
    
    // Инициализируем регистры
    for (int i = 0; i < NUM_REGISTERS; i++) {
        vm->registers[i] = TRYTE_FROM_INT(0);
    }
    
    // Инициализируем PC и SP
    vm->pc = TRYTE_FROM_INT(0);  // Программа начинается с начала памяти
    vm->sp = TRYTE_FROM_INT(-1);  // Стек пуст, указатель на -1
    
    // Инициализируем флаги
    vm->flags = TRYTE_FROM_INT(0);
    
    // Инициализируем обработчик прерываний
    vm->interrupt_callback = NULL;
    vm->interrupt_context = NULL;
    
    printf("DEBUG: vm_init: memory_size=%zu, sp=%d\n", vm->memory_size, vm->sp.value);
    
    return VM_OK;
}

void vm_free(vm_state_t* vm) {
    if (vm && vm->memory) {
        free(vm->memory);
        vm->memory = NULL;
        vm->memory_size = 0;
    }
}

void vm_reset(vm_state_t* vm) {
    if (!vm) {
        return;
    }
    
    // Очищаем память
    if (vm->memory) {
        for (size_t i = 0; i < vm->memory_size; i++) {
            vm->memory[i] = TRYTE_FROM_INT(0);
        }
    }
    
    // Сбрасываем регистры
    for (int i = 0; i < NUM_REGISTERS; i++) {
        vm->registers[i] = TRYTE_FROM_INT(0);
    }
    
    // Сбрасываем PC и SP
    vm->pc = TRYTE_FROM_INT(0);  // Программа начинается с начала памяти
    vm->sp = TRYTE_FROM_INT(-1);  // Стек пуст
    
    // Сбрасываем флаги
    vm->flags = TRYTE_FROM_INT(0);
    
    // Сохраняем обработчик прерываний
    // vm->interrupt_callback и vm->interrupt_context не меняются
}

vm_error_t vm_load_program(vm_state_t* vm, const tryte_t* program, size_t size) {
    if (!vm || !program || size == 0) {
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    if (size > vm->memory_size) {
        return VM_ERROR_OUT_OF_MEMORY;
    }
    
    // Сбрасываем состояние VM перед загрузкой новой программы
    vm_reset(vm);
    
    // Копируем программу в память
    for (size_t i = 0; i < size; i++) {
        memcpy(vm->memory[i].trits, program[i].trits, TRITS_PER_TRYTE);
        update_tryte_value(&vm->memory[i]);  // Обновляем значение после копирования
        printf("Loaded at %zu: value=%d\n", i, vm->memory[i].value);
    }
    
    // Устанавливаем PC на начало программы
    vm->pc = TRYTE_FROM_INT(0);
    
    return VM_OK;
}

vm_error_t vm_step(vm_state_t* vm) {
    if (!vm) {
        return VM_ERROR_INVALID_PARAMETER;
    }

    // Проверяем выход за границы памяти
    if (vm->pc.value < 0 || (size_t)vm->pc.value >= vm->memory_size) {
        return VM_ERROR_INVALID_ADDRESS;
    }

    // Читаем инструкцию
    instruction_t inst;
    if ((size_t)vm->pc.value + 2 >= vm->memory_size) {
        return VM_ERROR_INVALID_ADDRESS;
    }
    inst.opcode = vm->memory[vm->pc.value];
    inst.operand1 = vm->memory[vm->pc.value + 1];
    inst.operand2 = vm->memory[vm->pc.value + 2];

    // Выводим содержимое памяти для отладки
    printf("Memory at PC=%d: [%d, %d, %d]\n", 
           vm->pc.value,
           vm->memory[vm->pc.value].value,
           vm->memory[vm->pc.value + 1].value,
           vm->memory[vm->pc.value + 2].value);
    
    // Получаем базовый опкод
    int base_opcode = GET_BASE_OPCODE(inst.opcode.value);
    
    // Проверяем, что базовый опкод в допустимом диапазоне
    if (base_opcode < 0 || base_opcode > 100) {
        printf("Invalid opcode: %d at PC=%d\n", inst.opcode.value, vm->pc.value);
        return VM_ERROR_INVALID_OPCODE;
    }
    
    // Сохраняем старое значение PC
    int old_pc = vm->pc.value;
    
    // Выполняем инструкцию
    vm_error_t err = execute_instruction(vm, &inst);
    if (err != VM_OK) {
        return err;
    }
    
    // Увеличиваем PC только если он не был изменен инструкцией
    if (vm->pc.value == old_pc) {
        vm->pc.value += 3;  // Пропускаем опкод и два операнда
    }
    
    // Проверяем новое значение PC
    if ((size_t)vm->pc.value >= vm->memory_size) {
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    return VM_OK;
}

vm_error_t vm_run(vm_state_t* vm) {
    if (!vm) {
        return VM_ERROR_INVALID_PARAMETER;
    }

    while (true) {
        if (vm->pc.value < 0 || (size_t)vm->pc.value >= vm->memory_size) {
            return VM_ERROR_INVALID_ADDRESS;
        }

        vm_error_t result = vm_step(vm);
        
        // Проверяем результат выполнения
        if (result == VM_ERROR_HALT) {
            return VM_OK;  // Нормальное завершение по HALT
        }
        if (result != VM_OK) {
            return result;  // Возвращаем ошибку
        }
    }
}

// Установка callback'а для прерываний
void vm_set_interrupt_handler(vm_state_t* vm, vm_interrupt_callback_t callback, void* context) {
    if (vm) {
        vm->interrupt_callback = callback;
        vm->interrupt_context = context;
    }
} 