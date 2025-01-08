#include "unity.h"
#include "trias/parser.h"
#include "trias/lexer.h"
#include "trias/ast.h"
#include "zarya_vm.h"
#include "zarya_config.h"
#include "logging.h"

static vm_state_t vm;
static ast_manager_t* ast_manager;
static symbol_table_t* symbol_table;

// Инициализация перед каждым тестом
void setUp(void) {
    vm_init(&vm, MEMORY_SIZE_TRYTES);
    vm_reset(&vm);
    ast_manager = ast_manager_create();
    TEST_ASSERT_NOT_NULL(ast_manager);
    symbol_table = symbol_table_create();
    TEST_ASSERT_NOT_NULL(symbol_table);
}

// Очистка после каждого теста
void tearDown(void) {
    symbol_table_destroy(symbol_table);
    ast_manager_destroy(ast_manager);
    vm_free(&vm);
}

// Тесты
void test_parser_instructions(void) {
    const char* input = "PUSH #42\nPOP\nHALT";
    lexer_t* lexer = lexer_create_from_string(input);
    TEST_ASSERT_NOT_NULL(lexer);
    
    parser_t* parser = parser_create(lexer);
    TEST_ASSERT_NOT_NULL(parser);
    
    // Устанавливаем зависимости
    parser_set_ast_manager(parser, ast_manager);
    parser_set_symbol_table(parser, symbol_table);
    
    // Разбираем программу
    ast_program_t* program = parser_parse_program(parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_EQUAL(3, program->statement_count);
    
    // Проверяем первую инструкцию (PUSH #42)
    ast_instruction_t* inst = (ast_instruction_t*)program->statements[0];
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(AST_INSTRUCTION, inst->base.type);
    TEST_ASSERT_EQUAL_STRING("PUSH", inst->mnemonic);
    TEST_ASSERT_EQUAL(1, inst->operand_count);
    TEST_ASSERT_EQUAL(OPERAND_IMMEDIATE, inst->operands[0]->type);
    TEST_ASSERT_EQUAL(42, inst->operands[0]->immediate);
    
    // Проверяем вторую инструкцию (POP)
    inst = (ast_instruction_t*)program->statements[1];
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(AST_INSTRUCTION, inst->base.type);
    TEST_ASSERT_EQUAL_STRING("POP", inst->mnemonic);
    TEST_ASSERT_EQUAL(0, inst->operand_count);
    
    // Проверяем третью инструкцию (HALT)
    inst = (ast_instruction_t*)program->statements[2];
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL(AST_INSTRUCTION, inst->base.type);
    TEST_ASSERT_EQUAL_STRING("HALT", inst->mnemonic);
    TEST_ASSERT_EQUAL(0, inst->operand_count);
    
    ast_unref((ast_node_t*)program);
    parser_destroy(parser);
}

// Тест разбора директив
void test_parser_directives(void) {
    const char* input = ".org 100\n.space 42\n.align 8";
    lexer_t* lexer = lexer_create_from_string(input);
    TEST_ASSERT_NOT_NULL(lexer);
    
    parser_t* parser = parser_create(lexer);
    TEST_ASSERT_NOT_NULL(parser);
    
    // Устанавливаем зависимости
    parser_set_ast_manager(parser, ast_manager);
    parser_set_symbol_table(parser, symbol_table);
    
    // Разбираем программу
    ast_program_t* program = parser_parse_program(parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_EQUAL(3, program->statement_count);
    
    // Проверяем первую директиву (.org 100)
    ast_directive_t* dir = (ast_directive_t*)program->statements[0];
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_EQUAL(AST_DIRECTIVE, dir->base.type);
    TEST_ASSERT_EQUAL_STRING("org", dir->name);
    TEST_ASSERT_EQUAL(1, dir->arg_count);
    ast_operand_t* arg = (ast_operand_t*)dir->args[0];
    TEST_ASSERT_EQUAL(OPERAND_IMMEDIATE, arg->type);
    TEST_ASSERT_EQUAL(100, arg->immediate);
    
    // Проверяем вторую директиву (.space 42)
    dir = (ast_directive_t*)program->statements[1];
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_EQUAL(AST_DIRECTIVE, dir->base.type);
    TEST_ASSERT_EQUAL_STRING("space", dir->name);
    TEST_ASSERT_EQUAL(1, dir->arg_count);
    arg = (ast_operand_t*)dir->args[0];
    TEST_ASSERT_EQUAL(OPERAND_IMMEDIATE, arg->type);
    TEST_ASSERT_EQUAL(42, arg->immediate);
    
    // Проверяем третью директиву (.align 8)
    dir = (ast_directive_t*)program->statements[2];
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_EQUAL(AST_DIRECTIVE, dir->base.type);
    TEST_ASSERT_EQUAL_STRING("align", dir->name);
    TEST_ASSERT_EQUAL(1, dir->arg_count);
    arg = (ast_operand_t*)dir->args[0];
    TEST_ASSERT_EQUAL(OPERAND_IMMEDIATE, arg->type);
    TEST_ASSERT_EQUAL(8, arg->immediate);
    
    ast_unref((ast_node_t*)program);
    parser_destroy(parser);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_parser_instructions);
    RUN_TEST(test_parser_directives);
    
    return UNITY_END();
} 