#ifndef ZARYA_CONFIG_H
#define ZARYA_CONFIG_H

// Версия проекта
#define ZARYA_VERSION_MAJOR 0
#define ZARYA_VERSION_MINOR 1
#define ZARYA_VERSION_PATCH 0

// Константы для работы с тритами
#define TRITS_PER_TRYTE 6
#define TRITS_PER_WORD 18

// Размеры памяти
#define MAX_ADDRESS 364  // Максимальный адрес для трайта (3^5 + 3^4 + 3^3 + 3^2 + 3^1 + 3^0)
#define MEMORY_SIZE_TRYTES (MAX_ADDRESS + 1)  // Размер памяти в трайтах

// Константы для тритов
#define TRIT_NEGATIVE -1
#define TRIT_ZERO 0
#define TRIT_POSITIVE 1

// Настройки механизма прерываний
#define MAX_INTERRUPTS 16         // Максимальное количество прерываний

// Стандартные прерывания
#define INT_PUTCHAR 1            // Вывод символа
#define INT_GETCHAR 2            // Ввод символа
#define INT_PUTS 3               // Вывод строки
#define INT_GETS 4               // Ввод строки
#define INT_CLEAR 5              // Очистка экрана
#define INT_SETPOS 6             // Установка позиции курсора

// Флаги для отладки
#ifdef DEBUG
    #define ZARYA_DEBUG 1
#else
    #define ZARYA_DEBUG 0
#endif

// Размер инструкции в трайтах (опкод + два операнда)
#define INSTRUCTION_SIZE 3

// Регистры
#define NUM_REGISTERS 4          // Количество регистров общего назначения (R0-R3)

// Системные регистры
#define REG_PC 0                // Program Counter
#define REG_SP 1                // Stack Pointer

// Регистры общего назначения
#define REG_ACC 0               // R0: Аккумулятор
#define REG_IDX 1               // R1: Индекс
#define REG_TMP1 2              // R2: Временный 1
#define REG_TMP2 3              // R3: Временный 2

// Флаги состояния
#define FLAG_INTERRUPTS_ENABLED 0x01  // Прерывания разрешены

#endif // ZARYA_CONFIG_H 