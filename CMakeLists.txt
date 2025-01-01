cmake_minimum_required(VERSION 3.10)
project(zarya VERSION 0.1.0 LANGUAGES C)

# Установка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Опции сборки
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_WARNINGS "Enable warnings" ON)
option(ENABLE_ASAN "Enable Address Sanitizer" ON)

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

# Библиотека ассемблера ТРИАС
add_library(trias_lib
    src/trias/lexer.c
    src/trias/parser.c
    src/trias/codegen.c
    src/trias/trias_instructions.c
    src/trias/ast.c
)

target_link_libraries(trias_lib PRIVATE zarya_lib)
target_include_directories(trias_lib PUBLIC include src/trias)

add_executable(trias_assembler
    src/trias/main.c
)
target_link_libraries(trias_assembler PRIVATE trias_lib zarya_lib)

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

    # Добавляем общую библиотеку для тестов
    add_library(test_common STATIC
        tests/test_common.c
    )
    target_link_libraries(test_common PRIVATE unity zarya_lib)
    target_include_directories(test_common PUBLIC tests)

    # Тест ассемблера ТРИАС
    add_executable(test_trias tests/test_trias.c)
    target_link_libraries(test_trias PRIVATE unity trias_lib zarya_lib test_common)
    target_include_directories(test_trias PRIVATE 
        include 
        src
        src/trias
        src/vm
    )
    add_test(NAME test_trias COMMAND test_trias)
endif() 