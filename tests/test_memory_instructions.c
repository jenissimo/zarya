#include "test_common.h"

void test_memory_operations(void) {
    test_print_module_start("memory");
    
    // Тест LOAD и STORE с непосредственной адресацией
    // Сохраняем значение 42 по адресу 100
    instruction_t push_addr = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(100),  // Адрес
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_addr));
    
    instruction_t push_val = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(42),   // Значение
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_val));
    
    instruction_t store = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_STORE),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &store));
    TEST_ASSERT_EQUAL(42, vm.memory[100].value);
    test_print_result("STORE_IMM", true, NULL);
    
    // Загружаем значение с адреса 100
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_addr));
    
    instruction_t load = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_LOAD),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &load));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
    test_print_result("LOAD_IMM", true, NULL);
    
    // Тест LOAD и STORE с регистровой адресацией
    vm_reset(&vm);
    
    // Сохраняем адрес в регистр
    vm.registers[REG_IDX].value = 200;
    
    // Сохраняем значение 123 по адресу из регистра
    instruction_t push_val2 = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(123),   // Значение
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_val2));
    
    instruction_t store_reg = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_STORE),
        .operand1 = TRYTE_FROM_INT(REG_IDX),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &store_reg));
    TEST_ASSERT_EQUAL(123, vm.memory[200].value);
    test_print_result("STORE_REG", true, NULL);
    
    // Загружаем значение с адреса из регистра
    instruction_t load_reg = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_LOAD),
        .operand1 = TRYTE_FROM_INT(REG_IDX),
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &load_reg));
    TEST_ASSERT_EQUAL(123, vm.memory[vm.sp.value].value);
    test_print_result("LOAD_REG", true, NULL);
    
    // Тест обращения к несуществующему адресу
    vm_reset(&vm);
    
    // Пытаемся загрузить значение с неверного адреса
    instruction_t push_invalid = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(MEMORY_SIZE_TRYTES + 1),  // Неверный адрес
        .operand2 = TRYTE_FROM_INT(0)
    };
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_invalid));
    TEST_ASSERT_EQUAL(VM_ERROR_INVALID_ADDRESS, execute_instruction(&vm, &load));
    test_print_result("LOAD_INVALID", true, NULL);
    
    // Пытаемся сохранить значение по неверному адресу
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_invalid));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_val));
    TEST_ASSERT_EQUAL(VM_ERROR_INVALID_ADDRESS, execute_instruction(&vm, &store));
    test_print_result("STORE_INVALID", true, NULL);
    
    test_print_module_end("memory");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_memory_operations);
    return UNITY_END();
} 