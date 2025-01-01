#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "trit_ops.h"

// Преобразование трита в индекс для таблиц
int trit_to_index(trit_t t) {
    return t + 1;  // -1 -> 0, 0 -> 1, 1 -> 2
}

// Отрицание трита
trit_t trit_neg(trit_t t) {
    if (!is_valid_trit(t)) return 0;
    return -t;
}

// Логические операции с тритами
trit_t trit_and(trit_t a, trit_t b) {
    if (!is_valid_trit(a) || !is_valid_trit(b)) return 0;
    
    // Таблица истинности для AND:
    //   -1  0  1
    // -1 -1 0 -1
    //  0  0  0  0
    //  1 -1 0  1
    
    if (a == 0 || b == 0) return 0;
    if (a == b) return a;
    return -1;
}

trit_t trit_or(trit_t a, trit_t b) {
    if (!is_valid_trit(a) || !is_valid_trit(b)) return 0;
    
    // Таблица истинности для OR:
    //   -1  0  1
    // -1 -1 -1 1
    //  0 -1  0 1
    //  1  1  1 1
    
    if (a == 1 || b == 1) return 1;
    if (a == 0) return b;
    if (b == 0) return a;
    return -1;
}

// Обновление значения трайта из его тритов
void update_tryte_value(tryte_t* tryte) {
    if (!tryte) return;
    
    int value = 0;
    int power = 1;
    
    // Итерируем от младшего трита к старшему
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        value += tryte->trits[i] * power;
        power *= 3;
    }
    
    tryte->value = value;
}

void update_word_value(word_t* word) {
    if (!word) return;
    
    int value = 0;
    int power = 1;
    for (int i = 0; i < TRITS_PER_WORD; i++) {
        value += word->trits[i] * power;
        power *= 3;
    }
    word->value = value;
}

// Операции с трайтами
tryte_t create_tryte_from_int(int value) {
    tryte_t result = {0};
    
    // Преобразуем десятичное число в троичное сбалансированное
    int temp = value;
    bool is_negative = temp < 0;
    if (is_negative) {
        temp = -temp;  // Работаем с положительным числом
    }
    
    // Заполняем триты от младшего к старшему
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        int rem = temp % 3;
        temp /= 3;
        
        // Корректируем остаток для сбалансированной системы
        if (rem == 2) {
            rem = -1;
            temp++;
        }
        
        // Для отрицательных чисел инвертируем значения тритов
        result.trits[i] = (trit_t)(is_negative ? -rem : rem);
    }
    
    update_tryte_value(&result);
    return result;
}

int tryte_to_int(const tryte_t* tryte) {
    int result = 0;
    int power = 1;
    
    // Итерируем от младшего трита к старшему
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        result += tryte->trits[i] * power;
        power *= 3;
    }
    
    return result;
}

tryte_t tryte_add(const tryte_t* a, const tryte_t* b) {
    tryte_t result = {0};
    int carry = 0;
    
    // Складываем от младшего трита к старшему
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        // Складываем триты и перенос
        int sum = a->trits[i] + b->trits[i] + carry;
        
        // Обрабатываем перенос в сбалансированной троичной системе
        if (sum > 1) {
            result.trits[i] = sum - 3;
            carry = 1;
        } else if (sum < -1) {
            result.trits[i] = sum + 3;
            carry = -1;
        } else {
            result.trits[i] = sum;
            carry = 0;
        }
    }
    
    // Обновляем значение результата
    result.value = a->value + b->value;
    return result;
}

tryte_t tryte_sub(const tryte_t* a, const tryte_t* b) {
    // Вычитание - это сложение с отрицательным числом
    tryte_t neg_b = *b;
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        neg_b.trits[i] = trit_neg(b->trits[i]);
    }
    update_tryte_value(&neg_b);
    
    return tryte_add(a, &neg_b);
}

