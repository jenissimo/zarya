#include "test_common.h"

// Обработчик прерываний для тестов
static vm_error_t test_interrupt_handler(void* context, int interrupt_num) {
    (void)context;  // Подавляем предупреждение о неиспользуемом параметре
    last_interrupt_num = interrupt_num;
    return VM_OK;
}

void test_interrupt_operations(void) {
    test_print_module_start("interrupts");
    
    // Устанавливаем обработчик прерываний
    vm_set_interrupt_handler(&vm, test_interrupt_handler, &vm);

    // Тестируем включение прерываний
    instruction_t enable_int = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_INT),
        .operand1 = TRYTE_FROM_INT(1),  // 1 = включить
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &enable_int));
    TEST_ASSERT_TRUE(vm.flags.value & FLAG_INTERRUPTS_ENABLED);
    test_print_result("INT_ENABLE", true, NULL);

    // Тестируем вызов прерывания
    instruction_t call_int = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_INT),
        .operand1 = TRYTE_FROM_INT(42),  // номер прерывания
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &call_int));
    TEST_ASSERT_EQUAL(42, last_interrupt_num);
    test_print_result("INT_CALL", true, NULL);

    // Тестируем выключение прерываний
    instruction_t disable_int = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_INT),
        .operand1 = TRYTE_FROM_INT(-1),  // -1 = выключить
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &disable_int));
    TEST_ASSERT_FALSE(vm.flags.value & FLAG_INTERRUPTS_ENABLED);
    test_print_result("INT_DISABLE", true, NULL);

    // Пытаемся вызвать прерывание при выключенных прерываниях
    TEST_ASSERT_EQUAL(VM_ERROR_INTERRUPTS_DISABLED, execute_instruction(&vm, &call_int));
    test_print_result("INT_DISABLED_CALL", true, NULL);
    
    test_print_module_end("interrupts");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_interrupt_operations);
    return UNITY_END();
} 