#include "test_common.h"

void test_comparison_operations(void) {
    test_print_module_start("comparison");
    
    // Тестовые значения для сравнения
    int test_values[] = {-1, 0, 1};
    
    // Проверяем все комбинации EQ
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
            instruction_t eq = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_EQ),
                .operand1 = TRYTE_FROM_INT(0),
                .operand2 = TRYTE_FROM_INT(0)
            };
            
            vm_reset(&vm);
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &eq));
            
            // Проверяем результат
            int expected;
            if (test_values[i] == test_values[j]) {
                expected = 1;  // Равны (включая 0 == 0)
            } else {
                expected = -1; // Не равны
            }
            
            TEST_ASSERT_EQUAL(expected, vm.memory[vm.sp.value].value);
            char msg[64];
            snprintf(msg, sizeof(msg), "EQ(%d,%d)=%d", test_values[i], test_values[j], expected);
            test_print_result(msg, true, NULL);
        }
    }

    // Проверяем все комбинации LE
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
            instruction_t le = {
                .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_LE),
                .operand1 = TRYTE_FROM_INT(0),
                .operand2 = TRYTE_FROM_INT(0)
            };
            
            vm_reset(&vm);
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
            TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &le));
            
            // Проверяем результат
            int expected;
            if (test_values[i] <= test_values[j]) {
                expected = 1;  // Меньше или равно
            } else {
                expected = -1; // Больше
            }
            
            TEST_ASSERT_EQUAL(expected, vm.memory[vm.sp.value].value);
            char msg[64];
            snprintf(msg, sizeof(msg), "LE(%d,%d)=%d", test_values[i], test_values[j], expected);
            test_print_result(msg, true, NULL);
        }
    }
    
    test_print_module_end("comparison");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_comparison_operations);
    return UNITY_END();
} 