tryte_t tryte_mul(const tryte_t* a, const tryte_t* b) {
    tryte_t result = {0};
    int temp[TRITS_PER_TRYTE * 2] = {0};  // Временный массив для накопления результатов
    
    // Умножаем каждый трит первого числа на каждый трит второго
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        for (int j = 0; j < TRITS_PER_TRYTE; j++) {
            temp[i + j] += a->trits[i] * b->trits[j];
        }
    }
    
    // Обрабатываем переносы
    for (int i = 0; i < TRITS_PER_TRYTE * 2 - 1; i++) {
        while (temp[i] > 1) {
            temp[i] -= 3;
            temp[i + 1] += 1;
        }
        while (temp[i] < -1) {
            temp[i] += 3;
            temp[i + 1] -= 1;
        }
    }
    
    // Обрабатываем старший разряд
    while (temp[TRITS_PER_TRYTE * 2 - 1] > 1) {
        temp[TRITS_PER_TRYTE * 2 - 1] -= 3;
    }
    while (temp[TRITS_PER_TRYTE * 2 - 1] < -1) {
        temp[TRITS_PER_TRYTE * 2 - 1] += 3;
    }
    
    // Копируем результат
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        result.trits[i] = temp[i];
    }
    
    update_tryte_value(&result);
    return result;
}

tryte_t tryte_div(const tryte_t* a, const tryte_t* b) {
    tryte_t result = {0};
    
    // Проверка деления на ноль
    if (b->value == 0) {
        return result; // Возвращаем 0 при делении на ноль
    }
    
    // Простое целочисленное деление
    result = create_tryte_from_int(a->value / b->value);
    
    return result;
}

// Логические операции с трайтами
tryte_t tryte_and(const tryte_t* a, const tryte_t* b) {
    tryte_t result = {0};
    
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        result.trits[i] = trit_and(a->trits[i], b->trits[i]);
    }
    
    update_tryte_value(&result);
    return result;
}

tryte_t tryte_or(const tryte_t* a, const tryte_t* b) {
    tryte_t result = {0};
    
    // Применяем операцию OR к каждому триту
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        result.trits[i] = trit_or(a->trits[i], b->trits[i]);
    }
    
    update_tryte_value(&result);
    return result;
}

tryte_t tryte_not(const tryte_t* a) {
    tryte_t result = {0};
    
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        result.trits[i] = trit_neg(a->trits[i]);
    }
    
    update_tryte_value(&result);
    return result;
}

word_t create_word_from_int(int value) {
    word_t result = {0};
    
    // Приводим значение к диапазону машинного слова
    value = ((value % 0x10000) + 0x10000) % 0x10000;
    result.value = value;
    
    // Заполняем триты
    for (int i = 0; i < TRITS_PER_WORD; i++) {
        int remainder = value % 3;
        value /= 3;
        
        switch (remainder) {
            case 0:
                result.trits[i] = TRIT_ZERO;
                break;
            case 1:
                result.trits[i] = TRIT_POSITIVE;
                break;
            case 2:
                result.trits[i] = TRIT_NEGATIVE;
                value++;
                break;
        }
    }
    
    return result;
}

// Сдвиг тритов влево
void tryte_shift_left(tryte_t* tryte) {
    if (!tryte) return;
    
    // Сдвигаем все триты влево (от младшего к старшему)
    for (int i = TRITS_PER_TRYTE - 1; i > 0; i--) {
        tryte->trits[i] = tryte->trits[i - 1];
    }
    tryte->trits[0] = 0;  // Младший трит заполняем нулем
    
    // Обновляем значение
    update_tryte_value(tryte);
}

// Сдвиг тритов вправо
void tryte_shift_right(tryte_t* tryte) {
    if (!tryte) return;
    
    // Сдвигаем все триты вправо (от старшего к младшему)
    for (int i = 0; i < TRITS_PER_TRYTE - 1; i++) {
        tryte->trits[i] = tryte->trits[i + 1];
    }
    tryte->trits[TRITS_PER_TRYTE - 1] = 0;  // Старший трит заполняем нулем
    
    // Обновляем значение
    update_tryte_value(tryte);
}

// Вывод значений тритов в трайте
void print_tryte(const char* prefix, const tryte_t* tryte) {
    if (!prefix || !tryte) return;
    
    printf("%s: value=%d, trits=[", prefix, tryte->value);
    for (int i = 0; i < TRITS_PER_TRYTE; i++) {
        printf("%d", tryte->trits[i]);
        if (i < TRITS_PER_TRYTE - 1) {
            printf(",");
        }
    }
    printf("]\n");
} 