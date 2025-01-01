#ifndef INSTRUCTION_DEFS_H
#define INSTRUCTION_DEFS_H

#include "types.h"
#include "trit_ops.h"
#include <stdio.h>
#include <string.h>  // Для memcpy

// Режимы адресации (используют старший трит опкода)
#define ADDR_MODE_IMMEDIATE (-1)  // Непосредственный режим (#value)
#define ADDR_MODE_REGISTER  0     // Регистровый режим (Rn)
#define ADDR_MODE_INDIRECT  1     // Косвенный через регистр (@Rn)

// Получение режима адресации и базового опкода
static inline trit_t get_addr_mode(tryte_t opcode) {
    return opcode.trits[TRITS_PER_TRYTE - 1];  // Старший трит
}

static inline int get_base_opcode(tryte_t opcode) {
    // Копируем опкод, чтобы не изменять оригинал
    tryte_t t = opcode;
        
    // Очищаем старший трит (режим адресации)
    t.trits[TRITS_PER_TRYTE - 1] = 0;
    update_tryte_value(&t);
    
    return t.value;
}

#define GET_ADDR_MODE(opcode)     get_addr_mode(create_tryte_from_int(opcode))
#define GET_BASE_OPCODE(opcode)   get_base_opcode(create_tryte_from_int(opcode))
#define IS_IMMEDIATE(opcode)      (GET_ADDR_MODE(opcode) == ADDR_MODE_IMMEDIATE)
#define IS_REGISTER(opcode)       (GET_ADDR_MODE(opcode) == ADDR_MODE_REGISTER)
#define IS_INDIRECT(opcode)       (GET_ADDR_MODE(opcode) == ADDR_MODE_INDIRECT)

// Макрос для комбинирования режима адресации и опкода
static inline tryte_t make_opcode(trit_t mode, int op) {
    // Проверяем, что режим адресации в допустимом диапазоне
    if (mode != ADDR_MODE_IMMEDIATE && mode != ADDR_MODE_REGISTER && mode != ADDR_MODE_INDIRECT) {
        printf("Warning: invalid addressing mode %d\n", mode);
        mode = ADDR_MODE_IMMEDIATE;  // По умолчанию используем непосредственный режим
    }
    
    // Проверяем, что базовый опкод в допустимом диапазоне
    if (op < -121 || op > 121) {
        printf("Warning: base opcode %d out of range [-121, 121]\n", op);
    }
    
    // Создаем трайт с базовым опкодом
    tryte_t result = create_tryte_from_int(op);
    print_tryte("make_opcode after create", &result);
    
    // Устанавливаем режим адресации в старший трит
    result.trits[TRITS_PER_TRYTE - 1] = mode;
    update_tryte_value(&result);
    
    print_tryte("make_opcode final", &result);
    
    return result;
}
#define MAKE_OPCODE(mode, op) make_opcode(mode, op)

