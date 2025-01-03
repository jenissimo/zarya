#include "unity.h"
#include "instructions.h"
#include "zarya_vm.h"
#include "zarya_config.h"

static vm_state_t vm;

// Инициализация перед каждым тестом
void setUp(void) {
    vm_init(&vm, MEMORY_SIZE_TRYTES);
    vm_reset(&vm);
}

// Очистка после каждого теста
void tearDown(void) {
    vm_free(&vm);
}

// Вспомогательная функция для создания инструкции PUSH
static instruction_t create_push_instruction(int value) {
    return (instruction_t) {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(value),
        .operand2 = TRYTE_FROM_INT(0)
    };
}

// Тесты
void test_add_instruction(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(3);
    instruction_t add = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_ADD),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &add));
    TEST_ASSERT_EQUAL(8, vm.memory[vm.sp.value].value);
}

void test_sub_instruction(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(3);
    instruction_t sub = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_SUB),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &sub));
    TEST_ASSERT_EQUAL(2, vm.memory[vm.sp.value].value);
}

void test_mul_instruction(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(3);
    instruction_t mul = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_MUL),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &mul));
    TEST_ASSERT_EQUAL(15, vm.memory[vm.sp.value].value);
}

void test_div_instruction(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(3);
    instruction_t div = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_DIV),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &div));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
}

void test_div_by_zero(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(0);
    instruction_t div = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_DIV),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_ERROR_DIVISION_BY_ZERO, execute_instruction(&vm, &div));
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_add_instruction);
    RUN_TEST(test_sub_instruction);
    RUN_TEST(test_mul_instruction);
    RUN_TEST(test_div_instruction);
    RUN_TEST(test_div_by_zero);
    
    return UNITY_END();
} 