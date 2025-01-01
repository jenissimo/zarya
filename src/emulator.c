#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "emulator.h"
#include "zarya_config.h"
#include "stack.h"

// Стандартные обработчики прерываний
static vm_error_t handle_putchar(vm_state_t* vm) {
    if (vm->sp.value < 1) return VM_ERROR_STACK_UNDERFLOW;
    
    char c = (char)vm->memory[vm->sp.value].value;
    vm->sp.value--;
    putchar(c);
    return VM_OK;
}

static vm_error_t handle_getchar(vm_state_t* vm) {
    if (vm->sp.value >= MEMORY_SIZE_TRYTES - 1) 
        return VM_ERROR_STACK_OVERFLOW;
    
    vm->sp.value++;
    vm->memory[vm->sp.value].value = getchar();
    return VM_OK;
}

static vm_error_t handle_puts(vm_state_t* vm) {
    if (!vm) {
        return VM_ERROR_INVALID_PARAMETER;
    }
    
    // Получаем адрес строки из стека
    tryte_t addr_tryte;
    vm_error_t err = stack_pop(vm, &addr_tryte);
    if (err != VM_OK) {
        return err;
    }
    
    int addr = addr_tryte.value;
    if (addr < 0) {
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    // Выводим строку до нулевого символа или конца памяти
    while ((size_t)addr < vm->memory_size && vm->memory[addr].value != 0) {
        putchar(vm->memory[addr].value);
        addr++;
    }
    
    return VM_OK;
}

static vm_error_t handle_gets(vm_state_t* vm) {
    if (vm->sp.value < 2) return VM_ERROR_STACK_UNDERFLOW;
    
    int maxlen = vm->memory[vm->sp.value].value;
    vm->sp.value--;
    int addr = vm->memory[vm->sp.value].value;
    vm->sp.value--;
    
    char c;
    int i = 0;
    while (i < maxlen - 1 && (c = getchar()) != '\n' && c != EOF) {
        vm->memory[addr + i].value = c;
        i++;
    }
    vm->memory[addr + i].value = 0;
    return VM_OK;
}

static vm_error_t handle_clear(vm_state_t* vm) {
    (void)vm;  // Подавляем предупреждение о неиспользуемом параметре
    printf("\033[2J\033[H");  // ANSI escape sequence для очистки экрана
    return VM_OK;
}

static vm_error_t handle_setpos(vm_state_t* vm) {
    if (vm->sp.value < 2) return VM_ERROR_STACK_UNDERFLOW;
    
    int col = vm->memory[vm->sp.value].value;
    vm->sp.value--;
    int row = vm->memory[vm->sp.value].value;
    vm->sp.value--;
    
    printf("\033[%d;%dH", row, col);
    return VM_OK;
}

// Callback для VM, который вызывает обработчик прерываний эмулятора
static vm_error_t interrupt_callback(void* context, int int_num) {
    emulator_t* emu = (emulator_t*)context;
    return emulator_handle_interrupt(emu, int_num);
}

vm_error_t emulator_init(emulator_t* emu, vm_state_t* vm) {
    if (!emu || !vm) return VM_ERROR_INVALID_ADDRESS;
    
    memset(emu, 0, sizeof(emulator_t));
    emu->vm = vm;
    emu->interrupts_enabled = true;
    
    // Регистрируем стандартные обработчики
    emulator_register_handler(emu, EMU_INT_PUTCHAR, handle_putchar, NULL);
    emulator_register_handler(emu, EMU_INT_GETCHAR, handle_getchar, NULL);
    emulator_register_handler(emu, EMU_INT_PUTS, handle_puts, NULL);
    emulator_register_handler(emu, EMU_INT_GETS, handle_gets, NULL);
    emulator_register_handler(emu, EMU_INT_CLEAR, handle_clear, NULL);
    emulator_register_handler(emu, EMU_INT_SETPOS, handle_setpos, NULL);
    
    // Устанавливаем callback в VM
    vm_set_interrupt_handler(vm, interrupt_callback, emu);
    
    return VM_OK;
}

void emulator_free(emulator_t* emu) {
    if (!emu) return;
    
    // Освобождаем контексты устройств
    for (int i = 0; i < EMU_INT_COUNT; i++) {
        if (emu->device_contexts[i]) {
            free(emu->device_contexts[i]);
            emu->device_contexts[i] = NULL;
        }
    }
}

vm_error_t emulator_register_handler(emulator_t* emu, emu_interrupt_t type,
                                   interrupt_handler_t handler, void* context) {
    if (!emu || type >= EMU_INT_COUNT) return VM_ERROR_INVALID_ADDRESS;
    
    emu->handlers[type] = handler;
    emu->device_contexts[type] = context;
    
    return VM_OK;
}

vm_error_t emulator_raise_interrupt(emulator_t* emu, emu_interrupt_t type) {
    if (!emu || type >= EMU_INT_COUNT) return VM_ERROR_INVALID_ADDRESS;
    
    // Проверяем, разрешены ли прерывания
    if (!emu->interrupts_enabled) return VM_OK;
    
    // Вызываем обработчик прерывания
    if (emu->handlers[type]) {
        return emu->handlers[type](emu->vm);
    }
    
    return VM_ERROR_INVALID_INTERRUPT;
}

vm_error_t emulator_handle_interrupt(emulator_t* emu, int int_num) {
    if (!emu || int_num >= EMU_INT_COUNT) return VM_ERROR_INVALID_ADDRESS;
    
    // Проверяем, разрешены ли прерывания
    if (!emu->interrupts_enabled) return VM_OK;
    
    // Вызываем обработчик прерывания
    if (emu->handlers[int_num]) {
        return emu->handlers[int_num](emu->vm);
    }
    
    return VM_ERROR_INVALID_INTERRUPT;
}

vm_error_t emulator_step(emulator_t* emu) {
    if (!emu || !emu->vm) return VM_ERROR_INVALID_ADDRESS;
    
    // Проверяем наличие прерываний от устройств
    // (здесь можно добавить опрос состояния таймера, клавиатуры и т.д.)
    
    // Выполняем одну инструкцию VM
    return vm_step(emu->vm);
}

vm_error_t emulator_putchar(vm_state_t* vm) {
    if (!vm) return VM_ERROR_INVALID_ADDRESS;
    
    // Проверяем, есть ли символ в стеке
    if (vm->sp.value < 0) return VM_ERROR_STACK_UNDERFLOW;
    
    // Снимаем символ со стека и выводим
    char c = (char)vm->memory[vm->sp.value].value;
    TRYTE_DEC(vm->sp);
    putchar(c);
    return VM_OK;
}

vm_error_t emulator_getchar(vm_state_t* vm) {
    if (!vm) return VM_ERROR_INVALID_ADDRESS;
    
    // Проверяем, есть ли место в стеке
    if ((size_t)(vm->sp.value + 1) >= vm->memory_size) 
        return VM_ERROR_STACK_OVERFLOW;
    
    // Читаем символ и кладем в стек
    TRYTE_INC(vm->sp);
    TRYTE_SET_VALUE(vm->memory[vm->sp.value], getchar());
    return VM_OK;
}

vm_error_t emulator_puts(vm_state_t* vm, int addr) {
    if (!vm) {
        return VM_ERROR_INVALID_PARAMETER;
    }
    
    if (addr < 0) {
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    // Выводим строку до нулевого символа или конца памяти
    while ((size_t)addr < vm->memory_size && vm->memory[addr].value != 0) {
        putchar(vm->memory[addr].value);
        addr++;
    }
    
    return VM_OK;
}

vm_error_t emulator_gets(vm_state_t* vm, int addr, size_t maxlen) {
    if (!vm) {
        return VM_ERROR_INVALID_PARAMETER;
    }
    
    if (addr < 0 || (size_t)addr + maxlen >= vm->memory_size) {
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    // Читаем строку
    size_t i;
    for (i = 0; i < maxlen - 1; i++) {
        int ch = getchar();
        if (ch == EOF || ch == '\n') {
            break;
        }
        vm->memory[addr + i] = TRYTE_FROM_INT(ch);
    }
    
    // Добавляем завершающий нуль
    vm->memory[addr + i] = TRYTE_FROM_INT(0);
    
    return VM_OK;
}

vm_error_t emulator_clear(vm_state_t* vm) {
    if (!vm) return VM_ERROR_INVALID_ADDRESS;
    
    // Очищаем экран
    printf("\033[2J\033[H");
    return VM_OK;
}

vm_error_t emulator_setpos(vm_state_t* vm) {
    if (!vm) return VM_ERROR_INVALID_ADDRESS;
    
    // Проверяем, есть ли два параметра в стеке
    if (vm->sp.value < 1) return VM_ERROR_STACK_UNDERFLOW;
    
    // Снимаем параметры со стека
    int col = vm->memory[vm->sp.value].value;
    TRYTE_DEC(vm->sp);
    
    int row = vm->memory[vm->sp.value].value;
    TRYTE_DEC(vm->sp);
    
    // Устанавливаем позицию курсора
    printf("\033[%d;%dH", row, col);
    return VM_OK;
} 