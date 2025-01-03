#include "ast.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_NODES_CAPACITY 32

struct ast_manager {
    size_t node_count;     // Общее количество узлов
    size_t alloc_count;    // Количество выделений памяти
    size_t free_count;     // Количество освобождений памяти
    struct {
        ast_node_t** nodes;
        size_t count;
        size_t capacity;
    } owned_nodes;         // Узлы, которыми владеет менеджер
    
    struct {
        ast_node_t** nodes;
        size_t count;
        size_t capacity;
    } weak_refs;          // Слабые ссылки на узлы
};

// Предварительные объявления функций уничтожения
void ast_destroy_program(ast_node_t* node);
void ast_destroy_label(ast_node_t* node);
void ast_destroy_instruction(ast_node_t* node);

// Создание менеджера AST
ast_manager_t* ast_manager_create(void) {
    ast_manager_t* manager = calloc(1, sizeof(ast_manager_t));
    if (!manager) {
        LOG_ERROR(LOG_PARSER, "Не удалось выделить память для менеджера AST");
        return NULL;
    }
    
    manager->owned_nodes.nodes = calloc(INITIAL_NODES_CAPACITY, sizeof(ast_node_t*));
    manager->owned_nodes.capacity = INITIAL_NODES_CAPACITY;
    
    manager->weak_refs.nodes = calloc(INITIAL_NODES_CAPACITY, sizeof(ast_node_t*));
    manager->weak_refs.capacity = INITIAL_NODES_CAPACITY;
    
    if (!manager->owned_nodes.nodes || !manager->weak_refs.nodes) {
        LOG_ERROR(LOG_PARSER, "Не удалось выделить память для массивов узлов");
        ast_manager_destroy(manager);
        return NULL;
    }
    
    return manager;
}

// Инициализация базового узла
static void ast_init_base(ast_node_t* node, ast_manager_t* manager, ast_node_type_t type, const source_loc_t* loc) {
    if (!node || !manager || !loc) return;
    
    node->type = type;
    node->manager = manager;
    node->loc = *loc;
    node->is_owned = true;
    node->refs.strong_count = 1;
    node->refs.weak_count = 0;
    
    // Устанавливаем функцию уничтожения в зависимости от типа
    switch (type) {
        case AST_PROGRAM:
            node->destroy = ast_destroy_program;
            break;
        case AST_LABEL:
            node->destroy = ast_destroy_label;
            break;
        case AST_INSTRUCTION:
            node->destroy = ast_destroy_instruction;
            break;
        case AST_OPERAND:
            node->destroy = ast_destroy_operand;
            break;
        case AST_DIRECTIVE:
            node->destroy = ast_destroy_directive;
            break;
        default:
            node->destroy = NULL;
            break;
    }
    
    AST_TRACK_ALLOC(node);
}

// Проверка валидности узла
bool ast_is_valid(ast_node_t* node) {
    return node && node->is_owned && node->refs.strong_count > 0;
}

// Создание сильной ссылки
void ast_strong_ref(ast_node_t* node) {
    if (!ast_is_valid(node)) return;
    node->refs.strong_count++;
    LOG_TRACE(LOG_PARSER, "Увеличен счетчик сильных ссылок для узла типа %d (strong_count = %zu)",
              node->type, node->refs.strong_count);
}

// Создание слабой ссылки
void ast_weak_ref(ast_node_t* node) {
    if (!ast_is_valid(node)) return;
    node->refs.weak_count++;
    LOG_TRACE(LOG_PARSER, "Увеличен счетчик слабых ссылок для узла типа %d (weak_count = %zu)",
              node->type, node->refs.weak_count);
}

// Освобождение ресурсов узла (внутренняя функция)
static void ast_free_resources(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM: {
            ast_program_t* program = (ast_program_t*)node;
            free(program->statements);
            program->statements = NULL;
            program->statement_count = 0;
            break;
        }
        case AST_LABEL: {
            ast_label_t* label = (ast_label_t*)node;
            free(label->name);
            label->name = NULL;
            break;
        }
        case AST_INSTRUCTION: {
            ast_instruction_t* inst = (ast_instruction_t*)node;
            free(inst->mnemonic);
            free(inst->operands);
            inst->mnemonic = NULL;
            inst->operands = NULL;
            inst->operand_count = 0;
            break;
        }
        case AST_OPERAND: {
            ast_operand_t* op = (ast_operand_t*)node;
            if (op->type == OPERAND_LABEL) {
                free(op->label);
                op->label = NULL;
            }
            break;
        }
        case AST_DIRECTIVE: {
            ast_directive_t* dir = (ast_directive_t*)node;
            free(dir->name);
            free(dir->args);
            dir->name = NULL;
            dir->args = NULL;
            dir->arg_count = 0;
            break;
        }
        default:
            break;
    }
}

