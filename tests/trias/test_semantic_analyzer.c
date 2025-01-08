#include "unity.h"
#include <stdlib.h>  // для malloc, free
#include <string.h>  // для strcpy, strcat
#include "trias/semantic_analyzer.h"
#include "trias/parser.h"
#include "trias/lexer.h"
#include "trias/ast.h"
#include "zarya_vm.h"
#include "zarya_config.h"
#include "logging.h"

static vm_state_t vm;
static ast_manager_t* ast_manager;
static symbol_table_t* symbol_table;
static semantic_analyzer_t* analyzer;

// Инициализация перед каждым тестом
void setUp(void) {
    // Инициализация логирования
    log_init(stdout, LOG_DEBUG, LOG_SEMANTIC | LOG_TRIAS | LOG_LEXER | LOG_PARSER, true);
    
    vm_init(&vm, MEMORY_SIZE_TRYTES);
    vm_reset(&vm);
    ast_manager = ast_manager_create();
    TEST_ASSERT_NOT_NULL(ast_manager);
    symbol_table = symbol_table_create();
    TEST_ASSERT_NOT_NULL(symbol_table);
    symbol_table_clear(symbol_table); // Очищаем таблицу символов
    analyzer = semantic_analyzer_create();
    TEST_ASSERT_NOT_NULL(analyzer);
    semantic_analyzer_set_symbol_table(analyzer, symbol_table);
}

// Очистка после каждого теста
void tearDown(void) {
    semantic_analyzer_destroy(analyzer);
    symbol_table_destroy(symbol_table);
    ast_manager_destroy(ast_manager);
    vm_free(&vm);
}

// Вспомогательная функция для разбора программы
static ast_program_t* parse_program(const char* input) {
    lexer_t* lexer = lexer_create_from_string(input);
    TEST_ASSERT_NOT_NULL(lexer);
    
    parser_t* parser = parser_create(lexer);
    TEST_ASSERT_NOT_NULL(parser);
    
    parser_set_ast_manager(parser, ast_manager);
    parser_set_symbol_table(parser, symbol_table);
    
    ast_program_t* program = parser_parse_program(parser);
    
    parser_destroy(parser);
    return program;
}

// Тесты директив размещения
void test_org_directive(void) {
    printf("\n=== Тест директивы .org ===\n");
    
    // Корректная директива
    printf("\n[ТЕСТ 1] Корректная директива .org\n");
    ast_program_t* program = parse_program(".org 100");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Отрицательный адрес
    printf("\n[ТЕСТ 2] Ошибка - отрицательный адрес\n");
    program = parse_program(".org -1");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Без аргументов
    printf("\n[ТЕСТ 3] Ошибка - нет аргументов\n");
    program = parse_program(".org");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста директивы .org ===\n");
}

void test_space_directive(void) {
    printf("\n=== Тест директивы .space ===\n");
    
    // Корректная директива
    printf("\n[ТЕСТ 1] Корректная директива .space\n");
    ast_program_t* program = parse_program(".space 42");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Нулевой размер
    printf("\n[ТЕСТ 2] Ошибка - нулевой размер\n");
    program = parse_program(".space 0");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Отрицательный размер
    printf("\n[ТЕСТ 3] Ошибка - отрицательный размер\n");
    program = parse_program(".space -1");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста директивы .space ===\n");
}

void test_align_directive(void) {
    printf("\n=== Тест директивы .align ===\n");
    
    // Корректная директива
    printf("\n[ТЕСТ 1] Корректная директива .align\n");
    ast_program_t* program = parse_program(".align 8");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Нулевое выравнивание
    printf("\n[ТЕСТ 2] Ошибка - нулевое выравнивание\n");
    program = parse_program(".align 0");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Отрицательное выравнивание
    printf("\n[ТЕСТ 3] Ошибка - отрицательное выравнивание\n");
    program = parse_program(".align -1");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста директивы .align ===\n");
}

