#ifndef ZARYA_EMULATOR_H
#define ZARYA_EMULATOR_H

#include <stdbool.h>
#include "zarya_vm.h"

// Типы прерываний эмулятора
typedef enum {
    EMU_INT_PUTCHAR = 0,  // Вывод символа
    EMU_INT_GETCHAR,      // Ввод символа
    EMU_INT_PUTS,         // Вывод строки
    EMU_INT_GETS,         // Ввод строки
    EMU_INT_CLEAR,        // Очистка экрана
    EMU_INT_SETPOS,       // Установка позиции курсора
    EMU_INT_TIMER,        // Таймер
    EMU_INT_KEYBOARD,     // Клавиатура
    EMU_INT_COUNT         // Количество прерываний
} emu_interrupt_t;

// Обработчик прерывания
typedef vm_error_t (*interrupt_handler_t)(vm_state_t* vm);

// Состояние эмулятора
typedef struct {
    vm_state_t* vm;                                  // Виртуальная машина
    interrupt_handler_t handlers[EMU_INT_COUNT];     // Обработчики прерываний
    bool interrupts_enabled;                         // Флаг разрешения прерываний
    void* device_contexts[EMU_INT_COUNT];           // Контексты устройств
} emulator_t;

// Инициализация эмулятора
vm_error_t emulator_init(emulator_t* emu, vm_state_t* vm);

// О��вобождение ресурсов эмулятора
void emulator_free(emulator_t* emu);

// Регистрация обработчика прерывания
vm_error_t emulator_register_handler(emulator_t* emu, emu_interrupt_t type, 
                                   interrupt_handler_t handler, void* context);

// Генерация прерывания
vm_error_t emulator_raise_interrupt(emulator_t* emu, emu_interrupt_t type);

// Обработка прерывания
vm_error_t emulator_handle_interrupt(emulator_t* emu, int int_num);

// Выполнение одного шага эмулятора
vm_error_t emulator_step(emulator_t* emu);

#endif // ZARYA_EMULATOR_H 