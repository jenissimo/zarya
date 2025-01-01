@echo off
setlocal

REM Проверяем наличие WSL
wsl --list >nul 2>&1
if errorlevel 1 (
    echo ERROR: WSL не установлен
    exit /b 1
)

REM Проверяем наличие собранного файла
if not exist build\zarya (
    echo ERROR: Программа не собрана. Сначала выполните build.bat
    exit /b 1
)

REM Получаем текущий путь в формате WSL
for %%I in (.) do set "CURRENT_DIR=%%~fI"
set "WSL_PATH=/mnt/c%CURRENT_DIR:~2%"
set "WSL_PATH=%WSL_PATH:\=/%"

echo WSL путь: %WSL_PATH%

REM Запускаем программу через WSL
echo Запуск Заря...
wsl cd "%WSL_PATH%/build" ^&^& ./zarya %*

if errorlevel 1 (
    echo ERROR: Ошибка при выполнении
    exit /b 1
)

endlocal 