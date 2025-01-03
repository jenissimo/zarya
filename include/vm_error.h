#ifndef VM_ERROR_H
#define VM_ERROR_H

// Коды ошибок виртуальной машины и ассемблера
typedef enum {
    VM_OK = 0,                        // Нет ошибки
    
    // Общие ошибки
    VM_ERROR_INVALID_PARAMETER,       // Неверный параметр
    VM_ERROR_OUT_OF_MEMORY,          // Нехватка памяти
    VM_ERROR_INVALID_ADDRESS,        // Неверный адрес
    VM_ERROR_INVALID_OPCODE,         // Неверный опкод
    VM_ERROR_STACK_OVERFLOW,         // Переполнение стека
    VM_ERROR_STACK_UNDERFLOW,        // Опустошение стека
    VM_ERROR_DIVISION_BY_ZERO,       // Деление на ноль
    VM_ERROR_HALT,                   // Остановка программы (не ошибка)
    
    // Ошибки ассемблера
    VM_ERROR_SYNTAX_ERROR,           // Синтаксическая ошибка
    VM_ERROR_INVALID_OPERAND,        // Неверный операнд
    VM_ERROR_INVALID_REGISTER,       // Неверный регистр
    VM_ERROR_INVALID_NUMBER,         // Неверное число
    VM_ERROR_INVALID_LABEL,          // Неверная метка
    VM_ERROR_INVALID_DIRECTIVE,      // Неверная директива
    
    // Ошибки таблицы символов
    VM_ERROR_SYMBOL_NOT_FOUND,       // Символ не найден
    VM_ERROR_SYMBOL_ALREADY_DEFINED, // Символ уже определён
    VM_ERROR_INVALID_SCOPE,          // Неверная область видимости
    
    // Ошибки макросов
    VM_ERROR_MACRO_NOT_FOUND,        // Макрос не найден
    VM_ERROR_MACRO_ALREADY_DEFINED,  // Макрос уже определён
    VM_ERROR_MACRO_TOO_MANY_ARGS,    // Слишком много аргументов
    VM_ERROR_MACRO_TOO_FEW_ARGS,     // Слишком мало аргументов
    
    // Ошибки компоновщика
    VM_ERROR_UNRESOLVED_SYMBOL,      // Неразрешённый символ
    VM_ERROR_DUPLICATE_SYMBOL,       // Дублирующийся символ
    VM_ERROR_INVALID_RELOCATION,     // Неверная перемещаемая ссылка
    
    // Ошибки файлового ввода-вывода
    VM_ERROR_FILE_NOT_FOUND,         // Файл не найден
    VM_ERROR_FILE_ACCESS_DENIED,     // Доступ к файлу запрещён
    VM_ERROR_FILE_READ_ERROR,        // Ошибка чтения файла
    VM_ERROR_FILE_WRITE_ERROR,       // Ошибка записи файла
    
    VM_ERROR_COUNT                   // Количество кодов ошибок
} vm_error_t;

// Получение текстового описания ошибки
const char* vm_error_to_string(vm_error_t error);

#endif // VM_ERROR_H 