// Освобождение дочерних узлов (внутренняя функция)
static void ast_free_children(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM: {
            ast_program_t* program = (ast_program_t*)node;
            if (program->statements) {
                for (size_t i = 0; i < program->statement_count; i++) {
                    if (program->statements[i]) {
                        ast_unref(program->statements[i]);
                    }
                }
            }
            break;
        }
        case AST_INSTRUCTION: {
            ast_instruction_t* inst = (ast_instruction_t*)node;
            if (inst->operands) {
                for (size_t i = 0; i < inst->operand_count; i++) {
                    if (inst->operands[i]) {
                        ast_unref(&inst->operands[i]->base);
                    }
                }
            }
            break;
        }
        case AST_DIRECTIVE: {
            ast_directive_t* dir = (ast_directive_t*)node;
            if (dir->args) {
                for (size_t i = 0; i < dir->arg_count; i++) {
                    if (dir->args[i]) {
                        ast_unref(dir->args[i]);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

// Добавление узла в менеджер
static bool ast_manager_add_node(ast_manager_t* manager, ast_node_t* node) {
    if (!manager || !node) return false;
    
    // Проверяем, нужно ли увеличить массив owned_nodes
    if (manager->owned_nodes.count >= manager->owned_nodes.capacity) {
        size_t new_capacity = manager->owned_nodes.capacity * 2;
        ast_node_t** new_nodes = realloc(manager->owned_nodes.nodes, 
                                        new_capacity * sizeof(ast_node_t*));
        if (!new_nodes) {
            LOG_ERROR(LOG_PARSER, "Не удалось увеличить массив узлов");
            return false;
        }
        
        manager->owned_nodes.nodes = new_nodes;
        manager->owned_nodes.capacity = new_capacity;
    }
    
    // Добавляем узел
    manager->owned_nodes.nodes[manager->owned_nodes.count++] = node;
    manager->node_count++;
    manager->alloc_count++;
    
    return true;
}

// Удаление узла из менеджера
static void ast_manager_remove_node(ast_manager_t* manager, ast_node_t* node) {
    if (!manager || !node) return;
    
    // Ищем узел в массиве owned_nodes
    for (size_t i = 0; i < manager->owned_nodes.count; i++) {
        if (manager->owned_nodes.nodes[i] == node) {
            // Сдвигаем все последующие узлы
            memmove(&manager->owned_nodes.nodes[i],
                   &manager->owned_nodes.nodes[i + 1],
                   (manager->owned_nodes.count - i - 1) * sizeof(ast_node_t*));
            manager->owned_nodes.count--;
            manager->free_count++;
            break;
        }
    }
    
    // Очищаем слабые ссылки на этот узел
    for (size_t i = 0; i < manager->weak_refs.count; i++) {
        if (manager->weak_refs.nodes[i] == node) {
            memmove(&manager->weak_refs.nodes[i],
                   &manager->weak_refs.nodes[i + 1],
                   (manager->weak_refs.count - i - 1) * sizeof(ast_node_t*));
            manager->weak_refs.count--;
            i--; // Проверяем текущий индекс снова
        }
    }
}

// Создание программы
ast_program_t* ast_create_program(ast_manager_t* manager) {
    if (!manager) return NULL;
    
    ast_program_t* prog = calloc(1, sizeof(ast_program_t));
    if (!prog) {
        LOG_ERROR(LOG_PARSER, "Не удалось выделить память для программы");
        return NULL;
    }
    
    source_loc_t loc = {0}; // Программа не имеет конкретной локации
    ast_init_base(&prog->base, manager, AST_PROGRAM, &loc);
    
    if (!ast_manager_add_node(manager, &prog->base)) {
        free(prog);
        return NULL;
    }
    
    return prog;
}

// Создание инструкции
ast_instruction_t* ast_create_instruction(ast_manager_t* ast, const char* mnemonic, const source_loc_t* loc) {
    if (!ast || !mnemonic || !loc) return NULL;
    
    ast_instruction_t* inst = calloc(1, sizeof(ast_instruction_t));
    if (!inst) return NULL;
    
    // Инициализируем базовые поля
    ast_init_base(&inst->base, ast, AST_INSTRUCTION, loc);
    
    // Копируем мнемонику
    inst->mnemonic = strdup(mnemonic);
    if (!inst->mnemonic) {
        ast_free_node(ast, &inst->base);
        return NULL;
    }
    
    // Инициализируем массив операндов
    inst->operands = NULL;
    inst->operand_count = 0;
    
    // Добавляем узел в менеджер
    if (!ast_manager_add_node(ast, &inst->base)) {
        free(inst->mnemonic);
        free(inst);
        return NULL;
    }
    
    return inst;
}

// Создание метки
ast_label_t* ast_create_label(ast_manager_t* manager, const char* name, bool is_local, const source_loc_t* loc) {
    if (!manager || !name || !loc) {
        return NULL;
    }
    
    // Выделяем память под узел
    ast_label_t* label = calloc(1, sizeof(ast_label_t));
    if (!label) {
        return NULL;
    }
    
    // Инициализируем базовые поля
    ast_init_base(&label->base, manager, AST_LABEL, loc);
    
    // Копируем имя
    label->name = strdup(name);
    if (!label->name) {
        free(label);
        return NULL;
    }
    
    label->is_local = is_local;
    
    // Добавляем узел в менеджер
    if (!ast_manager_add_node(manager, &label->base)) {
        free(label->name);
        free(label);
        return NULL;
    }
    
    return label;
}

// Создание операнда
ast_operand_t* ast_create_operand(ast_manager_t* manager, operand_type_t type, const source_loc_t* loc) {
    if (!manager || !loc) return NULL;
    
    ast_operand_t* op = calloc(1, sizeof(ast_operand_t));
    if (!op) {
        LOG_ERROR(LOG_PARSER, "Не удалось выделить память для операнда");
        return NULL;
    }
    
    ast_init_base(&op->base, manager, AST_OPERAND, loc);
    op->type = type;
    
    if (!ast_manager_add_node(manager, &op->base)) {
        free(op);
        return NULL;
    }
    
    return op;
}

// Увеличение счетчика ссылок
void ast_ref(ast_node_t* node) {
    if (node) {
        node->refs.strong_count++;
        LOG_TRACE(LOG_AST, "Увеличен счетчик ссылок для узла типа %d (ref_count = %zu)",
                  node->type, node->refs.strong_count);
    }
}

// Уменьшение счетчика ссылок
void ast_unref(ast_node_t* node) {
    if (!node) return;
    
    // Проверяем валидность узла
    if (!ast_is_valid(node)) {
        LOG_WARN(LOG_AST, "ast_unref: Попытка освободить невалидный узел %p типа %d", 
                 (void*)node, node->type);
                 
        // Если узел все еще принадлежит менеджеру, удаляем его оттуда
        if (node->manager) {
            LOG_DEBUG(LOG_AST, "ast_unref: Удаляем невалидный узел %p из менеджера %p", 
                      (void*)node, (void*)node->manager);
            ast_manager_remove_node(node->manager, node);
        }
        
        // Освобождаем память узла
        LOG_DEBUG(LOG_AST, "ast_unref: Освобождаем память невалидного узла %p", (void*)node);
        free(node);
        return;
    }
    
    LOG_DEBUG(LOG_AST, "ast_unref: Уменьшаем счетчик для узла %p типа %d (strong_count=%d -> %d)", 
              (void*)node, node->type, node->refs.strong_count, node->refs.strong_count - 1);
    
    if (--node->refs.strong_count == 0) {
        LOG_DEBUG(LOG_AST, "ast_unref: Освобождаем узел %p типа %d", 
                  (void*)node, node->type);
        
        ast_manager_t* manager = node->manager;
        ast_node_type_t type = node->type;
        void (*destroy)(ast_node_t*) = node->destroy;
        
        // 1. Уведомляем менеджер об удалении узла
        if (manager) {
            LOG_DEBUG(LOG_AST, "ast_unref: Уведомляем менеджер %p об удалении узла %p", 
                      (void*)manager, (void*)node);
            ast_manager_remove_node(manager, node);
        }
        
        // 2. Освобождаем общие ресурсы (строки, массивы и т.д.)
        LOG_DEBUG(LOG_AST, "ast_unref: Освобождаем ресурсы для %p", (void*)node);
        ast_free_resources(node);
        
        // 3. Освобождаем дочерние узлы через общий механизм
        LOG_DEBUG(LOG_AST, "ast_unref: Освобождаем дочерние узлы для %p", (void*)node);
        ast_free_children(node);
        
        // 4. Помечаем узел как недействительный
        node->is_owned = false;
        node->type = AST_INVALID;
        
        // 5. Вызываем деструктор для специфичной очистки
        if (destroy) {
            LOG_DEBUG(LOG_AST, "ast_unref: Вызываем деструктор для узла %p", (void*)node);
            destroy(node);
        } else {
            LOG_DEBUG(LOG_AST, "ast_unref: Освобождаем память узла %p", (void*)node);
            free(node);
        }
    }
}

// Освобождение узла AST
void ast_free_node(ast_manager_t* manager, ast_node_t* node) {
    if (!manager || !node) return;
    
    // Сначала удаляем узел из менеджера
    ast_manager_remove_node(manager, node);
    
    // Освобождаем дочерние узлы
    ast_free_children(node);
    
    // Освобождаем ресурсы узла
    ast_free_resources(node);
    
    LOG_DEBUG(LOG_AST, "Освобожден узел типа %d", node->type);
    manager->free_count++;
    free(node);
}

// Добавление оператора в программу
void ast_program_add_statement(ast_program_t* program, ast_node_t* stmt) {
    if (!program || !stmt) {
        return;
    }
    
    // Увеличиваем размер массива при необходимости
    if (program->statement_count >= program->statement_capacity) {
        size_t new_capacity = program->statement_capacity == 0 ? 4 : program->statement_capacity * 2;
        ast_node_t** new_statements = realloc(program->statements, new_capacity * sizeof(ast_node_t*));
        if (!new_statements) {
            return;
        }
        program->statements = new_statements;
        program->statement_capacity = new_capacity;
    }
    
    // Добавляем оператор и увеличиваем счетчик ссылок
    program->statements[program->statement_count++] = stmt;
    ast_ref(stmt);
    
    LOG_DEBUG(LOG_AST, "Добавлен оператор типа %d в программу (всего %zu)",
              stmt->type, program->statement_count);
}

// Добавление операнда в инструкцию
void ast_instruction_add_operand(ast_instruction_t* inst, ast_operand_t* operand) {
    if (!inst || !operand) return;
    
    // Увеличиваем массив операндов
    size_t new_size = (inst->operand_count + 1) * sizeof(ast_operand_t*);
    ast_operand_t** new_operands = realloc(inst->operands, new_size);
    if (!new_operands) {
        LOG_ERROR(LOG_PARSER, "Не удалось добавить операнд в инструкцию");
        return;
    }
    
    inst->operands = new_operands;
    inst->operands[inst->operand_count] = operand;
    inst->operand_count++;
    
    operand->base.parent = &inst->base;
    ast_ref(&operand->base);  // Увеличиваем счетчик ссылок, так как инструкция теперь владеет операндом
    
    LOG_DEBUG(LOG_PARSER, "Добавлен операнд типа %d в инструкцию '%s' (всего %zu)",
              operand->type, inst->mnemonic, inst->operand_count);
}

// Создание директивы
ast_directive_t* ast_create_directive(ast_manager_t* manager, const char* name, const source_loc_t* loc) {
    ast_directive_t* dir = calloc(1, sizeof(ast_directive_t));
    if (!dir) {
        LOG_ERROR(LOG_PARSER, "Не удалось создать узел директивы");
        return NULL;
    }
    
    ast_init_base(&dir->base, manager, AST_DIRECTIVE, loc);
    dir->name = strdup(name);
    if (!dir->name) {
        LOG_ERROR(LOG_PARSER, "Не удалось скопировать имя директивы");
        free(dir);
        return NULL;
    }
    dir->args = NULL;
    dir->arg_count = 0;
    
    // Добавляем узел в менеджер
    if (!ast_manager_add_node(manager, &dir->base)) {
        free(dir->name);
        free(dir);
        return NULL;
    }
    
    manager->node_count++;
    manager->alloc_count++;
    
    LOG_DEBUG(LOG_PARSER, "Создан узел директивы '%s' на строке %zu, колонка %zu",
              name, loc->line, loc->column);
    return dir;
}

// Добавление аргумента к директиве
void ast_directive_add_arg(ast_directive_t* directive, ast_node_t* arg) {
    if (!directive || !arg) return;
    
    // Увеличиваем массив аргументов
    ast_node_t** new_args = realloc(directive->args, 
                                   (directive->arg_count + 1) * sizeof(ast_node_t*));
    if (!new_args) {
        LOG_ERROR(LOG_PARSER, "Не удалось увеличить массив аргументов директивы");
        return;
    }
    
    directive->args = new_args;
    directive->args[directive->arg_count++] = arg;
    
    // Увеличиваем счетчик ссылок на аргумент
    ast_ref(arg);
}

// Уничтожение директивы
void ast_destroy_directive(ast_node_t* node) {
    if (!node || node->type != AST_DIRECTIVE) return;
    
    ast_directive_t* directive = (ast_directive_t*)node;
    
    // Освобождаем имя директивы
    free(directive->name);
    directive->name = NULL;
    
    // Освобождаем аргументы
    if (directive->args) {
        for (size_t i = 0; i < directive->arg_count; i++) {
            if (directive->args[i]) {
                ast_unref(directive->args[i]);
            }
        }
        free(directive->args);
        directive->args = NULL;
    }
    directive->arg_count = 0;
    
    // Освобождаем сам узел
    free(directive);
}

// Уничтожение операнда
void ast_destroy_operand(ast_node_t* node) {
    if (!node || node->type != AST_OPERAND) return;
    
    ast_operand_t* operand = (ast_operand_t*)node;
    if (operand->type == OPERAND_LABEL && operand->label) {
        free(operand->label);
        operand->label = NULL;
    }
    
    free(operand);
}

// Специализированные функции создания операндов
ast_operand_t* ast_create_immediate_operand(ast_manager_t* manager, token_value_t value) {
    source_loc_t loc = {0, 0}; // Позиция будет установлена позже
    ast_operand_t* operand = ast_create_operand(manager, OPERAND_IMMEDIATE, &loc);
    if (!operand) return NULL;
    
    operand->immediate = value.number;
    return operand;
}

ast_operand_t* ast_create_register_operand(ast_manager_t* manager, token_value_t value) {
    source_loc_t loc = {0, 0}; // Позиция будет установлена позже
    ast_operand_t* operand = ast_create_operand(manager, OPERAND_REGISTER, &loc);
    if (!operand) return NULL;
    
    operand->register_num = value.reg_num;
    return operand;
}

ast_operand_t* ast_create_indirect_operand(ast_manager_t* manager, token_value_t value) {
    source_loc_t loc = {0, 0}; // Позиция будет установлена позже
    ast_operand_t* operand = ast_create_operand(manager, OPERAND_REGISTER, &loc);
    if (!operand) return NULL;
    
    operand->register_num = value.reg_num;
    operand->is_indirect = true;
    return operand;
}

ast_operand_t* ast_create_label_operand(ast_manager_t* manager, const char* label) {
    source_loc_t loc = {0, 0}; // Позиция будет установлена позже
    ast_operand_t* operand = ast_create_operand(manager, OPERAND_LABEL, &loc);
    if (!operand) return NULL;
    
    operand->label = strdup(label);
    if (!operand->label) {
        ast_unref(&operand->base);
        return NULL;
    }
    
    return operand;
}

/*
 * Механизм управления памятью в AST:
 * 
 * 1. Каждый узел AST имеет счетчик сильных ссылок (strong_count):
 *    - При создании узла strong_count = 1 (владелец - менеджер AST)
 *    - При добавлении узла в другой узел (например, операнд в инструкцию) strong_count++
 * 
 * 2. Когда узел уничтожается (strong_count становится 0):
 *    - ast_free_children() рекурсивно уменьшает счетчики ссылок всех дочерних узлов
 *    - ast_destroy_xxx() освобождает специфичные для типа узла ресурсы
 *    - ast_free_resources() освобождает общие ресурсы узла
 * 
 * 3. Менеджер AST отвечает за:
 *    - Хранение списка всех созданных узлов
 *    - Поддержание сильной ссылки на каждый узел
 * 
 * 4. При уничтожении менеджера:
 *    - Проходим по всем узлам в обратном порядке (от последнего к первому)
 *    - Для каждого узла вызываем ast_unref(), уменьшая счетчик ссылок от менеджера
 *    - Когда счетчик достигает 0, узел автоматически освобождает свои ресурсы и дочерние узлы
 * 
 * Пример для дерева "PUSH #42":
 * AST_PROGRAM (strong_count=2: менеджер + родитель)
 * └── AST_INSTRUCTION (strong_count=2: менеджер + program->statements[0])
 *     └── AST_OPERAND (strong_count=2: менеджер + inst->operands[0])
 * 
 * При уничтожении:
 * 1. ast_unref(operand) -> strong_count=1
 * 2. ast_unref(instruction) -> strong_count=1, освобождает операнд
 * 3. ast_unref(program) -> strong_count=1, освобождает инструкцию
 */

// Уничтожение менеджера AST
void ast_manager_destroy(ast_manager_t* manager) {
    if (!manager) return;
    
    LOG_DEBUG(LOG_AST, "=== Начало уничтожения AST менеджера %p ===", (void*)manager);
    LOG_DEBUG(LOG_AST, "Всего узлов: %zu, Выделено: %zu, Освобождено: %zu", 
              manager->node_count, manager->alloc_count, manager->free_count);
    
    // Освобождаем узлы в обратном порядке (от листьев к корню)
    for (size_t i = manager->owned_nodes.count; i > 0; i--) {
        ast_node_t* node = manager->owned_nodes.nodes[i - 1];
        if (!node) continue;
        
        LOG_DEBUG(LOG_AST, "Уменьшаем счетчик ссылок для узла %p типа %d (strong_count=%d)", 
                  (void*)node, node->type, node->refs.strong_count);
        
        // Уменьшаем счетчик ссылок от менеджера, это запустит каскадное освобождение
        ast_unref(node);
    }
    
    // Очищаем массивы
    LOG_DEBUG(LOG_AST, "--- Очистка менеджера ---");
    LOG_DEBUG(LOG_AST, "Освобождение массива owned_nodes (%zu узлов)", manager->owned_nodes.count);
    free(manager->owned_nodes.nodes);
    LOG_DEBUG(LOG_AST, "Освобождение массива weak_refs (%zu ссылок)", manager->weak_refs.count);
    free(manager->weak_refs.nodes);
    
    // Освобождаем сам менеджер
    LOG_DEBUG(LOG_AST, "=== Завершение уничтожения AST менеджера %p ===", (void*)manager);
    free(manager);
}

// Уничтожение программы
void ast_destroy_program(ast_node_t* node) {
    if (!node || node->type != AST_PROGRAM) return;
    
    ast_program_t* program = (ast_program_t*)node;
    
    // Освобождаем все операторы
    if (program->statements) {
        for (size_t i = 0; i < program->statement_count; i++) {
            if (program->statements[i]) {
                ast_unref(program->statements[i]);
            }
        }
        free(program->statements);
        program->statements = NULL;
    }
    
    program->statement_count = 0;
    program->statement_capacity = 0;
}

// Уничтожение метки
void ast_destroy_label(ast_node_t* node) {
    if (!node || node->type != AST_LABEL) return;
    
    ast_label_t* label = (ast_label_t*)node;
    if (label->name) {
        free(label->name);
        label->name = NULL;
    }
}

// Уничтожение инструкции
void ast_destroy_instruction(ast_node_t* node) {
    if (!node || node->type != AST_INSTRUCTION) return;
    
    ast_instruction_t* inst = (ast_instruction_t*)node;
    
    // Освобождаем мнемонику
    if (inst->mnemonic) {
        free(inst->mnemonic);
        inst->mnemonic = NULL;
    }
    
    // Освобождаем операнды
    if (inst->operands) {
        for (size_t i = 0; i < inst->operand_count; i++) {
            if (inst->operands[i]) {
                ast_unref(&inst->operands[i]->base);
            }
        }
        free(inst->operands);
        inst->operands = NULL;
    }
    
    inst->operand_count = 0;
} 