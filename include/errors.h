#ifndef ZARYA_ERRORS_H
#define ZARYA_ERRORS_H

// Коды ошибок
typedef enum {
    VM_OK = 0,                    // Успешное выполнение
    VM_ERROR_INVALID_ADDRESS,     // Неверный адрес
    VM_ERROR_INVALID_OPCODE,      // Неверный код операции
    VM_ERROR_INVALID_PARAMETER,   // Неверный параметр
    VM_ERROR_STACK_OVERFLOW,      // Переполнение стека
    VM_ERROR_STACK_UNDERFLOW,     // Стек пуст
    VM_ERROR_DIVISION_BY_ZERO,    // Деление на ноль
    VM_ERROR_OUT_OF_MEMORY,       // Недостаточно памяти
    VM_ERROR_INVALID_INTERRUPT,   // Неверный тип прерывания
    VM_ERROR_INTERRUPTS_DISABLED,   // Прерывания запрещены
    VM_ERROR_NO_INTERRUPT_HANDLER,   // Не установлен обработчик прерываний
    VM_ERROR_HALT,               // Программа завершена (HALT)
    VM_ERROR_INVALID_INSTRUCTION, // Неверная инструкция
    VM_ERROR_SYNTAX,            // Ошибка синтаксиса
    VM_ERROR_INVALID_REGISTER,  // Неверный номер регистра
    VM_ERROR_INVALID_ADDRESSING_MODE,  // Неверный режим адресации
} vm_error_t;

#endif // ZARYA_ERRORS_H 