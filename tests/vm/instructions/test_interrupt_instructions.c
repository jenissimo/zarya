#include "unity.h"
#include "instructions.h"
#include "zarya_vm.h"
#include "zarya_config.h"

static vm_state_t vm;
static int last_interrupt_num;

// Обработчик прерываний для тестов
static vm_error_t test_handler(void* context, int interrupt_num) {
    (void)context;  // Подавляем предупреждение о неиспользуемом параметре
    last_interrupt_num = interrupt_num;
    return VM_OK;
}

// Инициализация перед каждым тестом
void setUp(void) {
    vm_init(&vm, MEMORY_SIZE_TRYTES);
    vm_reset(&vm);
    last_interrupt_num = -1;
    vm_set_interrupt_handler(&vm, test_handler, NULL);
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
void test_int_instruction(void) {
    // Проверяем вызов прерывания
    instruction_t push = create_push_instruction(42);  // Номер прерывания
    instruction_t int_inst = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_INT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &int_inst));
    TEST_ASSERT_EQUAL(42, last_interrupt_num);
}

void test_cli_instruction(void) {
    // Проверяем запрет прерываний
    instruction_t cli = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_CLI),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &cli));
    TEST_ASSERT_EQUAL(TRIT_NEGATIVE, GET_FLAG(&vm, FLAG_INTERRUPT_TRIT));  // Прерывания запрещены
}

void test_sti_instruction(void) {
    // Проверяем разрешение прерываний
    instruction_t sti = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_STI),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &sti));
    TEST_ASSERT_EQUAL(TRIT_POSITIVE, GET_FLAG(&vm, FLAG_INTERRUPT_TRIT));  // Прерывания разрешены
}

void test_interrupt_disabled(void) {
    // Проверяем, что прерывания не обрабатываются, когда они запрещены
    instruction_t cli = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_CLI),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push = create_push_instruction(42);  // Номер прерывания
    instruction_t int_inst = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_INT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &cli));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_ERROR_INTERRUPTS_DISABLED, execute_instruction(&vm, &int_inst));
    TEST_ASSERT_EQUAL(-1, last_interrupt_num);  // Прерывание не должно быть обработано
}

void test_interrupt_enabled_after_sti(void) {
    // Проверяем, что прерывания обрабатываются после их разрешения
    instruction_t cli = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_CLI),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t sti = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_STI),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    instruction_t push = create_push_instruction(42);  // Номер прерывания
    instruction_t int_inst = {
        .opcode = MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_INT),
        .operand1 = TRYTE_FROM_INT(0),
        .operand2 = TRYTE_FROM_INT(0)
    };
    
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &cli));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &sti));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &push));
    TEST_ASSERT_EQUAL(VM_OK, execute_instruction(&vm, &int_inst));
    TEST_ASSERT_EQUAL(42, last_interrupt_num);  // Прерывание должно быть обработано
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_int_instruction);
    RUN_TEST(test_cli_instruction);
    RUN_TEST(test_sti_instruction);
    RUN_TEST(test_interrupt_disabled);
    RUN_TEST(test_interrupt_enabled_after_sti);
    
    return UNITY_END();
} 