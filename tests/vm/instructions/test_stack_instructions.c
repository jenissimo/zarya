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
void test_push_instruction(void) {
    instruction_t push = create_push_instruction(42);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
}

void test_pop_instruction(void) {
    // Сначала помещаем значение в стек
    instruction_t push = create_push_instruction(42);
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    
    // Затем извлекаем его
    instruction_t pop = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &pop));
    TEST_ASSERT_EQUAL(-1, vm.sp.value);  // Стек должен быть пустым
    TEST_ASSERT_EQUAL(42, vm.registers[0].value);  // Значение должно быть в регистре 0
}

void test_dup_instruction(void) {
    // Помещаем значение в стек
    instruction_t push = create_push_instruction(42);
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    
    // Дублируем его
    instruction_t dup = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_DUP),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &dup));
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value].value);
    TEST_ASSERT_EQUAL(42, vm.memory[vm.sp.value - 1].value);
}

void test_swap_instruction(void) {
    // Помещаем два значения в стек
    instruction_t push1 = create_push_instruction(1);
    instruction_t push2 = create_push_instruction(2);
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push1));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push2));
    
    // Меняем их местами
    instruction_t swap = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_SWAP),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &swap));
    TEST_ASSERT_EQUAL(1, vm.memory[vm.sp.value].value);
    TEST_ASSERT_EQUAL(2, vm.memory[vm.sp.value - 1].value);
}

void test_stack_underflow(void) {
    // Пытаемся извлечь значение из пустого стека
    instruction_t pop = {
        .opcode = MAKE_OPCODE(ADDR_MODE_REGISTER, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_ERROR_STACK_UNDERFLOW, execute_instruction(&vm, &pop));
}

void test_pop_invalid_addressing_mode(void) {
    // Сначала помещаем значение в стек
    instruction_t push = create_push_instruction(42);
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    
    // Пытаемся извлечь его с неверным режимом адресации
    instruction_t pop = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_POP),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_ERROR_INVALID_ADDRESSING_MODE, execute_instruction(&vm, &pop));
    TEST_ASSERT_EQUAL(0, vm.sp.value);  // Стек не должен измениться
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_push_instruction);
    RUN_TEST(test_pop_instruction);
    RUN_TEST(test_dup_instruction);
    RUN_TEST(test_swap_instruction);
    RUN_TEST(test_stack_underflow);
    RUN_TEST(test_pop_invalid_addressing_mode);
    
    return UNITY_END();
} 