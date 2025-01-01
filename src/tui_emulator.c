#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#define kbhit _kbhit
#define getch _getch
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#define sleep_ms(ms) usleep((ms) * 1000)

// Неблокирующее чтение символа
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    
    return 0;
}

// Чтение символа без эха
int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

#include "emulator.h"
#include "zarya_config.h"

#define WINDOW_WIDTH 80
#define WINDOW_HEIGHT 25
#define REGISTERS_WINDOW_HEIGHT 8
#define CODE_WINDOW_HEIGHT 8
#define STACK_WINDOW_HEIGHT 8
#define MEMORY_WINDOW_HEIGHT 8

// Индексы специальных регистров
#define REG_FLAGS 15 // Flags

typedef struct {
    vm_state_t vm;
    emulator_t emu;
    bool running;
    bool step_mode;
    int selected_reg;
    int selected_mem;
    
    // Кэш для предыдущего состояния
    struct {
        int pc;
        int sp;
        int flags;
        int registers[8];
        int selected_reg;
        int selected_mem;
        bool running;
        bool step_mode;
    } prev_state;
} tui_state_t;

// Очистка экрана
void clear_screen() {
    printf("\x1b[2J\x1b[H");
}

// Установка позиции курсора
void set_cursor(int x, int y) {
    printf("\x1b[%d;%dH", y + 1, x + 1);
}

// Отрисовка рамки с заголовком
void draw_box(int x, int y, int width, int height, const char* title) {
    set_cursor(x, y);
    printf("┌─%s", title);
    for (int i = strlen(title); i < width - 2; i++) printf("─");
    printf("┐");
    
    for (int i = 1; i < height - 1; i++) {
        set_cursor(x, y + i);
        printf("│");
        set_cursor(x + width - 1, y + i);
        printf("│");
    }
    
    set_cursor(x, y + height - 1);
    printf("└");
    for (int i = 1; i < width - 1; i++) printf("─");
    printf("┘");
}

// Отображение регистров
void draw_registers(tui_state_t* state) {
    bool need_redraw = false;
    
    // Проверяем изменения
    for (int i = 0; i < 8; i++) {
        if (state->vm.registers[i].value != state->prev_state.registers[i] ||
            state->selected_reg != state->prev_state.selected_reg) {
            need_redraw = true;
            break;
        }
    }
    
    if (state->vm.sp.value != state->prev_state.sp ||
        state->vm.pc.value != state->prev_state.pc ||
        state->vm.flags.value != state->prev_state.flags) {
        need_redraw = true;
    }
    
    if (!need_redraw) return;
    
    // Отрисовываем только если есть изменения
    draw_box(0, 0, 30, REGISTERS_WINDOW_HEIGHT, " Регистры ");
    
    // Регистры общего назначения
    for (int i = 0; i < 8; i++) {
        set_cursor(2, 1 + i);
        printf("R%d: %04X%s", i, state->vm.registers[i].value,
               i == state->selected_reg ? " ←" : "  ");
        state->prev_state.registers[i] = state->vm.registers[i].value;
    }
    
    // Специальные регистры
    set_cursor(2, 9);
    printf("SP: %04X", state->vm.sp.value);
    set_cursor(2, 10);
    printf("PC: %04X", state->vm.pc.value);
    set_cursor(2, 11);
    printf("FL: %04X", state->vm.flags.value);
    
    state->prev_state.sp = state->vm.sp.value;
    state->prev_state.pc = state->vm.pc.value;
    state->prev_state.flags = state->vm.flags.value;
    state->prev_state.selected_reg = state->selected_reg;
}

