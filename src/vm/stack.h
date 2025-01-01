#ifndef ZARYA_STACK_H
#define ZARYA_STACK_H

#include <stdbool.h>
#include "zarya_vm.h"

// Проверка состояния стека
bool stack_is_empty(const vm_state_t* vm);
bool stack_is_full(const vm_state_t* vm);

// Операции со стеком
vm_error_t stack_push(vm_state_t* vm, tryte_t value);
vm_error_t stack_pop(vm_state_t* vm, tryte_t* value);
vm_error_t stack_dup(vm_state_t* vm);
vm_error_t stack_swap(vm_state_t* vm);

#endif // ZARYA_STACK_H 