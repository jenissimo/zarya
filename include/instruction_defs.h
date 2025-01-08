#ifndef INSTRUCTION_DEFS_H
#define INSTRUCTION_DEFS_H

#include "types.h"
#include "trit_ops.h"
#include <stdio.h>
#include <string.h>  // Для memcpy
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Режимы адресации в опкоде (старший трит)
//-----------------------------------------------------------------------------
typedef enum {
    ADDR_MODE_IMMEDIATE = -1,  // Непосредственный режим (#value)
    ADDR_MODE_REGISTER  = 0,   // Регистровый режим (Rn)
    ADDR_MODE_INDIRECT  = 1    // Косвенный через регистр (@Rn)
} addr_mode_t;

//-----------------------------------------------------------------------------
// Маски режимов адресации для проверки допустимых режимов
//-----------------------------------------------------------------------------
typedef enum {
    ADDR_MODE_NONE      = 0,            // Нет операндов
    ADDR_MODE_IMM       = 1 << 0,       // Непосредственный (#value)
    ADDR_MODE_REG       = 1 << 1,       // Регистровый (Rn)
    ADDR_MODE_IND       = 1 << 2,       // Косвенный (@Rn)
    ADDR_MODE_ALL       = ADDR_MODE_IMM | ADDR_MODE_REG | ADDR_MODE_IND,  // Все режимы
    ADDR_MODE_REG_IND   = ADDR_MODE_REG | ADDR_MODE_IND,                  // Регистровый и косвенный
    ADDR_MODE_IMM_REG   = ADDR_MODE_IMM | ADDR_MODE_REG                   // Непосредственный и регистровый
} addr_mode_mask_t;

//-----------------------------------------------------------------------------
// Опкоды инструкций
//-----------------------------------------------------------------------------
typedef enum {
    // Нет операции (0)
    OP_NOP = 0,

    // Стековые операции (1-9)
    OP_PUSH = 1,   // Положить значение в стек
    OP_POP  = 2,   // Снять значение со стека
    OP_DUP  = 3,   // Дублировать верхний элемент
    OP_SWAP = 4,   // Поменять местами два верхних элемента
    OP_DROP = 5,   // Удалить верхний элемент
    OP_OVER = 6,   // Скопировать предпоследний элемент

    // Арифметические операции (10-19)
    OP_ADD = 10,   // Сложение
    OP_SUB = 11,   // Вычитание
    OP_MUL = 12,   // Умножение
    OP_DIV = 13,   // Деление

    // Логические операции (20-29)
    OP_AND = 20,   // Логическое И
    OP_OR  = 21,   // Логическое ИЛИ
    OP_NOT = 22,   // Логическое НЕ

    // Операции сравнения (30-39)
    OP_EQ  = 30,   // Равно
    OP_NEQ = 31,   // Не равно
    OP_LT  = 32,   // Меньше
    OP_GT  = 33,   // Больше
    OP_LE  = 34,   // Меньше или равно
    OP_GE  = 35,   // Больше или равно

    // Операции управления (40-49)
    OP_JMP  = 40,  // Безусловный переход
    OP_JZ   = 41,  // Переход если ноль
    OP_JNZ  = 42,  // Переход если не ноль
    OP_CALL = 43,  // Вызов подпрограммы
    OP_RET  = 44,  // Возврат из подпрограммы
    OP_HALT = 45,  // Остановка программы

    // Операции с памятью (60-69)
    OP_LOAD  = 60, // Загрузить значение из памяти в стек
    OP_STORE = 61, // Сохранить значение из стека в память

    // Системные операции (70-79)
    OP_INT = 70,   // Вызов прерывания
    OP_CLI = 71,   // Запретить прерывания
    OP_STI = 72,   // Разрешить прерывания

    // Псевдоинструкции (100+)
    OP_MOV   = 100, // Пересылка данных между регистрами
    OP_INC   = 101, // Инкремент значения в регистре
    OP_DEC   = 102, // Декремент значения в регистре
    OP_PUSHR = 110, // Положить значение регистра в стек
    OP_POPR  = 111, // Снять значение со стека в регистр
    OP_CLEAR = 112, // Очистить N элементов стека
    OP_CMP   = 120, // Сравнить два значения
    OP_TEST  = 121, // Проверить биты значения

    OP_COUNT  // Количество опкодов
} opcode_t;

//-----------------------------------------------------------------------------
// Диапазоны опкодов
//-----------------------------------------------------------------------------
#define DEFINE_OPCODE_RANGES(X) \
    /* Стековые операции */ \
    X(STACK,      1,  6) \
    /* Арифметические операции */ \
    X(ARITHMETIC, 10, 13) \
    /* Логические операции */ \
    X(LOGICAL,    20, 22) \
    /* Операции сравнения */ \
    X(COMPARISON, 30, 35) \
    /* Операции управления */ \
    X(CONTROL,    40, 45) \
    /* Операции с памятью */ \
    X(MEMORY,     60, 61) \
    /* Операции прерываний */ \
    X(INTERRUPT,  70, 72) \
    /* Псевдоинструкции */ \
    X(PSEUDO,    100, 121)

