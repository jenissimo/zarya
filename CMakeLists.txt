cmake_minimum_required(VERSION 3.10)
project(zarya VERSION 0.1.0 LANGUAGES C)

# Установка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Опции сборки
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_WARNINGS "Enable warnings" ON)
#option(ENABLE_ASAN "Enable Address Sanitizer" ON)

# Установка флагов компилятора
if(ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options(/W4 /Zi)
        add_link_options(/DEBUG)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic -g -O0)
    endif()
endif()

# Добавляем Address Sanitizer
if(ENABLE_ASAN)
    if(NOT MSVC)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Установка путей для включаемых файлов
include_directories(${PROJECT_SOURCE_DIR}/include)

# Создаем библиотеку с общим кодом
add_library(zarya_lib STATIC
    src/common/trit_ops.c
    src/vm/zarya_vm.c
    src/vm/instructions.c
    src/vm/stack.c
    src/emulator.c
)

target_include_directories(zarya_lib PUBLIC include src/vm)

# Библиотека общих компонентов
add_library(common_lib
    src/common/logging.c
)

target_include_directories(common_lib PUBLIC src/common)

# Библиотека ТРИАС
add_library(trias_lib
    src/trias/lexer.c
    src/trias/trias_instructions.c
    src/trias/parser.c
    src/trias/ast.c
    src/trias/symbol_table.c
)

target_link_libraries(trias_lib PRIVATE common_lib)
target_include_directories(trias_lib PUBLIC src/trias)

add_executable(zarya_emu
    src/tui_emulator.c
)
target_link_libraries(zarya_emu PRIVATE zarya_lib)

add_executable(zarya_cli
    src/cli_emulator.c
)
target_link_libraries(zarya_cli PRIVATE zarya_lib trias_lib)
target_include_directories(zarya_cli PRIVATE 
    include 
    src/trias
    src/vm
)

# Добавляем определение для компактного вывода тестов
if(DEFINED ENV{TEST_OUTPUT_COMPACT})
    add_compile_definitions(TEST_OUTPUT_COMPACT)
endif()

# Настройка тестирования
if(BUILD_TESTS)
    enable_testing()
    include(CTest)

    # Добавляем Unity как библиотеку
    add_library(unity STATIC
        external/unity/src/unity.c
    )
    target_include_directories(unity PUBLIC
        external/unity/src
    )

    # Функция для упрощения добавления тестов
    function(add_zarya_test TEST_NAME TEST_SOURCE)
        add_executable(${TEST_NAME} ${TEST_SOURCE})
        target_link_libraries(${TEST_NAME} PRIVATE unity trias_lib zarya_lib)
        target_include_directories(${TEST_NAME} PRIVATE 
            include 
            src
            src/trias
            src/vm
            src/common
        )
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endfunction()

    # Добавление тестов
    # Тесты виртуальной машины
    add_zarya_test(test_vm tests/vm/test_vm.c)
    add_zarya_test(test_stack_instructions tests/vm/instructions/test_stack_instructions.c)
    add_zarya_test(test_arithmetic_instructions tests/vm/instructions/test_arithmetic_instructions.c)
    add_zarya_test(test_logical_instructions tests/vm/instructions/test_logical_instructions.c)
    add_zarya_test(test_memory_instructions tests/vm/instructions/test_memory_instructions.c)
    add_zarya_test(test_comparison_instructions tests/vm/instructions/test_comparison_instructions.c)
    add_zarya_test(test_control_instructions tests/vm/instructions/test_control_instructions.c)
    add_zarya_test(test_interrupt_instructions tests/vm/instructions/test_interrupt_instructions.c)

    # Тесты ассемблера ТРИАС
    add_zarya_test(test_trias tests/common/test_trias.c)
    add_zarya_test(test_lexer tests/trias/test_lexer.c)
    add_zarya_test(test_lexer_comments tests/trias/test_lexer_comments.c)
    add_zarya_test(test_lexer_numbers tests/trias/test_lexer_numbers.c)
    add_zarya_test(test_parser tests/trias/test_parser.c)
    add_zarya_test(test_parser_instructions tests/trias/test_parser_instructions.c)
    add_zarya_test(test_symbol_table tests/trias/test_symbol_table.c)

    # Общие тесты
    add_zarya_test(test_trits tests/common/test_trits.c)
endif() 