// Отображение кода
void draw_code(tui_state_t* state) {
    if (state->vm.pc.value == state->prev_state.pc) return;
    
    draw_box(30, 0, 50, CODE_WINDOW_HEIGHT, " Код ");
    
    size_t pc = (size_t)state->vm.pc.value;
    size_t start = pc >= 2 ? pc - 2 : 0;
    size_t lines = CODE_WINDOW_HEIGHT - 3;
    size_t end = start + lines;
    
    if (end >= state->vm.memory_size) {
        end = state->vm.memory_size - 1;
        if (end >= lines && start > end - lines) {
            start = end - lines;
        }
    }
    
    for (size_t i = start; i <= end; i++) {
        set_cursor(32, 1 + (int)(i - start));
        printf("%04zX: %04X%s", i, state->vm.memory[i].value,
               i == pc ? " ←" : "  ");
    }
    
    state->prev_state.pc = state->vm.pc.value;
}

// Отображение стека
void draw_stack(tui_state_t* state) {
    if (state->vm.sp.value == state->prev_state.sp) return;
    
    draw_box(0, REGISTERS_WINDOW_HEIGHT, 30, STACK_WINDOW_HEIGHT, " Стек ");
    
    size_t sp = (size_t)state->vm.sp.value;
    size_t start = sp >= 2 ? sp - 2 : 0;
    size_t lines = STACK_WINDOW_HEIGHT - 3;
    size_t end = start + lines;
    
    if (end >= state->vm.memory_size) {
        end = state->vm.memory_size - 1;
        if (end >= lines && start > end - lines) {
            start = end - lines;
        }
    }
    
    for (size_t i = start; i <= end; i++) {
        set_cursor(2, REGISTERS_WINDOW_HEIGHT + 1 + (int)(i - start));
        printf("%04zX: %04X%s", i, state->vm.memory[i].value,
               i == sp ? " ←" : "  ");
    }
    
    state->prev_state.sp = state->vm.sp.value;
}

// Отображение памяти
void draw_memory(tui_state_t* state) {
    if (state->selected_mem == state->prev_state.selected_mem) return;
    
    draw_box(30, CODE_WINDOW_HEIGHT, 50, MEMORY_WINDOW_HEIGHT, " Память ");
    
    size_t mem = (size_t)state->selected_mem;
    size_t start = mem >= 2 ? mem - 2 : 0;
    size_t lines = MEMORY_WINDOW_HEIGHT - 3;
    size_t end = start + lines;
    
    if (end >= state->vm.memory_size) {
        end = state->vm.memory_size - 1;
        if (end >= lines && start > end - lines) {
            start = end - lines;
        }
    }
    
    for (size_t i = start; i <= end; i++) {
        set_cursor(32, CODE_WINDOW_HEIGHT + 1 + (int)(i - start));
        printf("%04zX: %04X%s", i, state->vm.memory[i].value,
               i == mem ? " ←" : "  ");
    }
    
    state->prev_state.selected_mem = state->selected_mem;
}

// Отображение статуса
void draw_status(tui_state_t* state) {
    if (state->running == state->prev_state.running &&
        state->step_mode == state->prev_state.step_mode) return;
        
    set_cursor(0, WINDOW_HEIGHT - 1);
    printf("\x1b[K"); // Очищаем строку
    printf("Статус: %s | Режим: %s | F1-Помощь F5-Старт/Стоп F6-Шаг ↑↓-Выбор",
           state->running ? "Работает" : "Остановлен",
           state->step_mode ? "Пошаговый" : "Обычный");
           
    state->prev_state.running = state->running;
    state->prev_state.step_mode = state->step_mode;
}

// Обновление всего экрана
void update_display(tui_state_t* state) {
    draw_registers(state);
    draw_code(state);
    draw_stack(state);
    draw_memory(state);
    draw_status(state);
    fflush(stdout);
}

// Прототипы функций
void handle_key(tui_state_t* state, int key);
void update_display(tui_state_t* state);

// Выполнение одной инструкции в TUI
bool tui_step(tui_state_t* state) {
    vm_error_t error = emulator_step(&state->emu);
    if (error != VM_OK) {
        state->running = false;
        set_cursor(0, WINDOW_HEIGHT - 2);
        printf("Ошибка выполнения: %d", error);
        return false;
    }
    return true;
}

