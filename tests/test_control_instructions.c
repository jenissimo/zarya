#include "test_common.h"

void test_control_operations(void) {
    test_print_module_start("control");
        
    // Тест безусловного перехода JMP
    instruction_t program[] = {
        {  // PUSH 100
            .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
            .operand1 = TRYTE_FROM_INT(100),
            .operand2 = TRYTE_FROM_INT(0)
        },
        {  // JMP
            .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_JMP),
            .operand1 = TRYTE_FROM_INT(0),
            .operand2 = TRYTE_FROM_INT(0)
        }
    };
    
    // Загружаем программу в память
    for (size_t i = 0; i < sizeof(program) / sizeof(program[0]); i++) {
        word_t word = encode_instruction(&program[i]);
        // Создаем трайты и копируем в них триты
        tryte_t tryte0 = {0}, tryte1 = {0}, tryte2 = {0};
        memcpy(tryte0.trits, &word.trits[0], TRITS_PER_TRYTE);
        memcpy(tryte1.trits, &word.trits[TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        memcpy(tryte2.trits, &word.trits[2 * TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        // Обновляем значения трайтов
        update_tryte_value(&tryte0);
        update_tryte_value(&tryte1);
        update_tryte_value(&tryte2);
        // Сохраняем трайты в память
        vm.memory[i * 3] = tryte0;
        vm.memory[i * 3 + 1] = tryte1;
        vm.memory[i * 3 + 2] = tryte2;
    }
    
    // Выполняем PUSH 100
    TEST_ASSERT_EQUAL(VM_OK, vm_step(&vm));
    TEST_ASSERT_EQUAL(3, vm.pc.value);
    TEST_ASSERT_EQUAL(100, vm.memory[vm.sp.value].value);
    
    // Выполняем JMP
    TEST_ASSERT_EQUAL(VM_OK, vm_step(&vm));
    TEST_ASSERT_EQUAL(100, vm.pc.value);
    test_print_result("JMP", true, NULL);
    
    // Тест условного перехода JZ (не должен выполнять переход при условии -1)
    vm_reset(&vm);
    instruction_t push_addr = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(200),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push_cond = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
        .operand1 = TRYTE_FROM_INT(-1),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t jz = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_JZ),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    int pc_before = vm.pc.value;
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_addr));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_cond));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &jz));
    TEST_ASSERT_EQUAL(pc_before + INSTRUCTION_SIZE, vm.pc.value);  // Не перешел, а продвинулся на одну инструкцию
    test_print_result("JZ_FALSE", true, NULL);
    
    // Тест условного перехода JZ (должен выполнять переход при условии 0)
    vm_reset(&vm);
    push_cond.operand1 = TRYTE_FROM_INT(0);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_addr));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_cond));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &jz));
    TEST_ASSERT_EQUAL(200, vm.pc.value);  // Должен перейти
    test_print_result("JZ_TRUE", true, NULL);
    
    // Тест условного перехода JNZ (должен выполнять переход при условии 1)
    vm_reset(&vm);
    push_addr.operand1 = TRYTE_FROM_INT(300);
    push_cond.operand1 = TRYTE_FROM_INT(1);
    instruction_t jnz = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_JNZ),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_addr));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_cond));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &jnz));
    TEST_ASSERT_EQUAL(300, vm.pc.value);  // Должен перейти
    test_print_result("JNZ_TRUE", true, NULL);
    
    // Тест условного перехода JNZ (не должен выполнять переход при условии 0)
    vm_reset(&vm);
    push_addr.operand1 = TRYTE_FROM_INT(300);
    push_cond.operand1 = TRYTE_FROM_INT(0);
    
    pc_before = vm.pc.value;
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_addr));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push_cond));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &jnz));
    TEST_ASSERT_EQUAL(pc_before + INSTRUCTION_SIZE, vm.pc.value);  // Не перешел, а продвинулся на одну инструкцию
    test_print_result("JNZ_FALSE", true, NULL);
    
    // Тест CALL и RET
    vm_reset(&vm);
    
    // Загружаем программу с вызовом подпрограммы
    instruction_t call_program[] = {
        {  // PUSH 100 (адрес подпрограммы)
            .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),
            .operand1 = TRYTE_FROM_INT(100),
            .operand2 = TRYTE_FROM_INT(0)
        },
        {  // CALL
            .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_CALL),
            .operand1 = TRYTE_FROM_INT(0),
            .operand2 = TRYTE_FROM_INT(0)
        }
    };
    
    // Загружаем программу в память
    for (size_t i = 0; i < sizeof(call_program) / sizeof(call_program[0]); i++) {
        word_t word = encode_instruction(&call_program[i]);
        // Создаем трайты и копируем в них триты
        tryte_t tryte0 = {0}, tryte1 = {0}, tryte2 = {0};
        memcpy(tryte0.trits, &word.trits[0], TRITS_PER_TRYTE);
        memcpy(tryte1.trits, &word.trits[TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        memcpy(tryte2.trits, &word.trits[2 * TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        // Обновляем значения трайтов
        update_tryte_value(&tryte0);
        update_tryte_value(&tryte1);
        update_tryte_value(&tryte2);
        // Сохраняем трайты в память
        vm.memory[i * 3] = tryte0;
        vm.memory[i * 3 + 1] = tryte1;
        vm.memory[i * 3 + 2] = tryte2;
    }
    
    // Загружаем подпрограмму по адресу 100
    instruction_t subroutine[] = {
        {  // RET
            .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_RET),
            .operand1 = TRYTE_FROM_INT(0),
            .operand2 = TRYTE_FROM_INT(0)
        }
    };
    
    // Загружаем подпрограмму в память
    for (size_t i = 0; i < sizeof(subroutine) / sizeof(subroutine[0]); i++) {
        word_t word = encode_instruction(&subroutine[i]);
        // Создаем трайты и копируем в них триты
        tryte_t tryte0 = {0}, tryte1 = {0}, tryte2 = {0};
        memcpy(tryte0.trits, &word.trits[0], TRITS_PER_TRYTE);
        memcpy(tryte1.trits, &word.trits[TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        memcpy(tryte2.trits, &word.trits[2 * TRITS_PER_TRYTE], TRITS_PER_TRYTE);
        // Обновляем значения трайтов
        update_tryte_value(&tryte0);
        update_tryte_value(&tryte1);
        update_tryte_value(&tryte2);
        // Сохраняем трайты в память по адресу 100
        vm.memory[100 + i * 3] = tryte0;
        vm.memory[100 + i * 3 + 1] = tryte1;
        vm.memory[100 + i * 3 + 2] = tryte2;
    }
    
    // Выполняем PUSH 100
    TEST_ASSERT_EQUAL(VM_OK, vm_step(&vm));
    TEST_ASSERT_EQUAL(3, vm.pc.value);
    TEST_ASSERT_EQUAL(100, vm.memory[vm.sp.value].value);
    
    // Выполняем CALL
    TEST_ASSERT_EQUAL(VM_OK, vm_step(&vm));
    TEST_ASSERT_EQUAL(100, vm.pc.value);
    TEST_ASSERT_EQUAL(6, vm.memory[vm.sp.value].value);
    test_print_result("CALL", true, NULL);
    
    // Выполняем RET
    TEST_ASSERT_EQUAL(VM_OK, vm_step(&vm));
    TEST_ASSERT_EQUAL(6, vm.pc.value);
    TEST_ASSERT_EQUAL(-1, vm.sp.value);  // После RET стек должен быть пустым
    test_print_result("RET", true, NULL);
    
    test_print_module_end("control");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_control_operations);
    return UNITY_END();
} 