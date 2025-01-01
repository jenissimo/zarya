#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "zarya_vm.h"
#include "emulator.h"
#include "zarya_config.h"

// Функция для вывода состояния машины
void print_state(vm_state_t* vm) {
    printf("Состояние машины:\n");
    printf("PC: %d\n", vm->pc.value);
    printf("SP: %d\n", vm->sp.value);
    printf("Флаги: %d\n", vm->flags.value);
    printf("Регистры:\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%d: %d\n", i, vm->registers[i].value);
    }
    printf("\n");
}

// Функция для загрузки программы из файла
vm_error_t load_program(const char* filename, vm_state_t* vm) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка: не удалось открыть файл %s\n", filename);
        return VM_ERROR_INVALID_ADDRESS;
    }

    // Получаем размер файла
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Читаем программу в память
    size_t words_read = fread(vm->memory, sizeof(word_t), file_size / sizeof(word_t), file);
    fclose(file);

    if (words_read * sizeof(word_t) != file_size) {
        fprintf(stderr, "Ошибка: не удалось прочитать файл полностью\n");
        return VM_ERROR_INVALID_ADDRESS;
    }

    return VM_OK;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <файл_программы> [-q]\n", argv[0]);
        fprintf(stderr, "  -q: быстрый режим (показать только начальное и конечное состояние)\n");
        return 1;
    }

    bool quick_mode = false;
    if (argc > 2 && strcmp(argv[2], "-q") == 0) {
        quick_mode = true;
    }

    // Инициализация виртуальной машины
    vm_state_t vm;
    emulator_t emu;
    
    if (vm_init(&vm, MEMORY_SIZE_TRYTES) != VM_OK) {
        fprintf(stderr, "Ошибка: не удалось инициализировать виртуальную машину\n");
        return 1;
    }

    if (emulator_init(&emu, &vm) != VM_OK) {
        fprintf(stderr, "Ошибка: не удалось инициализировать эмулятор\n");
        vm_free(&vm);
        return 1;
    }

    // Загрузка программы
    if (load_program(argv[1], &vm) != VM_OK) {
        fprintf(stderr, "Ошибка: не удалось загрузить программу\n");
        vm_free(&vm);
        return 1;
    }

    printf("Программа загружена успешно\n\n");

    // Показываем начальное состояние
    printf("=== Начальное состояние ===\n");
    print_state(&vm);

    bool is_running = true;
    if (quick_mode) {
        // В быстром режиме выполняем программу до конца
        while (is_running) {
            vm_error_t err = emulator_step(&emu);
            if (err == VM_ERROR_HALT) {
                is_running = false;
            } else if (err != VM_OK) {
                fprintf(stderr, "Ошибка выполнения: %d\n", err);
                break;
            }
        }
        printf("=== Конечное состояние ===\n");
        print_state(&vm);
    } else {
        // Интерактивный режим
        printf("Команды:\n");
        printf("  s - выполнить один шаг\n");
        printf("  r - выполнить до конца\n");
        printf("  q - выход\n\n");

        char cmd;
        while (1) {
            printf("Введите команду (s/r/q): ");
            scanf(" %c", &cmd);

            if (cmd == 'q') {
                break;
            } else if (cmd == 's') {
                if (!is_running) {
                    printf("Программа завершена\n");
                    break;
                }
                vm_error_t err = emulator_step(&emu);
                if (err == VM_ERROR_HALT) {
                    is_running = false;
                    printf("Программа завершена\n");
                } else if (err != VM_OK) {
                    fprintf(stderr, "Ошибка выполнения: %d\n", err);
                    break;
                }
                print_state(&vm);
            } else if (cmd == 'r') {
                while (is_running) {
                    vm_error_t err = emulator_step(&emu);
                    if (err == VM_ERROR_HALT) {
                        is_running = false;
                    } else if (err != VM_OK) {
                        fprintf(stderr, "Ошибка выполнения: %d\n", err);
                        break;
                    }
                }
                printf("=== Конечное состояние ===\n");
                print_state(&vm);
                break;
            }
        }
    }

    vm_free(&vm);
    return 0;
} 