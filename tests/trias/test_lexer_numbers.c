#include "unity.h"
#include "trias/lexer.h"
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

// Тесты
void test_lexer_numbers(void) {
    const char* input = "PUSH #42\nPUSH #-13\nPUSH #+7";
    lexer_t* lexer = lexer_create_from_string(input);
    TEST_ASSERT_NOT_NULL(lexer);
    
    // Тест первого числа (42)
    token_t token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("PUSH", token.text);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_HASH, token.type);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_NUMBER, token.type);
    TEST_ASSERT_EQUAL(42, token.value.number);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_NEWLINE, token.type);
    token_destroy(&token);
    
    // Тест второго числа (-13)
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("PUSH", token.text);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_HASH, token.type);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_NUMBER, token.type);
    TEST_ASSERT_EQUAL(-13, token.value.number);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_NEWLINE, token.type);
    token_destroy(&token);
    
    // Тест третьего числа (+7)
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("PUSH", token.text);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_HASH, token.type);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_NUMBER, token.type);
    TEST_ASSERT_EQUAL(7, token.value.number);
    token_destroy(&token);
    
    token = lexer_next_token(lexer);
    TEST_ASSERT_EQUAL(TOKEN_EOF, token.type);
    token_destroy(&token);
    
    lexer_destroy(lexer);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_lexer_numbers);
    
    return UNITY_END();
} 