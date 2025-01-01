#ifndef ZARYA_TRIT_OPS_H
#define ZARYA_TRIT_OPS_H

#include "types.h"

// Базовые операции с тритами
trit_t trit_add(trit_t a, trit_t b);
trit_t trit_mul(trit_t a, trit_t b);
trit_t trit_neg(trit_t t);
trit_t trit_and(trit_t a, trit_t b);
trit_t trit_or(trit_t a, trit_t b);

// Операции с трайтами
tryte_t create_tryte_from_int(int value);
int tryte_to_int(const tryte_t* tryte);

// Арифметические операции
tryte_t tryte_add(const tryte_t* a, const tryte_t* b);
tryte_t tryte_sub(const tryte_t* a, const tryte_t* b);
tryte_t tryte_mul(const tryte_t* a, const tryte_t* b);
tryte_t tryte_div(const tryte_t* a, const tryte_t* b);

// Логические операции
tryte_t tryte_and(const tryte_t* a, const tryte_t* b);
tryte_t tryte_or(const tryte_t* a, const tryte_t* b);
tryte_t tryte_not(const tryte_t* a);

// Вспомогательные функции
void update_tryte_value(tryte_t* tryte);

// Операции сдвига
void tryte_shift_left(tryte_t* t);
void tryte_shift_right(tryte_t* t);

// Вывод значений тритов в трайте
void print_tryte(const char* prefix, const tryte_t* tryte);

#endif // ZARYA_TRIT_OPS_H 