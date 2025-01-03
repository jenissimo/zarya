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
void test_and_instruction_true_true(void) {
    // В троичной логике: 1 AND 1 = 1
    instruction_t push1 = create_push_instruction(1);
    instruction_t push2 = create_push_instruction(1);
    instruction_t and = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_AND),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &and));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
}

void test_and_instruction_true_false(void) {
    // В троичной логике: 1 AND -1 = -1
    instruction_t push1 = create_push_instruction(1);
    instruction_t push2 = create_push_instruction(-1);
    instruction_t and = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_AND),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &and));
    TEST_ASSERT_EQUAL(-1, vm.memory[vm.sp.value].value);
}

void test_and_instruction_with_unknown(void) {
    // В троичной логике: 1 AND 0 = 0
    instruction_t push1 = create_push_instruction(1);
    instruction_t push2 = create_push_instruction(0);
    instruction_t and = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_AND),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &and));
    TEST_ASSERT_EQUAL(0, vm.memory[vm.sp.value].value);
}

void test_or_instruction_true_false(void) {
    // В троичной логике: 1 OR -1 = 1
    instruction_t push1 = create_push_instruction(1);
    instruction_t push2 = create_push_instruction(-1);
    instruction_t or = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_OR),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &or));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
}

void test_or_instruction_with_unknown(void) {
    // В троичной логике: 0 OR -1 = -1
    instruction_t push1 = create_push_instruction(0);
    instruction_t push2 = create_push_instruction(-1);
    instruction_t or = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_OR),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &or));
    TEST_ASSERT_EQUAL(-1, vm.memory[vm.sp.value].value);
}

void test_not_instruction_true(void) {
    // В троичной логике: NOT 1 = -1
    instruction_t push = create_push_instruction(1);
    instruction_t not = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_NOT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &not));
    TEST_ASSERT_EQUAL(-1, vm.memory[vm.sp.value].value);
}

void test_not_instruction_false(void) {
    // В троичной логике: NOT -1 = 1
    instruction_t push = create_push_instruction(-1);
    instruction_t not = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_NOT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &not));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
}

void test_not_instruction_unknown(void) {
    // В троичной логике: NOT 0 = 0
    instruction_t push = create_push_instruction(0);
    instruction_t not = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_NOT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &not));
    TEST_ASSERT_EQUAL(0, vm.memory[vm.sp.value].value);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_and_instruction_true_true);
    RUN_TEST(test_and_instruction_true_false);
    RUN_TEST(test_and_instruction_with_unknown);
    RUN_TEST(test_or_instruction_true_false);
    RUN_TEST(test_or_instruction_with_unknown);
    RUN_TEST(test_not_instruction_true);
    RUN_TEST(test_not_instruction_false);
    RUN_TEST(test_not_instruction_unknown);
    
    return UNITY_END();
} 