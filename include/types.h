#ifndef ZARYA_TYPES_H
#define ZARYA_TYPES_H

#include <stdint.h>
#include "zarya_config.h"

// Базовый тип для трита
typedef int8_t trit_t;

// Структура для трайта
typedef struct {
    trit_t trits[TRITS_PER_TRYTE];
    int32_t value;  // Целочисленный эквивалент для быстрых операций
} tryte_t;

// Структура для машинного слова
typedef struct {
    trit_t trits[TRITS_PER_WORD];
    int64_t value;  // Целочисленный эквивалент для быстрых операций
} word_t;

// Макросы для автоматического обновления значений
#define TRYTE_SET_VALUE(t, v) ((t).value = (v))
#define TRYTE_GET_VALUE(t) ((t).value)
#define TRYTE_FROM_INT(v) create_tryte_from_int(v)

#define TRYTE_INC(t) TRYTE_SET_VALUE(t, (t).value + 1)
#define TRYTE_DEC(t) TRYTE_SET_VALUE(t, (t).value - 1)

#define WORD_SET_VALUE(w, v) do { \
    (w).value = (v); \
    update_word_value(&(w)); \
} while(0)

// Функции для проверки корректности значений
static inline int is_valid_trit(trit_t t) {
    return (t >= TRIT_NEGATIVE && t <= TRIT_POSITIVE);
}

// Прототипы функций для создания типов
tryte_t create_tryte_from_int(int value);
word_t create_word_from_int(int value);

#endif // ZARYA_TYPES_H 