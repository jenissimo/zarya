#include "test_common.h"

void test_arithmetic_operations(void) {
    test_print_module_start("arithmetic");
    
    // Тест ADD
    instruction_t push1 = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(5),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push2 = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(3),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t add = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_ADD),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &add));
    TEST_ASSERT_EQUAL(8, vm.memory[vm.sp.value].value);
    test_print_result("ADD", true, NULL);
    
    // Тест SUB
    vm_reset(&vm);
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));  // 5
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));  // 3
    instruction_t sub = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_SUB),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &sub));
    TEST_ASSERT_EQUAL(2, vm.memory[vm.sp.value].value);
    test_print_result("SUB", true, NULL);
    
    // Тест MUL
    vm_reset(&vm);
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));  // 5
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));  // 3
    instruction_t mul = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_MUL),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &mul));
    TEST_ASSERT_EQUAL(15, vm.memory[vm.sp.value].value);
    test_print_result("MUL", true, NULL);
    
    // Тест DIV
    vm_reset(&vm);
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));  // 5
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));  // 3
    instruction_t div = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_DIV),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &div));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);  // 5/3 = 1 (целочисленное деление)
    test_print_result("DIV", true, NULL);
    
    test_print_module_end("arithmetic");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_arithmetic_operations);
    return UNITY_END();
} 