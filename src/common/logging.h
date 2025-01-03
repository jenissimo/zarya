#ifndef COMMON_LOGGING_H
#define COMMON_LOGGING_H

#include <stdio.h>
#include <stdbool.h>

// Уровни логирования
typedef enum {
    LOG_NONE = 0,      // Логирование отключено
    LOG_ERROR = 1,     // Критические ошибки
    LOG_WARN = 2,      // Предупреждения
    LOG_INFO = 3,      // Информационные сообщения
    LOG_DEBUG = 4,     // Отладочная информация
    LOG_TRACE = 5      // Детальная трассировка
} log_level_t;

// Подсистемы для логирования
typedef enum {
    LOG_CORE = 1 << 0,     // Ядро системы
    LOG_MEMORY = 1 << 1,   // Работа с памятью
    LOG_LEXER = 1 << 2,    // Лексический анализатор
    LOG_PARSER = 1 << 3,   // Синтаксический анализатор
    LOG_CODEGEN = 1 << 4,  // Генератор кода
    LOG_VM = 1 << 5,       // Виртуальная машина
    LOG_AST = 1 << 6,      // Абстрактное синтаксическое дерево
    LOG_TRITS = 1 << 7,    // Работа с тритами
    LOG_TRIAS = 1 << 8,    // Ассемблер ТРИАС
    LOG_ALL = (1 << 9) - 1 // Все подсистемы (9 бит достаточно)
} log_subsystem_t;

// Цвета для разных уровней логирования
#define LOG_COLOR_ERROR "\033[1;31m"  // Ярко-красный
#define LOG_COLOR_WARN  "\033[1;33m"  // Ярко-желтый
#define LOG_COLOR_INFO  "\033[1;32m"  // Ярко-зеленый
#define LOG_COLOR_DEBUG "\033[1;36m"  // Ярко-голубой
#define LOG_COLOR_TRACE "\033[1;35m"  // Ярко-фиолетовый
#define LOG_COLOR_RESET "\033[0m"     // Сброс цвета

// Инициализация системы логирования
void log_init(FILE* output, log_level_t level, log_subsystem_t subsystems, bool use_colors);

// Установка уровня логирования
void log_set_level(log_level_t level);

// Установка активных подсистем
void log_set_subsystems(log_subsystem_t subsystems);

// Включение/выключение цветного вывода
void log_set_colors(bool use_colors);

// Проверка, активно ли логирование для данного уровня и подсистемы
bool log_is_enabled(log_level_t level, log_subsystem_t subsystem);

// Функция логирования
void log_message(log_level_t level, log_subsystem_t subsystem, 
                const char* file, int line, const char* func,
                const char* format, ...);

// Макросы для удобного логирования
#define LOG_ERROR(subsys, ...) \
    log_message(LOG_ERROR, subsys, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_WARN(subsys, ...) \
    log_message(LOG_WARN, subsys, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_INFO(subsys, ...) \
    log_message(LOG_INFO, subsys, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_DEBUG(subsys, ...) \
    log_message(LOG_DEBUG, subsys, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_TRACE(subsys, ...) \
    log_message(LOG_TRACE, subsys, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Получение строкового представления уровня логирования
const char* log_level_to_string(log_level_t level);

// Получение строкового представления подсистемы
const char* log_subsystem_to_string(log_subsystem_t subsystem);

#endif // COMMON_LOGGING_H 