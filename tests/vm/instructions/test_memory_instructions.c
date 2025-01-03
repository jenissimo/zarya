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
void test_load_immediate(void) {
    // LOAD #42
    instruction_t load = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_LOAD),
        .operand1 = TRYTE_FROM_INT(42),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &load));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
}

void test_load_register(void) {
    // Сначала загружаем значение в регистр R0
    instruction_t push = create_push_instruction(42);
    instruction_t pop = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),  // R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &pop));
    
    // Затем загружаем значение из регистра
    instruction_t load = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_LOAD),
        .operand1 = TRYTE_FROM_INT(0),  // R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &load));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
}

void test_load_indirect(void) {
    // Сначала помещаем значение в память
    instruction_t push1 = create_push_instruction(42);
    instruction_t push2 = create_push_instruction(100);  // адрес
    instruction_t store = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_STORE),
        .operand1 = TRYTE_FROM_INT(100),  // Сохраняем по адресу 100
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &store));
    
    // Загружаем адрес в регистр R0
    instruction_t push3 = create_push_instruction(100);
    instruction_t pop = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),  // R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push3));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &pop));
    
    // Загружаем значение по адресу из регистра
    instruction_t load = {
        .opcode = MAKE_OPCODE(ADDR_MODE_INDIRECT, OP_LOAD),
        .operand1 = TRYTE_FROM_INT(0),  // @R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &load));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
}

void test_store_immediate(void) {
    // PUSH 42, STORE #100
    instruction_t push = create_push_instruction(42);
    instruction_t store = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_STORE),
        .operand1 = TRYTE_FROM_INT(100),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &store));
    TEST_ASSERT_EQUAL(42, vm.memory[100].value);
}

void test_store_register(void) {
    // Сначала загружаем адрес в регистр R0
    instruction_t push1 = create_push_instruction(100);
    instruction_t pop = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),  // R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &pop));
    
    // Затем сохраняем значение по адресу из регистра
    instruction_t push2 = create_push_instruction(42);
    instruction_t store = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_STORE),
        .operand1 = TRYTE_FROM_INT(0),  // R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &store));
    TEST_ASSERT_EQUAL(42, vm.memory[100].value);
}

void test_store_indirect(void) {
    // Сначала загружаем адрес указателя в регистр R0
    instruction_t push1 = create_push_instruction(50);
    instruction_t pop1 = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),  // R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &pop1));
    
    // Сохраняем адрес целевой ячейки по адресу указателя
    instruction_t push2 = create_push_instruction(100);
    instruction_t store1 = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_STORE),
        .operand1 = TRYTE_FROM_INT(0),  // R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &store1));
    
    // Сохраняем значение по адресу из указателя
    instruction_t push3 = create_push_instruction(42);
    instruction_t store2 = {
        .opcode = MAKE_OPCODE(ADDR_MODE_INDIRECT, OP_STORE),
        .operand1 = TRYTE_FROM_INT(0),  // @R0
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push3));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &store2));
    TEST_ASSERT_EQUAL(42, vm.memory[100].value);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_load_immediate);
    RUN_TEST(test_load_register);
    RUN_TEST(test_load_indirect);
    RUN_TEST(test_store_immediate);
    RUN_TEST(test_store_register);
    RUN_TEST(test_store_indirect);
    
    return UNITY_END();
} 