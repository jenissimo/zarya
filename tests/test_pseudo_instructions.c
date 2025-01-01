#include "unity.h"
#include "zarya_vm.h"
#include "trias/codegen.h"
#include "trias/trias_instructions.h"
#include "trias/parser.h"
#include "trias/lexer.h"
#include "vm/stack.h"
#include "types.h"

// Вспомогательные функции
static vm_state_t vm;
static codegen_t codegen;

void setUp(void) {
    vm_init(&vm, 1024);  // Выделяем 1024 трайта памяти
    codegen_init(&codegen);
}

void tearDown(void) {
    vm_free(&vm);
    codegen_free(&codegen);
}

// Вспомогательная функция для компиляции и выполнения кода
static vm_error_t compile_and_run(const char* code) {
    lexer_t lexer;
    lexer_init(&lexer, code);
    
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_node_t* ast = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Добавляем вывод AST для отладки
    printf("----- AST Dump -----\n");
    print_ast(ast, 0);
    printf("--------------------\n");

    vm_error_t err = codegen_generate(&codegen, ast);
    free_ast(ast);  // Освобождаем память AST
    if (err != VM_OK) return err;
    
    // Загружаем код в VM и выполняем
    vm_load_program(&vm, codegen.code, codegen.code_size);
    return vm_run(&vm);
}

// Тесты для MOV
void test_mov_registers(void) {
    vm.registers[0] = TRYTE_FROM_INT(42);
    printf("Before MOV: R0=%d\n", TRYTE_GET_VALUE(vm.registers[0]));
    
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("MOV R1, R0\n"));
    
    printf("After MOV: R0=%d, R1=%d\n", 
           TRYTE_GET_VALUE(vm.registers[0]),
           TRYTE_GET_VALUE(vm.registers[1]));
           
    TEST_ASSERT_EQUAL(42, TRYTE_GET_VALUE(vm.registers[1]));
}

// Тесты для INC/DEC
void test_inc_dec(void) {
    // INC R0 -> PUSH R0; PUSH 1; ADD; POP R0
    vm.registers[0] = TRYTE_FROM_INT(41);
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("INC R0\n"));
    TEST_ASSERT_EQUAL(42, TRYTE_GET_VALUE(vm.registers[0]));
    
    // DEC R0 -> PUSH R0; PUSH 1; SUB; POP R0
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("DEC R0\n"));
    TEST_ASSERT_EQUAL(41, TRYTE_GET_VALUE(vm.registers[0]));
}

// Тесты для PUSHR/POPR
void test_pushr_popr(void) {
    // PUSHR R0 -> PUSH R0
    vm.registers[0] = TRYTE_FROM_INT(42);
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("PUSHR R0\n"));
    tryte_t value;
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(42, TRYTE_GET_VALUE(value));
    
    // POPR R1 -> POP R1
    TEST_ASSERT_EQUAL(VM_OK, stack_push(&vm, TRYTE_FROM_INT(42)));
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("POPR R1\n"));
    TEST_ASSERT_EQUAL(42, TRYTE_GET_VALUE(vm.registers[1]));
}

// Тест для CLEAR
void test_clear(void) {
    // Заполняем стек
    TEST_ASSERT_EQUAL(VM_OK, stack_push(&vm, TRYTE_FROM_INT(1)));
    TEST_ASSERT_EQUAL(VM_OK, stack_push(&vm, TRYTE_FROM_INT(2)));
    TEST_ASSERT_EQUAL(VM_OK, stack_push(&vm, TRYTE_FROM_INT(3)));
    
    // CLEAR 2 -> DROP; DROP
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("CLEAR 2\n"));
    tryte_t value;
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(1, TRYTE_GET_VALUE(value));
    TEST_ASSERT_EQUAL(VM_ERROR_STACK_UNDERFLOW, stack_pop(&vm, &value));
}

// Тест для CMP
void test_cmp(void) {
    // CMP R0, R1 -> PUSH R0; PUSH R1; SUB
    vm.registers[0] = TRYTE_FROM_INT(42);
    vm.registers[1] = TRYTE_FROM_INT(42);
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("CMP R0, R1\n"));
    tryte_t value;
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(0, TRYTE_GET_VALUE(value)); // Равны
    
    vm.registers[0] = TRYTE_FROM_INT(43);
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("CMP R0, R1\n"));
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_TRUE(TRYTE_GET_VALUE(value) > 0); // R0 > R1
    
    vm.registers[0] = TRYTE_FROM_INT(41);
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("CMP R0, R1\n"));
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_TRUE(TRYTE_GET_VALUE(value) < 0); // R0 < R1
}

// Тест для TEST
void test_test(void) {
    // TEST R0 -> PUSH R0; DUP; AND
    vm.registers[0] = TRYTE_FROM_INT(1);  // Положительное
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("TEST R0\n"));
    tryte_t value;
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(1, TRYTE_GET_VALUE(value));
    
    vm.registers[0] = TRYTE_FROM_INT(-1); // Отрицательное
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("TEST R0\n"));
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(-1, TRYTE_GET_VALUE(value));
    
    vm.registers[0] = TRYTE_FROM_INT(0);  // Ноль
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run("TEST R0\n"));
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(0, TRYTE_GET_VALUE(value));
}

// Тест на последовательность псевдоинструкций
void test_pseudo_sequence(void) {
    const char* code = 
        "MOV R0, R1\n"   // Копируем значение
        "INC R0\n"       // Увеличиваем
        "PUSHR R0\n"     // Сохраняем в стек
        "DEC R0\n"       // Уменьшаем
        "CMP R0, R1\n";  // Сравниваем
    
    vm.registers[1] = TRYTE_FROM_INT(42);
    TEST_ASSERT_EQUAL(VM_OK, compile_and_run(code));
    
    tryte_t value;
    // Проверяем результат сравнения (должны быть равны)
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(0, TRYTE_GET_VALUE(value));
    // Проверяем значение в стеке (должно быть 43)
    TEST_ASSERT_EQUAL(VM_OK, stack_pop(&vm, &value));
    TEST_ASSERT_EQUAL(43, TRYTE_GET_VALUE(value));
    // Проверяем значение в R0 (должно быть 42)
    TEST_ASSERT_EQUAL(42, TRYTE_GET_VALUE(vm.registers[0]));
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_mov_registers);
    RUN_TEST(test_inc_dec);
    RUN_TEST(test_pushr_popr);
    RUN_TEST(test_clear);
    RUN_TEST(test_cmp);
    RUN_TEST(test_test);
    RUN_TEST(test_pseudo_sequence);

    return UNITY_END();
} 