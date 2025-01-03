#include "logging.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>

// Глобальное состояние логирования
static struct {
    FILE* output;              // Файл для вывода
    log_level_t level;         // Текущий уровень логирования
    log_subsystem_t subsystems; // Активные подсистемы
    bool use_colors;           // Использовать ли цветной вывод
} log_state = {
    .output = NULL,
    .level = LOG_NONE,
    .subsystems = 0,
    .use_colors = false
};

// Строковые представления уровней логирования
static const char* level_strings[] = {
    "NONE",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE"
};

// Цвета для разных уровней
static const char* level_colors[] = {
    "",                 // NONE
    LOG_COLOR_ERROR,    // ERROR
    LOG_COLOR_WARN,     // WARN
    LOG_COLOR_INFO,     // INFO
    LOG_COLOR_DEBUG,    // DEBUG
    LOG_COLOR_TRACE     // TRACE
};

// Строковые представления подсистем
static const struct {
    log_subsystem_t subsystem;
    const char* name;
} subsystem_strings[] = {
    {LOG_CORE, "CORE"},
    {LOG_MEMORY, "MEMORY"},
    {LOG_LEXER, "LEXER"},
    {LOG_PARSER, "PARSER"},
    {LOG_CODEGEN, "CODEGEN"},
    {LOG_VM, "VM"},
    {LOG_AST, "AST"},
    {LOG_TRITS, "TRITS"},
    {0, NULL}
};

void log_init(FILE* output, log_level_t level, log_subsystem_t subsystems, bool use_colors) {
    log_state.output = output ? output : stderr;
    log_state.level = level;
    log_state.subsystems = subsystems;
    log_state.use_colors = use_colors;
}

void log_set_level(log_level_t level) {
    log_state.level = level;
}

void log_set_subsystems(log_subsystem_t subsystems) {
    log_state.subsystems = subsystems;
}

void log_set_colors(bool use_colors) {
    log_state.use_colors = use_colors;
}

bool log_is_enabled(log_level_t level, log_subsystem_t subsystem) {
    return level <= log_state.level && (log_state.subsystems & subsystem);
}

const char* log_level_to_string(log_level_t level) {
    if (level >= 0 && level < sizeof(level_strings) / sizeof(level_strings[0])) {
        return level_strings[level];
    }
    return "UNKNOWN";
}

const char* log_subsystem_to_string(log_subsystem_t subsystem) {
    for (int i = 0; subsystem_strings[i].name; i++) {
        if (subsystem_strings[i].subsystem == subsystem) {
            return subsystem_strings[i].name;
        }
    }
    return "UNKNOWN";
}

// Получение текущего времени в формате [ЧЧ:ММ:СС.мс]
static void get_time_string(char* buffer, size_t size) {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    
    // Получаем миллисекунды
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int ms = ts.tv_nsec / 1000000;
    
    snprintf(buffer, size, "[%02d:%02d:%02d.%03d]",
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms);
}

void log_message(log_level_t level, log_subsystem_t subsystem,
                const char* file, int line, const char* func,
                const char* format, ...) {
    if (!log_is_enabled(level, subsystem)) return;
    
    // Получаем время
    char time_buffer[32];
    get_time_string(time_buffer, sizeof(time_buffer));
    
    // Начало строки с цветом
    if (log_state.use_colors) {
        fprintf(log_state.output, "%s", level_colors[level]);
    }
    
    // Выводим заголовок
    fprintf(log_state.output, "%s %-5s %-7s %s:%d %s(): ",
            time_buffer,
            log_level_to_string(level),
            log_subsystem_to_string(subsystem),
            file, line, func);
    
    // Выводим сообщение
    va_list args;
    va_start(args, format);
    vfprintf(log_state.output, format, args);
    va_end(args);
    
    // Сброс цвета и перевод строки
    if (log_state.use_colors) {
        fprintf(log_state.output, LOG_COLOR_RESET);
    }
    fprintf(log_state.output, "\n");
    
    // Сразу сбрасываем буфер
    fflush(log_state.output);
} 