// Базовые инструкции (транслируются напрямую в машинный код)
#define BASIC_INSTRUCTION_LIST(X) \
    /* Нет операции */ \
    X(NOP,    0,  0, "Нет операции", "Системные", NULL) \
    \
    /* Стековые операции */ \
    X(PUSH,   1,  1, "Положить значение в стек (поддерживает все режимы адресации)", "Стек", NULL) \
    X(POP,    2,  1, "Снять значение со стека (поддерживает регистровый и прямой режимы)", "Стек", NULL) \
    X(DUP,    3,  0, "Дублировать верхний элемент стека", "Стек", NULL) \
    X(SWAP,   4,  0, "Поменять местами два верхних элемента", "Стек", NULL) \
    X(DROP,   5,  0, "Удалить верхний элемент", "Стек", NULL) \
    X(OVER,   6,  0, "Скопировать предпоследний элемент на вершину", "Стек", NULL) \
    \
    /* Арифметические операции */ \
    X(ADD,   10,  0, "Сложить два верхних элемента", "Арифметика", NULL) \
    X(SUB,   11,  0, "Вычесть верхний элемент из предыдущего", "Арифметика", NULL) \
    X(MUL,   12,  0, "Умножить два верхних элемента", "Арифметика", NULL) \
    X(DIV,   13,  0, "Разделить предыдущий элемент на верхний", "Арифметика", NULL) \
    \
    /* Логические операции */ \
    X(AND,   20,  0, "Логическое И", "Логика", NULL) \
    X(OR,    21,  0, "Логическое ИЛИ", "Логика", NULL) \
    X(NOT,   22,  0, "Логическое НЕ", "Логика", NULL) \
    \
    /* Операции сравнения */ \
    X(EQ,    30,  0, "Равно", "Сравнение", NULL) \
    X(NEQ,   31,  0, "Не равно", "Сравнение", NULL) \
    X(LT,    32,  0, "Меньше", "Сравнение", NULL) \
    X(GT,    33,  0, "Больше", "Сравнение", NULL) \
    X(LE,    34,  0, "Меньше или равно", "Сравнение", NULL) \
    X(GE,    35,  0, "Больше или равно", "Сравнение", NULL) \
    \
    /* Операции управления */ \
    X(JMP,   40,  1, "Безусловный переход", "Управление", NULL) \
    X(JZ,    41,  1, "Переход если ноль", "Управление", NULL) \
    X(JNZ,   42,  1, "Переход если не ноль", "Управление", NULL) \
    X(CALL,  43,  1, "Вызов подпрограммы", "Управление", NULL) \
    X(RET,   44,  0, "Возврат из подпрограммы", "Управление", NULL) \
    X(HALT,  45,  0, "Остановка программы", "Управление", NULL) \
    \
    /* Операции ввода-вывода */ \
    X(IN,    50,  0, "Ввод значения", "Ввод-вывод", NULL) \
    X(OUT,   51,  0, "Вывод значения", "Ввод-вывод", NULL) \
    \
    /* Операции с памятью */ \
    X(LOAD,  60,  2, "Загрузить значение из памяти", "Память", NULL) \
    X(STORE, 61,  2, "Сохранить значение в память", "Память", NULL) \
    \
    /* Системные операции */ \
    X(INT,   70,  0, "Вызов прерывания", "Системные", NULL) \
    X(CLI,   71,  0, "Запретить прерывания", "Системные", NULL) \
    X(STI,   72,  0, "Разрешить прерывания", "Системные", NULL)

// Псевдоинструкции (транслируются в последовательность базовых инструкций)
#define PSEUDO_INSTRUCTION_LIST(X) \
    /* Работа с регистрами */ \
    X(MOV,  100,  2, "Пересылка данных между регистрами", "Псевдоинструкции", handle_mov) \
    X(INC,  101,  1, "Инкремент значения в регистре", "Псевдоинструкции", handle_inc) \
    X(DEC,  102,  1, "Декремент значения в регистре", "Псевдоинструкции", handle_dec) \
    \
    /* Работа со стеком */ \
    X(PUSHR, 110,  1, "Положить значение регистра в стек", "Псевдоинструкции", handle_pushr) \
    X(POPR,  111,  1, "Снять значение со стека в регистр", "Псевдоинструкции", handle_popr) \
    X(CLEAR, 112,  1, "Очистить N элементов стека", "Псевдоинструкции", handle_clear) \
    \
    /* Сравнения и тесты */ \
    X(CMP,  120,  2, "Сравнить два значения", "Псевдоинструкции", handle_cmp) \
    X(TEST, 121,  1, "Проверить биты значения", "Псевдоинструкции", handle_test)

// Полный список инструкций (включает базовые и псевдоинструкции)
#define INSTRUCTION_LIST(X) \
    BASIC_INSTRUCTION_LIST(X) \
    PSEUDO_INSTRUCTION_LIST(X)

#endif // INSTRUCTION_DEFS_H 