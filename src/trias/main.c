#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "errors.h"

// Чтение файла в строку
static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка: не удалось открыть файл '%s'\n", path);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Ошибка: не удалось выделить память\n");
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';
    
    fclose(file);
    return buffer;
}

// Запись бинарного файла
static bool write_binary_file(const char* filename, const uint8_t* data, size_t size) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Ошибка: не удалось открыть файл '%s' для записи\n", filename);
        return false;
    }
    
    // Записываем размер в байтах
    size_t bytes_written = fwrite(data, 1, size, file);
    if (bytes_written != size) {
        fprintf(stderr, "Ошибка: не удалось записать данные в файл '%s'\n", filename);
        fclose(file);
        return false;
    }
    
    fclose(file);
    return true;
}

// Вывод справки
static void print_help(const char* program) {
    printf("Использование: %s [опции] входной_файл\n", program);
    printf("Опции:\n");
    printf("  -h          Показать эту справку\n");
    printf("  -o файл     Указать выходной файл\n");
    printf("  -v          Подробный вывод\n");
}

int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* output_file = "a.out";
    bool verbose = false;
    
    // Разбор аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Ошибка: после -o ожидается имя файла\n");
                return 1;
            }
            output_file = argv[i];
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Ошибка: неизвестная опция '%s'\n", argv[i]);
            return 1;
        } else {
            if (input_file) {
                fprintf(stderr, "Ошибка: указано несколько входных файлов\n");
                return 1;
            }
            input_file = argv[i];
        }
    }
    
    if (!input_file) {
        fprintf(stderr, "Ошибка: не указан входной файл\n");
        return 1;
    }
    
    // Читаем исходный код
    char* source = read_file(input_file);
    if (!source) return 1;
    
    if (verbose) {
        printf("Компиляция %s -> %s\n", input_file, output_file);
    }
    
    // Инициализируем компоненты
    lexer_t lexer;
    parser_t parser;
    codegen_t codegen;
    
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    codegen_init(&codegen);
    
    // Компилируем
    ast_node_t* program = parse_program(&parser);
    if (!program || parser_had_error(&parser)) {
        fprintf(stderr, "Ошибка: не удалось разобрать программу\n");
        free(source);
        return 1;
    }
    
    vm_error_t error = codegen_generate(&codegen, program);
    if (error != VM_OK || codegen_had_error(&codegen)) {
        fprintf(stderr, "Ошибка: не удалось сгенерировать код\n");
        free_ast(program);
        codegen_free(&codegen);
        free(source);
        return 1;
    }
    
    // Получаем сгенерированный код
    size_t code_size;
    const word_t* code = codegen_get_code(&codegen, &code_size);
    
    // Записываем выходной файл
    if (!write_binary_file(output_file, (const uint8_t*)code, code_size * sizeof(word_t))) {
        free_ast(program);
        codegen_free(&codegen);
        free(source);
        return 1;
    }
    
    if (verbose) {
        printf("Готово! Размер кода: %zu байт\n", code_size);
    }
    
    // Освобождаем ресурсы
    free_ast(program);
    codegen_free(&codegen);
    free(source);
    
    return 0;
} 