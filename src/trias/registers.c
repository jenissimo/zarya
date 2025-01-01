#include "registers.h"
#include <string.h>

// Таблица регистров
static const register_info_t registers[] = {
    {"R0", 0},
    {"R1", 1},
    {"R2", 2},
    {"R3", 3},
    {"R4", 4},
    {"R5", 5},
    {"R6", 6},
    {"R7", 7},
    {NULL, 0}  // Маркер конца таблицы
};

// Поиск регистра по имени
const register_info_t* find_register(const char* name) {
    for (const register_info_t* reg = registers; reg->name != NULL; reg++) {
        if (strcmp(reg->name, name) == 0) {
            return reg;
        }
    }
    return NULL;
} 