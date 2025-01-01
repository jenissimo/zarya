#include "test_common.h"

void test_logical_operations(void) {
    test_print_module_start("logical");
    
    // Тестовые значения для тритов (-1, 0, 1)
    int test_values[] = {-1, 0, 1};
    
    // Проверяем все комбинации AND
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            // Загружаем операнды
            instruction_t push1 = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
                .operand1 = TRYTE_FROM_INT(test_values[i]),
                .operand2 = TRYTE_FROM_INT(0)
            };
            instruction_t push2 = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
                .operand1 = TRYTE_FROM_INT(test_values[j]),
                .operand2 = TRYTE_FROM_INT(0)
            };
            instruction_t and = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_AND),
                .operand1 = TRYTE_FROM_INT(0),
                .operand2 = TRYTE_FROM_INT(0)
            };
            
            vm_reset(&vm);
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &and));
            
            // Проверяем результат
            int expected;
            if (test_values[i] == 0 || test_values[j] == 0) {
                expected = 0;
            } else if (test_values[i] == -1 || test_values[j] == -1) {
                expected = -1;
            } else {
                expected = 1;
            }
            
            TEST_ASSERT_EQUAL(expected, vm.memory[vm.sp.value].value);
            char msg[64];
            snprintf(msg, sizeof(msg), "AND(%d,%d)=%d", test_values[i], test_values[j], expected);
            test_print_result(msg, true, NULL);
        }
    }

    // Проверяем все комбинации OR
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            // Загружаем операнды
            instruction_t push1 = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
                .operand1 = TRYTE_FROM_INT(test_values[i]),
                .operand2 = TRYTE_FROM_INT(0)
            };
            instruction_t push2 = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
                .operand1 = TRYTE_FROM_INT(test_values[j]),
                .operand2 = TRYTE_FROM_INT(0)
            };
            instruction_t or = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_OR),
                .operand1 = TRYTE_FROM_INT(0),
                .operand2 = TRYTE_FROM_INT(0)
            };
            
            vm_reset(&vm);
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &or));
            
            // Проверяем результат
            int expected;
            if (test_values[i] == 1 || test_values[j] == 1) {
                expected = 1;
            } else if (test_values[i] == 0) {
                expected = test_values[j];
            } else if (test_values[j] == 0) {
                expected = test_values[i];
            } else {
                expected = -1;
            }
            
            TEST_ASSERT_EQUAL(expected, vm.memory[vm.sp.value].value);
            char msg[64];
            snprintf(msg, sizeof(msg), "OR(%d,%d)=%d", test_values[i], test_values[j], expected);
            test_print_result(msg, true, NULL);
        }
    }
    
    // Проверяем NOT
    for (int i = 0; i < 3; i++) {
        // Загружаем операнд
        instruction_t push = {
            .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
            .operand1 = TRYTE_FROM_INT(test_values[i]),
            .operand2 = TRYTE_FROM_INT(0)
        };
        instruction_t not = {
            .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_NOT),
            .operand1 = TRYTE_FROM_INT(0),
            .operand2 = TRYTE_FROM_INT(0)
        };
        
        vm_reset(&vm);
        TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
        TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &not));
        
        // В троичной логике NOT:
        // NOT -1 = 1
        // NOT 0 = 0
        // NOT 1 = -1
        int expected = -test_values[i];
        
        TEST_ASSERT_EQUAL(expected, vm.memory[vm.sp.value].value);
        char msg[64];
        snprintf(msg, sizeof(msg), "NOT(%d)=%d", test_values[i], expected);
        test_print_result(msg, true, NULL);
    }
    
    test_print_module_end("logical");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_logical_operations);
    return UNITY_END();
} 