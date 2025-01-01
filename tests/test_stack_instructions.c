#include "test_common.h"

void test_stack_operations(void) {
    test_print_module_start("stack");
    
    // Тест PUSH/POP
    instruction_t push = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(42),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
    test_print_result("PUSH", true, NULL);
    
    instruction_t pop = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &pop));
    test_print_result("POP", true, NULL);
    
    // Тест DUP
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));  // PUSH 42
    instruction_t dup = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_DUP),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &dup));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value - 1].value);
    test_print_result("DUP", true, NULL);
    
    test_print_module_end("stack");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stack_operations);
    return UNITY_END();
} 