void test_trit_directive(void) {
    printf("\n=== Тест директивы .trit ===\n");
    
    // Корректные директивы с десятичными числами
    printf("\n[ТЕСТ 1] Корректное десятичное число\n");
    ast_program_t* program = parse_program(".trit 0");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    printf("\n[ТЕСТ 2] Корректные десятичные числа через запятую\n");
    program = parse_program(".trit -1, 0, 1");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Корректные директивы с троичными числами
    printf("\n[ТЕСТ 3] Корректное троичное число\n");
    program = parse_program(".trit 0t0");
    if (!program) {
        printf("[DEBUG] Ошибка: parse_program вернул NULL\n");
    } else {
        printf("[DEBUG] parse_program успешно вернул программу\n");
    }
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    printf("\n[ТЕСТ 4] Корректные троичные числа через запятую\n");
    program = parse_program(".trit 0t-, 0t0, 0t+");
    if (!program) {
        printf("[DEBUG] Ошибка: parse_program вернул NULL\n");
    } else {
        printf("[DEBUG] parse_program успешно вернул программу\n");
    }
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Без аргументов
    printf("\n[ТЕСТ 5] Ошибка - нет аргументов\n");
    program = parse_program(".trit");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Недопустимые значения
    printf("\n[ТЕСТ 6] Ошибка - недопустимое десятичное значение\n");
    program = parse_program(".trit 2");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n[ТЕСТ 7] Ошибка - отрицательное недопустимое значение\n");
    program = parse_program(".trit -2");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n[ТЕСТ 8] Ошибка - недопустимое значение в списке\n");
    program = parse_program(".trit -1, 2, 1");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Недопустимые троичные значения
    printf("\n[ТЕСТ 9] Ошибка - некорректное троичное число\n");
    program = parse_program(".trit 0t++");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n[ТЕСТ 10] Ошибка - некорректное троичное число в списке\n");
    program = parse_program(".trit 0t-, 0t++, 0t+");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста директивы .trit ===\n");
}

void test_tryte_directive(void) {
    printf("\n=== Тест директивы .tryte ===\n");
    
    // Тест 1: Корректное десятичное число
    printf("\n[ТЕСТ 1] Корректное десятичное число\n");
    ast_program_t* program = parse_program(".tryte 0");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Тест 2: Корректные десятичные числа через запятую
    printf("\n[ТЕСТ 2] Корректные десятичные числа через запятую\n");
    program = parse_program(".tryte -364, 0, 364");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Тест 3: Корректное троичное число
    printf("\n[ТЕСТ 3] Корректное троичное число\n");
    program = parse_program(".tryte 0t000000");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Тест 4: Корректные троичные числа через запятую
    printf("\n[ТЕСТ 4] Корректные троичные числа через запятую\n");
    program = parse_program(".tryte 0t------, 0t000000, 0t++++++");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Тест 5: Ошибка - нет аргументов
    printf("\n[ТЕСТ 5] Ошибка - нет аргументов\n");
    program = parse_program(".tryte");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Тест 6: Ошибка - неверное количество тритов
    printf("\n[ТЕСТ 6] Ошибка - неверное количество тритов\n");
    program = parse_program(".tryte 0t+++++");  // 5 тритов вместо 6
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Тест 7: Ошибка - некорректные символы
    printf("\n[ТЕСТ 7] Ошибка - некорректные символы\n");
    program = parse_program(".tryte 0t------, 0t+++++2, 0t++++++");  // Некорректный символ '2'
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста директивы .tryte ===\n");
}

// Тесты для проверки меток
void test_label_definitions(void) {
    printf("\n=== Тест определений меток ===\n");
    ast_program_t* program;
    /*
    // Корректное определение метки
    printf("\n[ТЕСТ 1] Корректное определение метки\n");
    ast_program_t* program = parse_program("start: PUSH #0");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Повторное определение метки
    printf("\n[ТЕСТ 2] Ошибка - повторное определение метки\n");
    program = parse_program("start: PUSH #0\nstart: POP R0");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    */

    // Локальные метки в разных областях видимости
    printf("\n[ТЕСТ 3] Локальные метки в разных областях\n");
    program = parse_program(".scope func1\n.local: PUSH #1\n.endscope\n.scope func2\n.local: PUSH #2\n.endscope");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста определений меток ===\n");
}

void test_label_references(void) {
    printf("\n=== Тест ссылок на метки ===\n");
    
    // Корректная ссылка на метку
    printf("\n[ТЕСТ 1] Корректная ссылка на метку\n");
    ast_program_t* program = parse_program("start: PUSH #0\nJMP start");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Ссылка на неопределенную метку
    printf("\n[ТЕСТ 2] Ошибка - ссылка на неопределенную метку\n");
    program = parse_program("JMP undefined");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Ссылка на локальную метку из другой области видимости
    printf("\n[ТЕСТ 3] Ошибка - ссылка на недоступную локальную метку\n");
    program = parse_program(".scope func1\n.local: PUSH #1\n.endscope\n.scope func2\nJMP .local\n.endscope");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Слишком большое смещение для относительного перехода
    printf("\n[ТЕСТ 4] Ошибка - слишком большое смещение\n");
    char* large_program = malloc(10000);
    strcpy(large_program, "start: PUSH #0\n");
    for(int i = 0; i < 1000; i++) {
        strcat(large_program, "PUSH #0\n");
    }
    strcat(large_program, "JMP start");
    
    program = parse_program(large_program);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    free(large_program);
    
    printf("\n=== Завершение теста ссылок на метки ===\n");
}

