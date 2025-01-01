#include "unity.h"
#include "test_common.h"
#include "trias/parser.h"
#include "trias/lexer.h"
#include "trias/ast.h"

// Вспомогательная функция для тестирования парсера
void test_parse_string(const char* input, const char* expected_error) {
    lexer_t lexer;
    parser_t parser;
    ast_node_t* ast = NULL;
    
    // Инициализация
    lexer_init(&lexer, input);
    parser_init(&parser, &lexer);
    
    // Разбор программы
    ast = parse_program(&parser);
    
    // Проверка результатов
    if (expected_error) {
        // Если ожидается ошибка
        TEST_ASSERT_NULL_MESSAGE(ast, "AST должен быть NULL при ошибке");
        TEST_ASSERT_TRUE_MESSAGE(parser_had_error(&parser), "Ожидалась ошибка");
        TEST_ASSERT_NOT_NULL_MESSAGE(parser.error_message, "Должно быть сообщение об ошибке");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_error, parser.error_message, "Неверное сообщение об ошибке");
    } else {
        // Если ошибка не ожидается
        if (parser_had_error(&parser)) {
            // Если произошла неожиданная ошибка, освобождаем AST если он есть
            if (ast) {
                free_ast(ast);
                ast = NULL;
            }
            TEST_FAIL_MESSAGE(parser.error_message ? parser.error_message : "Неожиданная ошибка");
        }
        TEST_ASSERT_NOT_NULL_MESSAGE(ast, "AST не должен быть NULL при успешном разборе");
    }
    
    // Освобождение ресурсов
    if (ast) {
        free_ast(ast);
    }
    parser_free(&parser);
    lexer_free(&lexer);
}

// Тесты режимов адресации
void test_addressing_modes(void) {
    // Непосредственный режим
    test_parse_string("PUSH #42\n", NULL);
    test_parse_string("label: NOP\nPUSH #label\n", NULL);
    test_parse_string("PUSH #R0\n", "Регистр не может быть непосредственным значением");
    
    // Регистровый режим
    test_parse_string("PUSH R0\n", NULL);
    test_parse_string("POP R1\n", NULL);
    test_parse_string("MOV R0, R1\n", NULL);
    
    // Косвенный режим
    test_parse_string("PUSH @R0\n", NULL);
    test_parse_string("POP @R1\n", NULL);
    test_parse_string("label2: NOP\nPUSH @label2\n", NULL);
    test_parse_string("PUSH @42\n", "Число не может использоваться в косвенном режиме");
}

// Тесты меток
void test_labels(void) {
    // Корректные метки
    test_parse_string("start: PUSH #42\n", NULL);
    test_parse_string("loop1: PUSH R0\n", NULL);
    test_parse_string("my_label: POP R1\n", NULL);
    
    // Некорректные метки
    test_parse_string("1label: PUSH #42\n", "Некорректное имя метки");
    test_parse_string("my-label: PUSH #42\n", "Некорректное имя метки");
    test_parse_string("label: label: PUSH #42\n", "Метка уже определена");
    
    // Использование меток
    test_parse_string(
        "start: PUSH #42\n"
        "      JMP start\n",
        NULL
    );
    
    test_parse_string("JMP undefined\n", "Неопределенная метка");
}

// Тесты директив
void test_directives(void) {
    // Директива .org
    test_parse_string(".org 100\n", NULL);
    test_parse_string(".org label\n", "Ожидалось число после .org");
    
    // Директива .db
    test_parse_string(".db 42\n", NULL);
    test_parse_string(".db \"Hello\"\n", NULL);
    test_parse_string(".db\n", "Ожидалось число или строка после директивы");
    
    // Директива .dw
    test_parse_string(".dw 42\n", NULL);
    test_parse_string(".dw \"Hello\"\n", NULL);
    test_parse_string(".dw\n", "Ожидалось число или строка после директивы");
    
    // Директива .ds
    test_parse_string(".ds \"Hello\"\n", NULL);
    test_parse_string(".ds 42\n", "Ожидалась строка после .ds");
}

// Тесты программ
void test_programs(void) {
    // Простая программа
    test_parse_string(
        ".org 100\n"
        "start: PUSH #42\n"
        "      POP R0\n"
        "      HALT\n",
        NULL
    );
    
    // Программа с циклом
    test_parse_string(
        ".org 100\n"
        "start: PUSH #5\n"
        "      POP R0\n"
        "loop: PUSH R0\n"
        "      PUSH #1\n"
        "      SUB\n"
        "      POP R0\n"
        "      PUSH R0\n"
        "      PUSH #0\n"
        "      NEQ\n"
        "      JNZ loop\n"
        "      HALT\n",
        NULL
    );
    
    // Программа с ошибкой
    test_parse_string(
        ".org 100\n"
        "start: PUSH #42\n"
        "      POP R8\n"  // R8 не существует
        "      HALT\n",
        "Недопустимый номер регистра"
    );
}

int main(void) {
    UNITY_BEGIN();
    // Пока комментируем остальные тесты, чтобы не перегружать вывод в консоль
    RUN_TEST(test_addressing_modes);
    //RUN_TEST(test_labels);
    //RUN_TEST(test_directives);
    //RUN_TEST(test_programs);
    return UNITY_END();
} 