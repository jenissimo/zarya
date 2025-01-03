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
void test_jmp_instruction(void) {
    // Загружаем программу:
    // PUSH 42
    // JMP 100
    // PUSH 10  ; Эта инструкция должна быть пропущена
    // PUSH 20  ; Эта инструкция должна быть пропущена
    // [100] PUSH 30
    
    instruction_t push1 = create_push_instruction(42);
    instruction_t jmp = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_JMP),
        .operand1 = TRYTE_FROM_INT(100),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push2 = create_push_instruction(10);
    instruction_t push3 = create_push_instruction(20);
    instruction_t push4 = create_push_instruction(30);
    
    // Загружаем программу в память
    vm.memory[0] = push1.opcode;
    vm.memory[1] = push1.operand1;
    vm.memory[2] = push1.operand2;
    
    vm.memory[3] = jmp.opcode;
    vm.memory[4] = jmp.operand1;
    vm.memory[5] = jmp.operand2;
    
    vm.memory[6] = push2.opcode;
    vm.memory[7] = push2.operand1;
    vm.memory[8] = push2.operand2;
    
    vm.memory[9] = push3.opcode;
    vm.memory[10] = push3.operand1;
    vm.memory[11] = push3.operand2;
    
    vm.memory[100] = push4.opcode;
    vm.memory[101] = push4.operand1;
    vm.memory[102] = push4.operand2;
    
    // Выполняем программу
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));  // PUSH 42
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &jmp));   // JMP 100
    TEST_ASSERT_EQUAL(100, vm.pc.value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push4));  // PUSH 30
    TEST_ASSERT_EQUAL(30, vm.memory[vm.sp.value].value);
}

void test_jz_instruction_taken(void) {
    // Загружаем программу:
    // PUSH 0
    // JZ 100
    // PUSH 10  ; Эта инструкция должна быть пропущена
    // [100] PUSH 20
    
    instruction_t push1 = create_push_instruction(0);
    instruction_t jz = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_JZ),
        .operand1 = TRYTE_FROM_INT(100),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push2 = create_push_instruction(10);
    instruction_t push3 = create_push_instruction(20);
    
    // Загружаем программу в память
    vm.memory[0] = push1.opcode;
    vm.memory[1] = push1.operand1;
    vm.memory[2] = push1.operand2;
    
    vm.memory[3] = jz.opcode;
    vm.memory[4] = jz.operand1;
    vm.memory[5] = jz.operand2;
    
    vm.memory[6] = push2.opcode;
    vm.memory[7] = push2.operand1;
    vm.memory[8] = push2.operand2;
    
    vm.memory[100] = push3.opcode;
    vm.memory[101] = push3.operand1;
    vm.memory[102] = push3.operand2;
    
    // Выполняем программу
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));  // PUSH 0
    TEST_ASSERT_EQUAL(0, vm.memory[vm.sp.value].value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &jz));    // JZ 100
    TEST_ASSERT_EQUAL(100, vm.pc.value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push3));  // PUSH 20
    TEST_ASSERT_EQUAL(20, vm.memory[vm.sp.value].value);
}

void test_jz_instruction_not_taken(void) {
    // Загружаем программу:
    // PUSH 1
    // JZ 100
    // PUSH 10  ; Эта инструкция должна быть выполнена
    // [100] PUSH 20
    
    instruction_t push1 = create_push_instruction(1);
    instruction_t jz = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_JZ),
        .operand1 = TRYTE_FROM_INT(100),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push2 = create_push_instruction(10);
    
    // Загружаем программу в память
    vm.memory[0] = push1.opcode;
    vm.memory[1] = push1.operand1;
    vm.memory[2] = push1.operand2;
    
    vm.memory[3] = jz.opcode;
    vm.memory[4] = jz.operand1;
    vm.memory[5] = jz.operand2;
    
    vm.memory[6] = push2.opcode;
    vm.memory[7] = push2.operand1;
    vm.memory[8] = push2.operand2;
    
    // Выполняем программу
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));  // PUSH 1
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &jz));    // JZ 100 (не должен перейти)
    TEST_ASSERT_EQUAL(6, vm.pc.value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));  // PUSH 10
    TEST_ASSERT_EQUAL(10, vm.memory[vm.sp.value].value);
}

void test_call_ret_instructions(void) {
    // Загружаем программу:
    // PUSH 1
    // CALL 100
    // PUSH 3
    // HALT
    // [100] PUSH 2
    // RET
    
    instruction_t push1 = create_push_instruction(1);
    instruction_t call = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_CALL),
        .operand1 = TRYTE_FROM_INT(100),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push2 = create_push_instruction(2);
    instruction_t ret = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_RET),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push3 = create_push_instruction(3);
    instruction_t halt = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_HALT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    // Загружаем программу в память
    vm.memory[0] = push1.opcode;
    vm.memory[1] = push1.operand1;
    vm.memory[2] = push1.operand2;
    
    vm.memory[3] = call.opcode;
    vm.memory[4] = call.operand1;
    vm.memory[5] = call.operand2;
    
    vm.memory[6] = push3.opcode;
    vm.memory[7] = push3.operand1;
    vm.memory[8] = push3.operand2;
    
    vm.memory[9] = halt.opcode;
    vm.memory[10] = halt.operand1;
    vm.memory[11] = halt.operand2;
    
    vm.memory[100] = push2.opcode;
    vm.memory[101] = push2.operand1;
    vm.memory[102] = push2.operand2;
    
    vm.memory[103] = ret.opcode;
    vm.memory[104] = ret.operand1;
    vm.memory[105] = ret.operand2;
    
    // Выполняем программу
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));  // PUSH 1
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &call));   // CALL 100
    TEST_ASSERT_EQUAL(100, vm.pc.value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));  // PUSH 2
    TEST_ASSERT_EQUAL(2, vm.memory[vm.sp.value].value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &ret));    // RET
    TEST_ASSERT_EQUAL(6, vm.pc.value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push3));  // PUSH 3
    TEST_ASSERT_EQUAL(3, vm.memory[vm.sp.value].value);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &halt));   // HALT
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_jmp_instruction);
    RUN_TEST(test_jz_instruction_taken);
    RUN_TEST(test_jz_instruction_not_taken);
    RUN_TEST(test_call_ret_instructions);
    
    return UNITY_END();
} 