void test_label_scopes(void) {
    printf("\n=== Тест областей видимости меток ===\n");
    
    // Глобальная метка видна во всех областях
    printf("\n[ТЕСТ 1] Глобальная метка видна в локальной области\n");
    ast_program_t* program = parse_program(
        "global: PUSH #0\n"
        ".scope func1\n"
        "JMP global\n"
        ".endscope"
    );
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Вложенные области видимости
    printf("\n[ТЕСТ 2] Вложенные области видимости\n");
    program = parse_program(
        ".scope outer\n"
        ".outer_local: PUSH #1\n"
        ".scope inner\n"
        "JMP .outer_local\n"  // Можно ссылаться на метку из внешней области
        ".inner_local: PUSH #2\n"
        ".endscope\n"
        "JMP .inner_local\n"  // Нельзя ссылаться на метку из внутренней области
        ".endscope"
    );
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Экспорт/импорт меток
    printf("\n[ТЕСТ 3] Экспорт локальной метки\n");
    program = parse_program(
        ".scope module1\n"
        ".export entry\n"
        "entry: PUSH #1\n"
        ".endscope\n"
        ".scope module2\n"
        "JMP entry\n"  // Можно ссылаться на экспортированную метку
        ".endscope"
    );
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста областей видимости меток ===\n");
}

void test_label_forward_references(void) {
    printf("\n=== Тест прямых и обратных ссылок на метки ===\n");
    
    // Прямая ссылка (метка определена после использования)
    printf("\n[ТЕСТ 1] Прямая ссылка на метку\n");
    ast_program_t* program = parse_program(
        "JMP forward\n"
        "PUSH #1\n"
        "forward: PUSH #2"
    );
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Циклические ссылки
    printf("\n[ТЕСТ 2] Циклические ссылки\n");
    program = parse_program(
        "loop1: PUSH #1\n"
        "JMP loop2\n"
        "loop2: PUSH #2\n"
        "JMP loop1"
    );
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Множественные ссылки на одну метку
    printf("\n[ТЕСТ 3] Множественные ссылки\n");
    program = parse_program(
        "JMP target\n"
        "PUSH #1\n"
        "JMP target\n"
        "PUSH #2\n"
        "JMP target\n"
        "target: RET"
    );
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста прямых и обратных ссылок на метки ===\n");
}

void test_instruction_operands(void) {
    printf("\n=== Тест проверки операндов инструкций ===\n");
    
    // Корректное количество операндов
    printf("\n[ТЕСТ 1] Корректное количество операндов\n");
    ast_program_t* program = parse_program("PUSH #42");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Недостаточно операндов
    printf("\n[ТЕСТ 2] Ошибка - недостаточно операндов\n");
    program = parse_program("PUSH");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    // Слишком много операндов
    printf("\n[ТЕСТ 3] Ошибка - слишком много операндов\n");
    program = parse_program("PUSH #42, R1");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста проверки операндов инструкций ===\n");
}

void test_instruction_addressing_modes(void) {
    printf("\n=== Тест режимов адресации инструкций ===\n");
    
    // Корректные режимы адресации
    printf("\n[ТЕСТ 1] Корректные режимы адресации\n");
    ast_program_t* program = parse_program(
        "PUSH #42\n"    // Непосредственный режим
        "POP R1\n"      // Регистровый режим
        "STORE @R2\n"   // Косвенный режим
    );
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_TRUE(semantic_analyzer_check_program(analyzer, program));
    ast_unref((ast_node_t*)program);
    
    // Некорректный режим адресации
    printf("\n[ТЕСТ 2] Ошибка - некорректный режим адресации\n");
    program = parse_program("INC @R1");  // INC поддерживает только регистровый режим
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста режимов адресации инструкций ===\n");
}

void test_unknown_instructions(void) {
    printf("\n=== Тест неизвестных инструкций ===\n");
    
    // Неизвестная инструкция
    printf("\n[ТЕСТ 1] Ошибка - неизвестная инструкция\n");
    ast_program_t* program = parse_program("UNKNOWN R1");
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(semantic_analyzer_check_program(analyzer, program));
    TEST_ASSERT_NOT_NULL(semantic_analyzer_get_error(analyzer));
    ast_unref((ast_node_t*)program);
    
    printf("\n=== Завершение теста неизвестных инструкций ===\n");
}

int main(void) {
    UNITY_BEGIN();
    
    // Тесты директив
    /*
    RUN_TEST(test_org_directive);
    RUN_TEST(test_space_directive);
    RUN_TEST(test_align_directive);
    RUN_TEST(test_trit_directive);
    RUN_TEST(test_tryte_directive);
    */
    
    // Тесты для проверки инструкций
    /*
    RUN_TEST(test_instruction_operands);
    RUN_TEST(test_instruction_addressing_modes);
    RUN_TEST(test_unknown_instructions);
    */

    // Тесты для проверки меток
    RUN_TEST(test_label_definitions);
    //RUN_TEST(test_label_references);
    //RUN_TEST(test_label_scopes);
    //RUN_TEST(test_label_forward_references);

    return UNITY_END();
} 