// Определяем константы диапазонов
#define DEFINE_RANGE(name, start, end) \
    enum { \
        name##_OPCODE_START = start, \
        name##_OPCODE_END = end \
    };

DEFINE_OPCODE_RANGES(DEFINE_RANGE)

#undef DEFINE_RANGE

// Макрос для проверки принадлежности опкода к диапазону
#define IS_OPCODE_IN_RANGE(opcode, start, end) ((opcode) >= (start) && (opcode) <= (end))

// Макросы для проверки типа инструкции
#define DEFINE_IS_OPCODE(name, start, end) \
    static inline bool IS_##name##_OPCODE(int opcode) { \
        return IS_OPCODE_IN_RANGE(opcode, name##_OPCODE_START, name##_OPCODE_END); \
    }

DEFINE_OPCODE_RANGES(DEFINE_IS_OPCODE)

#undef DEFINE_IS_OPCODE
#undef DEFINE_OPCODE_RANGES

//-----------------------------------------------------------------------------
// Функции для работы с режимами адресации
//-----------------------------------------------------------------------------
// Получение режима адресации и базового опкода
static inline trit_t get_addr_mode(tryte_t opcode) {
    return opcode.trits[TRITS_PER_TRYTE - 1];  // Старший трит
}

static inline int get_base_opcode(tryte_t opcode) {
    tryte_t t = opcode;
    t.trits[TRITS_PER_TRYTE - 1] = 0;  // Очищаем старший трит
    update_tryte_value(&t);
    return t.value;
}

#define GET_ADDR_MODE(opcode)     get_addr_mode(create_tryte_from_int(opcode))
#define GET_BASE_OPCODE(opcode)   get_base_opcode(create_tryte_from_int(opcode))

// Проверка режима адресации
static inline bool is_immediate(int opcode) {
    return GET_ADDR_MODE(opcode) == ADDR_MODE_IMMEDIATE;
}

static inline bool is_register(int opcode) {
    return GET_ADDR_MODE(opcode) == ADDR_MODE_REGISTER;
}

static inline bool is_indirect(int opcode) {
    return GET_ADDR_MODE(opcode) == ADDR_MODE_INDIRECT;
}

#define IS_IMMEDIATE(opcode)      is_immediate(opcode)
#define IS_REGISTER(opcode)       is_register(opcode)
#define IS_INDIRECT(opcode)       is_indirect(opcode)

// Комбинирование режима адресации и опкода
static inline tryte_t make_opcode(addr_mode_t mode, int op) {
    // Проверяем, что режим адресации в допустимом диапазоне
    if (mode != ADDR_MODE_IMMEDIATE && mode != ADDR_MODE_REGISTER && mode != ADDR_MODE_INDIRECT) {
        printf("Warning: invalid addressing mode %d\n", mode);
        mode = ADDR_MODE_IMMEDIATE;  // По умолчанию используем непосредственный режим
    }
    
    // Проверяем, что базовый опкод в допустимом диапазоне
    if (op < 0 || op > OP_COUNT - 1) {
        printf("Warning: base opcode %d out of range [0, %d]\n", op, OP_COUNT - 1);
    }
    
    // Создаем трайт с базовым опкодом
    tryte_t result = create_tryte_from_int(op);
    
    // Устанавливаем режим адресации в старший трит
    result.trits[TRITS_PER_TRYTE - 1] = mode;
    update_tryte_value(&result);
    
    return result;
}

#define MAKE_OPCODE(mode, op) make_opcode(mode, op)

//-----------------------------------------------------------------------------
// Описание инструкций
//-----------------------------------------------------------------------------
typedef struct {
    const char* name;             // Мнемоника
    int value;                    // Значение опкода
    int operands;                 // Количество операндов
    const char* desc;            // Описание
    const char* group;           // Группа инструкций
    addr_mode_mask_t addr_mode;  // Допустимые режимы адресации
} instruction_info_t;

