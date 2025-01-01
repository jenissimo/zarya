#include <stdlib.h>
#include "ast.h"
#include "lexer.h"

// Освобождение памяти узла AST
void free_ast(ast_node_t* node) {
    if (!node) return;
    
    // Сначала освобождаем следующий узел, чтобы избежать переполнения стека
    ast_node_t* next = node->next;
    node->next = NULL;  // Предотвращаем циклические ссылки
    
    // Освобождаем память в зависимости от типа узла
    switch (node->type) {
        case NODE_INSTRUCTION:
            // Освобождаем имя инструкции
            if (node->value.instruction.name) {
                free(node->value.instruction.name);
                node->value.instruction.name = NULL;
            }
            // Освобождаем операнды
            for (int i = 0; i < 2; i++) {
                if (node->value.instruction.operands[i]) {
                    free_ast(node->value.instruction.operands[i]);
                    node->value.instruction.operands[i] = NULL;
                }
            }
            break;
            
        case NODE_DIRECTIVE:
            // Освобождаем значение директивы
            if (node->value.directive.directive_type == TOKEN_DIR_DS && 
                node->value.directive.value.string) {
                free(node->value.directive.value.string);
                node->value.directive.value.string = NULL;
            }
            break;
            
        case NODE_LABEL:
            // Освобождаем имя метки
            if (node->value.label.text) {
                free(node->value.label.text);
                node->value.label.text = NULL;
            }
            break;
            
        case NODE_IDENTIFIER:
            // Освобождаем идентификатор
            if (node->value.identifier.text) {
                free(node->value.identifier.text);
                node->value.identifier.text = NULL;
            }
            break;
            
        case NODE_STRING:
            // Освобождаем строку
            if (node->value.string.text) {
                free(node->value.string.text);
                node->value.string.text = NULL;
            }
            break;
            
        case NODE_NUMBER:
        case NODE_CHAR:
        case NODE_REGISTER:
            // Для этих типов нет динамически выделенной памяти
            break;
            
        case NODE_PROGRAM:
            // Для программы нет специальных данных
            break;
            
        default:
            // Неизвестный тип узла - пропускаем
            break;
    }
    
    // Освобождаем сам узел
    free(node);
    
    // Освобождаем следующий узел
    if (next) {
        free_ast(next);
    }
}

// Создание узла AST
ast_node_t* create_ast_node(node_type_t type) {
    ast_node_t* node = (ast_node_t*)calloc(1, sizeof(ast_node_t));
    if (node) {
        node->type = type;
        // Инициализируем операнды NULL-ами для инструкций
        if (type == NODE_INSTRUCTION) {
            node->value.instruction.operands[0] = NULL;
            node->value.instruction.operands[1] = NULL;
        }
    }
    return node;
}

// Добавление узла в конец списка
void append_ast_node(ast_node_t** head, ast_node_t* node) {
    if (!head || !node) return;
    
    if (!*head) {
        *head = node;
        return;
    }
    
    ast_node_t* current = *head;
    while (current->next) {
        current = current->next;
    }
    current->next = node;
} 