// Основной цикл эмуляции
void emulation_loop(tui_state_t* state) {
    while (true) {
        update_display(state);
        
        if (state->running && !state->step_mode) {
            if (!tui_step(state)) {
                break;
            }
            sleep_ms(100); // 100ms
        }
        
        if (kbhit()) {
            int key = getch();
            handle_key(state, key);
            if (key == 'q' || key == 'Q') break;
        }
    }
}

// Обработка клавиш
void handle_key(tui_state_t* state, int key) {
    switch (key) {
        case 'w': // Вверх
        case 'W':
            if (state->selected_reg > 0) state->selected_reg--;
            break;
            
        case 's': // Вниз
        case 'S':
            if (state->selected_reg < NUM_REGISTERS - 1) state->selected_reg++;
            break;
            
        case 0x3B: // F1 (помощь)
            // TODO: показать помощь
            break;
            
        case 0x3F: // F5 (старт/стоп)
            state->running = !state->running;
            break;
            
        case 0x40: // F6 (шаг)
            if (!state->running) {
                tui_step(state);
            }
            break;
            
        case 'q': // Выход
        case 'Q':
            state->running = false;
            break;
    }
}

// Загрузка программы из файла
bool load_program(tui_state_t* state, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка: не удалось открыть файл %s\n", filename);
        return false;
    }
    
    // Получаем размер файла
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0) {
        fprintf(stderr, "Ошибка: файл пуст\n");
        fclose(file);
        return false;
    }
    
    if ((size_t)size > state->vm.memory_size * sizeof(tryte_t)) {
        fprintf(stderr, "Ошибка: программа слишком большая\n");
        fclose(file);
        return false;
    }
    
    // Читаем программу в память
    size_t words = (size_t)size / sizeof(tryte_t);
    size_t read = fread(state->vm.memory, sizeof(tryte_t), words, file);
    fclose(file);
    
    if (read != words) {
        fprintf(stderr, "Ошибка: не удалось прочитать файл полностью (прочитано %zu из %zu слов)\n", 
                read, words);
        return false;
    }
    
    return true;
}

// Инициализация терминала
void init_terminal(void) {
#ifdef _WIN32
    // Включаем поддержку ANSI escape-последовательностей в Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    // Устанавливаем кодировку UTF-8
    SetConsoleOutputCP(CP_UTF8);
#else
    // В Unix-подобных системах включаем raw mode
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    
    // Очищаем экран и прячем курсор
    printf("\x1b[2J\x1b[?25l");
#endif
}

// Восстановление терминала
void restore_terminal(void) {
#ifdef _WIN32
    // Восстанавливаем кодировку по умолчанию
    SetConsoleOutputCP(GetACP());
#else
    // Восстанавливаем canonical mode и показываем курсор
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    printf("\x1b[?25h");
#endif
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <программа>\n", argv[0]);
        return 1;
    }
    
    printf("Инициализация эмулятора...\n");
    
    // Инициализируем терминал
    init_terminal();
    
    tui_state_t state = {0};
    vm_init(&state.vm, 65536); // 64K памяти
    
    // Инициализируем эмулятор
    if (emulator_init(&state.emu, &state.vm) != VM_OK) {
        fprintf(stderr, "Ошибка: не удалось инициализировать эмулятор\n");
        vm_free(&state.vm);
        restore_terminal();
        return 1;
    }
    
    printf("Загрузка программы %s...\n", argv[1]);
    
    if (!load_program(&state, argv[1])) {
        emulator_free(&state.emu);
        vm_free(&state.vm);
        restore_terminal();
        return 1;
    }
    
    printf("Программа загружена успешно\n");
    
    // Инициализация начального состояния
    state.running = false;
    state.step_mode = false;
    state.selected_reg = 0;
    state.selected_mem = 0;
    
    printf("Запуск эмуляции...\n");
    
    // Запуск эмуляции
    emulation_loop(&state);
    
    // Восстанавливаем терминал
    restore_terminal();
    
    // Освобождаем память
    emulator_free(&state.emu);
    vm_free(&state.vm);
    
    return 0;
} 