// Таблица инструкций
static const instruction_info_t instruction_table[] = {
    // Нет операции
    {"NOP",    OP_NOP,  0, "Нет операции", "Системные", ADDR_MODE_NONE},

    // Стековые операции
    {"PUSH",   OP_PUSH,  1, "Положить значение в стек", "Стек", ADDR_MODE_ALL},
    {"POP",    OP_POP,   1, "Снять значение со стека", "Стек", ADDR_MODE_REG_IND},
    {"DUP",    OP_DUP,   0, "Дублировать верхний элемент стека", "Стек", ADDR_MODE_NONE},
    {"SWAP",   OP_SWAP,  0, "Поменять местами два верхних элемента", "Стек", ADDR_MODE_NONE},
    {"DROP",   OP_DROP,  0, "Удалить верхний элемент", "Стек", ADDR_MODE_NONE},
    {"OVER",   OP_OVER,  0, "Скопировать предпоследний элемент на вершину", "Стек", ADDR_MODE_NONE},

    // Арифметические операции
    {"ADD",    OP_ADD,   0, "Сложить два верхних элемента", "Арифметика", ADDR_MODE_NONE},
    {"SUB",    OP_SUB,   0, "Вычесть верхний элемент из предыдущего", "Арифметика", ADDR_MODE_NONE},
    {"MUL",    OP_MUL,   0, "Умножить два верхних элемента", "Арифметика", ADDR_MODE_NONE},
    {"DIV",    OP_DIV,   0, "Разделить предыдущий элемент на верхний", "Арифметика", ADDR_MODE_NONE},

    // Логические операции
    {"AND",    OP_AND,   0, "Логическое И", "Логика", ADDR_MODE_NONE},
    {"OR",     OP_OR,    0, "Логическое ИЛИ", "Логика", ADDR_MODE_NONE},
    {"NOT",    OP_NOT,   0, "Логическое НЕ", "Логика", ADDR_MODE_NONE},

    // Операции сравнения
    {"EQ",     OP_EQ,    0, "Равно", "Сравнение", ADDR_MODE_NONE},
    {"NEQ",    OP_NEQ,   0, "Не равно", "Сравнение", ADDR_MODE_NONE},
    {"LT",     OP_LT,    0, "Меньше", "Сравнение", ADDR_MODE_NONE},
    {"GT",     OP_GT,    0, "Больше", "Сравнение", ADDR_MODE_NONE},
    {"LE",     OP_LE,    0, "Меньше или равно", "Сравнение", ADDR_MODE_NONE},
    {"GE",     OP_GE,    0, "Больше или равно", "Сравнение", ADDR_MODE_NONE},

    // Операции управления
    {"JMP",    OP_JMP,   1, "Безусловный переход", "Управление", ADDR_MODE_IMM},
    {"JZ",     OP_JZ,    1, "Переход если ноль", "Управление", ADDR_MODE_IMM},
    {"JNZ",    OP_JNZ,   1, "Переход если не ноль", "Управление", ADDR_MODE_IMM},
    {"CALL",   OP_CALL,  1, "Вызов подпрограммы", "Управление", ADDR_MODE_IMM},
    {"RET",    OP_RET,   0, "Возврат из подпрограммы", "Управление", ADDR_MODE_NONE},
    {"HALT",   OP_HALT,  0, "Остановка программы", "Управление", ADDR_MODE_NONE},

    // Операции с памятью
    {"LOAD",   OP_LOAD,  1, "Загрузить значение из памяти в стек", "Память", ADDR_MODE_ALL},
    {"STORE",  OP_STORE, 1, "Сохранить значение из стека в память", "Память", ADDR_MODE_ALL},

    // Системные операции
    {"INT",    OP_INT,   0, "Вызов прерывания", "Системные", ADDR_MODE_NONE},
    {"CLI",    OP_CLI,   0, "Запретить прерывания", "Системные", ADDR_MODE_NONE},
    {"STI",    OP_STI,   0, "Разрешить прерывания", "Системные", ADDR_MODE_NONE},

    // Псевдоинструкции
    {"MOV",    OP_MOV,   2, "Пересылка данных между регистрами", "Псевдоинструкции", ADDR_MODE_REG_IND},
    {"INC",    OP_INC,   1, "Инкремент значения в регистре", "Псевдоинструкции", ADDR_MODE_REG},
    {"DEC",    OP_DEC,   1, "Декремент значения в регистре", "Псевдоинструкции", ADDR_MODE_REG},
    {"PUSHR",  OP_PUSHR, 1, "Положить значение регистра в стек", "Псевдоинструкции", ADDR_MODE_REG},
    {"POPR",   OP_POPR,  1, "Снять значение со стека в регистр", "Псевдоинструкции", ADDR_MODE_REG},
    {"CLEAR",  OP_CLEAR, 1, "Очистить N элементов стека", "Псевдоинструкции", ADDR_MODE_IMM},
    {"CMP",    OP_CMP,   2, "Сравнить два значения", "Псевдоинструкции", ADDR_MODE_ALL},
    {"TEST",   OP_TEST,  1, "Проверить биты значения", "Псевдоинструкции", ADDR_MODE_ALL}
};

