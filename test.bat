@echo off
setlocal

REM Проверяем наличие WSL
wsl --list >nul 2>&1
if errorlevel 1 (
    echo ERROR: WSL не установлен
    exit /b 1
)

REM Проверяем наличие директории сборки
if not exist build (
    echo ERROR: Проект не собран. Сначала выполните build.bat
    exit /b 1
)

REM Получаем текущий путь в формате WSL
for %%I in (.) do set "CURRENT_DIR=%%~fI"
set "WSL_PATH=/mnt/c%CURRENT_DIR:~2%"
set "WSL_PATH=%WSL_PATH:\=/%"

echo WSL путь: %WSL_PATH%

REM Запускаем тесты через WSL
echo Запуск тестов...
wsl cd "%WSL_PATH%/build" ^&^& ctest -j1 --output-on-failure %*

if errorlevel 1 (
    echo ERROR: Некоторые тесты не прошли
    exit /b 1
)

echo Все тесты успешно пройдены
endlocal 