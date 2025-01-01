#ifndef ZARYA_VM_H
#define ZARYA_VM_H

#include <stddef.h>
#include "types.h"
#include "errors.h"
#include "zarya_config.h"

// Callback для обработки прерываний
typedef vm_error_t (*vm_interrupt_callback_t)(void* context, int int_num);

// Состояние виртуальной машины
typedef struct {
    // Память и стек
    tryte_t* memory;          // Общая память (включая стек)
    size_t memory_size;       // Размер памяти в трайтах
    
    // Системные регистры
    tryte_t pc;              // Program Counter
    tryte_t sp;              // Stack Pointer
    tryte_t flags;           // Флаги состояния
    
    // Регистры общего назначения
    tryte_t registers[NUM_REGISTERS];  // R0-R3
    
    // Обработка прерываний
    vm_interrupt_callback_t interrupt_callback;  // Callback для прерываний
    void* interrupt_context;                     // Контекст для callback'а
} vm_state_t;

// Инициализация виртуальной машины
vm_error_t vm_init(vm_state_t* vm, size_t memory_size);

// Освобождение ресурсов виртуальной машины
void vm_free(vm_state_t* vm);

// Сброс состояния виртуальной машины
void vm_reset(vm_state_t* vm);

// Загрузка программы в память
vm_error_t vm_load_program(vm_state_t* vm, const tryte_t* program, size_t size);

// Выполнение одного шага программы
vm_error_t vm_step(vm_state_t* vm);

// Выполнение программы до инструкции HALT
vm_error_t vm_run(vm_state_t* vm);

// Установка callback'а для прерываний
void vm_set_interrupt_handler(vm_state_t* vm, vm_interrupt_callback_t callback, void* context);

#endif // ZARYA_VM_H 