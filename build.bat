@echo off
setlocal

REM Проверяем наличие WSL
wsl --list >nul 2>&1
if errorlevel 1 (
    echo ERROR: WSL не установлен
    exit /b 1
)

REM Проверяем аргументы
set BUILD_TYPE=Debug
set CLEAN_BUILD=0

:parse_args
if "%1"=="" goto done_args
if /i "%1"=="release" set BUILD_TYPE=Release
if /i "%1"=="clean" set CLEAN_BUILD=1
shift
goto parse_args
:done_args

REM Создаем директорию build если её нет или очищаем если указан флаг clean
if %CLEAN_BUILD%==1 (
    if exist build rmdir /s /q build
)
if not exist build mkdir build

REM Получаем текущий путь в формате WSL
for %%I in (.) do set "CURRENT_DIR=%%~fI"
set "WSL_PATH=/mnt/c%CURRENT_DIR:~2%"
set "WSL_PATH=%WSL_PATH:\=/%"

echo WSL путь: %WSL_PATH%

REM Запускаем сборку через WSL
echo Настройка проекта через CMake...
wsl cd "%WSL_PATH%" ^&^& cmake -B build -S . -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

echo Сборка проекта...
wsl cd "%WSL_PATH%" ^&^& cmake --build build

if errorlevel 1 (
    echo ERROR: Ошибка при сборке
    exit /b 1
)

echo Сборка успешно завершена
endlocal 