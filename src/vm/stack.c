#include "stack.h"
#include "zarya_config.h"
#include <stdio.h>

bool stack_is_empty(const vm_state_t* vm) {
    if (!vm) return true;
    bool empty = vm->sp.value < 0;
    printf("DEBUG: stack_is_empty: sp=%d, empty=%d\n", vm->sp.value, empty);
    return empty;
}

bool stack_is_full(const vm_state_t* vm) {
    if (!vm) return true;
    bool full = (size_t)(vm->sp.value + 1) >= vm->memory_size;
    printf("DEBUG: stack_is_full: sp=%d, memory_size=%zu, full=%d\n", 
           vm->sp.value, vm->memory_size, full);
    return full;
}

vm_error_t stack_push(vm_state_t* vm, tryte_t value) {
    if (!vm) return VM_ERROR_INVALID_ADDRESS;
    
    printf("DEBUG: stack_push: trying to push value=%d, current_sp=%d\n", 
           value.value, vm->sp.value);
    
    if (stack_is_full(vm)) {
        fprintf(stderr, "STACK OVERFLOW: sp=%d, memory_size=%zu\n", 
                vm->sp.value, vm->memory_size);
        return VM_ERROR_STACK_OVERFLOW;
    }
    
    int new_sp = vm->sp.value + 1;
    if (new_sp < 0 || (size_t)new_sp >= vm->memory_size) {
        fprintf(stderr, "DEBUG: stack_push: invalid new address sp=%d, memory_size=%zu\n", 
                new_sp, vm->memory_size);
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    vm->sp = TRYTE_FROM_INT(new_sp);
    printf("DEBUG: stack_push: new sp=%d\n", vm->sp.value);
    
    vm->memory[vm->sp.value] = value;
    printf("DEBUG: stack_push: value=%d pushed at sp=%d\n", 
           TRYTE_GET_VALUE(value), vm->sp.value);
    
    return VM_OK;
}

vm_error_t stack_pop(vm_state_t* vm, tryte_t* value) {
    if (!vm || !value) return VM_ERROR_INVALID_ADDRESS;
    
    printf("DEBUG: stack_pop: trying to pop from sp=%d\n", vm->sp.value);
    
    if (stack_is_empty(vm)) {
        fprintf(stderr, "STACK UNDERFLOW: sp=%d\n", vm->sp.value);
        return VM_ERROR_STACK_UNDERFLOW;
    }
    
    if (vm->sp.value < 0 || (size_t)vm->sp.value >= vm->memory_size) {
        fprintf(stderr, "DEBUG: stack_pop: invalid address sp=%d, memory_size=%zu\n", 
                vm->sp.value, vm->memory_size);
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    *value = vm->memory[vm->sp.value];
    printf("DEBUG: stack_pop: value=%d popped from sp=%d\n", 
           TRYTE_GET_VALUE(*value), vm->sp.value);
    
    vm->sp = TRYTE_FROM_INT(vm->sp.value - 1);
    printf("DEBUG: stack_pop: new sp=%d\n", vm->sp.value);
    
    return VM_OK;
}

vm_error_t stack_dup(vm_state_t* vm) {
    printf("DEBUG: stack_dup: current sp=%d\n", vm->sp.value);
    
    if (stack_is_empty(vm)) {
        fprintf(stderr, "STACK DUP ERROR: stack is empty, sp=%d\n", vm->sp.value);
        return VM_ERROR_STACK_UNDERFLOW;
    }
    if (stack_is_full(vm)) {
        fprintf(stderr, "STACK DUP ERROR: stack is full, sp=%d\n", vm->sp.value);
        return VM_ERROR_STACK_OVERFLOW;
    }
    
    if (vm->sp.value < 0 || (size_t)vm->sp.value >= vm->memory_size) {
        fprintf(stderr, "DEBUG: stack_dup: invalid address sp=%d, memory_size=%zu\n", 
                vm->sp.value, vm->memory_size);
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    tryte_t value = vm->memory[vm->sp.value];
    printf("DEBUG: stack_dup: duplicating value=%d from sp=%d\n", 
           TRYTE_GET_VALUE(value), vm->sp.value);
    return stack_push(vm, value);
}

vm_error_t stack_swap(vm_state_t* vm) {
    printf("DEBUG: stack_swap: current sp=%d\n", vm->sp.value);
    
    if (vm->sp.value < 1) {
        fprintf(stderr, "STACK SWAP ERROR: insufficient items, sp=%d\n", vm->sp.value);
        return VM_ERROR_STACK_UNDERFLOW;
    }
    
    if (vm->sp.value < 0 || (size_t)vm->sp.value >= vm->memory_size ||
        vm->sp.value - 1 < 0 || (size_t)(vm->sp.value - 1) >= vm->memory_size) {
        fprintf(stderr, "DEBUG: stack_swap: invalid address sp=%d, memory_size=%zu\n", 
                vm->sp.value, vm->memory_size);
        return VM_ERROR_INVALID_ADDRESS;
    }
    
    tryte_t temp = vm->memory[vm->sp.value];
    vm->memory[vm->sp.value] = vm->memory[vm->sp.value - 1];
    vm->memory[vm->sp.value - 1] = temp;
    printf("DEBUG: stack_swap: swapped values %d <-> %d at sp=%d and sp-1=%d\n", 
           TRYTE_GET_VALUE(temp), TRYTE_GET_VALUE(vm->memory[vm->sp.value]), 
           vm->sp.value, vm->sp.value - 1);
    return VM_OK;
}