//-----------------------------------------------------------------------------
// Типы директив
//-----------------------------------------------------------------------------
typedef enum {
    // Директивы размещения кода и данных
    DIR_ORG = 1,     // Установка адреса
    DIR_SPACE,       // Резервирование памяти
    DIR_ALIGN,       // Выравнивание
    
    // Директивы определения данных
    DIR_TRIT = 10,   // Определение трита
    DIR_TRYTE,       // Определение трайта
    
    // Директивы областей видимости
    DIR_SCOPE = 20,  // Начало области видимости
    DIR_ENDSCOPE,    // Конец области видимости
    DIR_EXPORT,      // Экспорт метки
    DIR_IMPORT,      // Импорт метки
    DIR_LOCAL,       // Локальная метка
    
    DIR_COUNT        // Количество директив
} directive_type_t;

//-----------------------------------------------------------------------------
// Диапазоны директив
//-----------------------------------------------------------------------------
#define DEFINE_DIRECTIVE_RANGES(X) \
    /* Директивы размещения */ \
    X(PLACEMENT,  1,  3) \
    /* Директивы данных */ \
    X(DATA,      10, 11) \
    /* Директивы областей видимости */ \
    X(SCOPE,     20, 24)

// Определяем константы диапазонов
#define DEFINE_RANGE_DIR(name, start, end) \
    enum { \
        name##_DIRECTIVE_START = start, \
        name##_DIRECTIVE_END = end \
    };

DEFINE_DIRECTIVE_RANGES(DEFINE_RANGE_DIR)
#undef DEFINE_RANGE_DIR

// Макрос для проверки принадлежности директивы к диапазону
#define IS_DIRECTIVE_IN_RANGE(dir, start, end) ((dir) >= (start) && (dir) <= (end))

// Макросы для проверки типа директивы
#define DEFINE_IS_DIRECTIVE(name, start, end) \
    static inline bool IS_##name##_DIRECTIVE(int dir) { \
        return IS_DIRECTIVE_IN_RANGE(dir, name##_DIRECTIVE_START, name##_DIRECTIVE_END); \
    }

DEFINE_DIRECTIVE_RANGES(DEFINE_IS_DIRECTIVE)
#undef DEFINE_IS_DIRECTIVE
#undef DEFINE_DIRECTIVE_RANGES

//-----------------------------------------------------------------------------
// Описание директив
//-----------------------------------------------------------------------------
typedef struct {
    const char* name;      // Имя директивы (без точки)
    directive_type_t type; // Тип директивы
    int min_args;         // Минимальное количество аргументов
    int max_args;         // Максимальное количество аргументов (-1 = неограниченно)
    const char* desc;     // Описание
} directive_info_t;

// Таблица директив
static const directive_info_t directive_table[] = {
    // Директивы размещения
    {"org",      DIR_ORG,      1, 1, "Установка текущего адреса"},
    {"space",    DIR_SPACE,    1, 1, "Резервирование памяти"},
    {"align",    DIR_ALIGN,    1, 1, "Выравнивание адреса"},
    
    // Директивы данных
    {"trit",     DIR_TRIT,     1, -1, "Определение трита"},
    {"tryte",    DIR_TRYTE,    1, -1, "Определение трайта"},
    
    // Директивы областей видимости
    {"scope",    DIR_SCOPE,    1, 1, "Начало области видимости"},
    {"endscope", DIR_ENDSCOPE, 0, 0, "Конец области видимости"},
    {"export",   DIR_EXPORT,   1, 1, "Экспорт метки"},
    {"import",   DIR_IMPORT,   1, 1, "Импорт метки"},
    {"local",    DIR_LOCAL,    1, 1, "Объявление локальной метки"}
};

//-----------------------------------------------------------------------------
// Вспомогательные функции для работы с директивами
//-----------------------------------------------------------------------------
// Получение информации о директиве по имени
static inline const directive_info_t* get_directive_info(const char* name) {
    for (size_t i = 0; i < sizeof(directive_table) / sizeof(directive_table[0]); i++) {
        if (strcmp(directive_table[i].name, name) == 0) {
            return &directive_table[i];
        }
    }
    return NULL;
}

// Проверка является ли имя известной директивой
static inline bool is_known_directive(const char* name) {
    return get_directive_info(name) != NULL;
}

//-----------------------------------------------------------------------------
// Вспомогательные функции
//-----------------------------------------------------------------------------
// Получение информации об инструкции по опкоду
static inline const instruction_info_t* get_instruction_info(int opcode) {
    for (size_t i = 0; i < sizeof(instruction_table) / sizeof(instruction_table[0]); i++) {
        if (instruction_table[i].value == opcode) {
            return &instruction_table[i];
        }
    }
    return NULL;
}

#endif // INSTRUCTION_DEFS_H 