#include "unity.h"
#include "instructions.h"
#include "zarya_vm.h"
#include "zarya_config.h"
#include "trit_ops.h"
#include "stack.h"

static vm_state_t vm;

void setUp(void) {
    // Инициализация перед каждым тестом
    TEST_ASSERT_EQUAL(VM_OK, vm_init(&vm, MEMORY_SIZE_TRYTES));
    vm_reset(&vm);  // Сбрасываем состояние VM перед каждым тестом
}

void tearDown(void) {
    // Очистка после каждого теста
    vm_free(&vm);
}

void test_vm_init(void) {
    printf("\n=== Тесты инициализации VM ===\n");
    
    // Освобождаем ресурсы от setUp
    vm_free(&vm);
    
    // Проверяем инициализацию с корректными параметрами
    TEST_ASSERT_EQUAL(VM_OK, vm_init(&vm, MEMORY_SIZE_TRYTES));
    TEST_ASSERT_NOT_NULL(vm.memory);
    TEST_ASSERT_EQUAL(-1, vm.sp.value);  // SP указывает на -1 (пустой стек)
    TEST_ASSERT_EQUAL(0, vm.pc.value);  // PC указывает на начало памяти
    TEST_ASSERT_NULL(vm.interrupt_callback);  // Обработчик прерываний не установлен
    TEST_ASSERT_NULL(vm.interrupt_context);  // Контекст прерываний не установлен
    
    // Проверяем, что память очищена
    for (int i = 0; i < MEMORY_SIZE_TRYTES; i++) {
        TEST_ASSERT_EQUAL(0, vm.memory[i].value);
    }
    
    // Проверяем инициализацию с некорректными параметрами
    TEST_ASSERT_EQUAL(VM_ERROR_INVALID_ADDRESS, vm_init(NULL, MEMORY_SIZE_TRYTES));
}

void test_vm_free(void) {
    printf("\n=== Тесты освобождения VM ===\n");
    
    // Освобождаем ресурсы от setUp
    vm_free(&vm);
    
    // Инициализируем VM
    TEST_ASSERT_EQUAL(VM_OK, vm_init(&vm, MEMORY_SIZE_TRYTES));
    TEST_ASSERT_NOT_NULL(vm.memory);
    
    // Освобождаем ресурсы
    vm_free(&vm);
    TEST_ASSERT_NULL(vm.memory);
    
    // Повторное освобождение не должно вызывать ошибок
    vm_free(&vm);
    TEST_ASSERT_NULL(vm.memory);
    
    // Освобождение NULL не должно вызывать ошибок
    vm_free(NULL);
}

void test_vm_reset(void) {
    printf("\n=== Тесты сброса VM ===\n");
    
    // Освобождаем ресурсы от setUp
    vm_free(&vm);
    
    // Инициализируем VM
    TEST_ASSERT_EQUAL(VM_OK, vm_init(&vm, MEMORY_SIZE_TRYTES));
    
    // Заполняем память и регистры тестовыми значениями
    for (int i = 0; i < MEMORY_SIZE_TRYTES; i++) {
        vm.memory[i].value = i;
    }
    vm.sp.value = 42;
    vm.pc.value = 24;
    
    // Сбрасываем состояние
    vm_reset(&vm);
    
    // Проверяем, что память очищена
    for (int i = 0; i < MEMORY_SIZE_TRYTES; i++) {
        TEST_ASSERT_EQUAL(0, vm.memory[i].value);
    }
    
    // Проверяем, что регистры сброшены
    TEST_ASSERT_EQUAL(-1, vm.sp.value);  // SP = -1 для пустого стека
    TEST_ASSERT_EQUAL(0, vm.pc.value);  // PC указывает на начало памяти
    
    // Проверяем сброс с некорректными параметрами
    vm_reset(NULL);  // Не должно вызывать ошибок
}

void test_vm_step(void) {
    // Создаем простую программу: PUSH 1
    tryte_t program[] = {
        MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),  // PUSH с непосредственной адресацией
        TRYTE_FROM_INT(1),        // Операнд 1
        TRYTE_FROM_INT(0)         // Операнд 2 (не используется)
    };
    
    // Загружаем программу
    vm_error_t err = vm_load_program(&vm, program, 3);
    TEST_ASSERT_EQUAL(VM_OK, err);
    
    // Выполняем один шаг
    err = vm_step(&vm);
    TEST_ASSERT_EQUAL(VM_OK, err);
    
    // Проверяем, что значение 1 было помещено в стек
    tryte_t value;
    err = stack_pop(&vm, &value);
    TEST_ASSERT_EQUAL(VM_OK, err);
    TEST_ASSERT_EQUAL(1, value.value);
}

void test_vm_run(void) {
    // Создаем программу: PUSH 1, PUSH 1, ADD, HALT
    tryte_t program[] = {
        MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),  // PUSH с непосредственной адресацией
        TRYTE_FROM_INT(1),        // 1
        TRYTE_FROM_INT(0),        // -
        MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_PUSH),  // PUSH с непосредственной адресацией
        TRYTE_FROM_INT(1),        // 1
        TRYTE_FROM_INT(0),        // -
        MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_ADD),   // ADD с непосредственной адресацией
        TRYTE_FROM_INT(0),        // -
        TRYTE_FROM_INT(0),        // -
        MAKE_OPCODE(ADDR_MODE_IMMEDIATE, OP_HALT),  // HALT с непосредственной адресацией
        TRYTE_FROM_INT(0),        // -
        TRYTE_FROM_INT(0)         // -
    };
    
    // Загружаем программу
    vm_error_t err = vm_load_program(&vm, program, 12);
    TEST_ASSERT_EQUAL(VM_OK, err);
    
    // Запускаем программу
    err = vm_run(&vm);
    TEST_ASSERT_EQUAL(VM_OK, err);
    
    // Проверяем результат: 1 + 1 = 2
    tryte_t result;
    err = stack_pop(&vm, &result);
    TEST_ASSERT_EQUAL(VM_OK, err);
    TEST_ASSERT_EQUAL(2, result.value);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_vm_init);
    RUN_TEST(test_vm_free);
    RUN_TEST(test_vm_reset);
    RUN_TEST(test_vm_step);
    RUN_TEST(test_vm_run);
    return UNITY_END();
} 