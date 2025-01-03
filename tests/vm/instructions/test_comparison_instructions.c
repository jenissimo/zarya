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
void test_eq_instruction_true(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(5);
    instruction_t eq = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_EQ),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &eq));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
}

void test_eq_instruction_false(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(3);
    instruction_t eq = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_EQ),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &eq));
    TEST_ASSERT_EQUAL(-1, vm.memory[vm.sp.value].value);
}

void test_lt_instruction_true(void) {
    instruction_t push1 = create_push_instruction(3);
    instruction_t push2 = create_push_instruction(5);
    instruction_t lt = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_LT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &lt));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
}

void test_lt_instruction_false(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(3);
    instruction_t lt = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_LT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &lt));
    TEST_ASSERT_EQUAL(-1, vm.memory[vm.sp.value].value);
}

void test_gt_instruction_true(void) {
    instruction_t push1 = create_push_instruction(5);
    instruction_t push2 = create_push_instruction(3);
    instruction_t gt = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_GT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &gt));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
}

void test_gt_instruction_false(void) {
    instruction_t push1 = create_push_instruction(3);
    instruction_t push2 = create_push_instruction(5);
    instruction_t gt = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_GT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &gt));
    TEST_ASSERT_EQUAL(-1, vm.memory[vm.sp.value].value);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_eq_instruction_true);
    RUN_TEST(test_eq_instruction_false);
    RUN_TEST(test_lt_instruction_true);
    RUN_TEST(test_lt_instruction_false);
    RUN_TEST(test_gt_instruction_true);
    RUN_TEST(test_gt_instruction_false);
    
    return UNITY_END();
} 