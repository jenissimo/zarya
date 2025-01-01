@echo off
setlocal

REM Проверяем наличие WSL
wsl --list >nul 2>&1
if errorlevel 1 (
    echo ERROR: WSL не установлен
    exit /b 1
)

REM Проверяем наличие входного файла
if "%~1"=="" (
    echo ERROR: Не указан входной файл
    echo Использование: trias.bat input.tri [output.tro]
    exit /b 1
)

REM Проверяем существование входного файла
if not exist "%~1" (
    echo ERROR: Файл %~1 не найден
    exit /b 1
)

REM Определяем выходной файл
set "INPUT_FILE=%~1"
set "OUTPUT_FILE=%~2"
if "%OUTPUT_FILE%"=="" (
    for %%I in ("%INPUT_FILE%") do set "OUTPUT_FILE=%%~dpI%%~nI.tro"
)

REM Получаем пути в формате WSL
for %%I in ("%INPUT_FILE%") do set "WSL_INPUT=/mnt/c%%~pnxI"
for %%I in ("%OUTPUT_FILE%") do set "WSL_OUTPUT=/mnt/c%%~pnxI"
set "WSL_INPUT=%WSL_INPUT:\=/%"
set "WSL_OUTPUT=%WSL_OUTPUT:\=/%"

echo Компиляция %INPUT_FILE% в %OUTPUT_FILE%...

REM Запускаем ассемблер через WSL
wsl cd /mnt/c/Projects/zarya ^&^& ./build/trias_assembler -o "%WSL_OUTPUT%" "%WSL_INPUT%"

if errorlevel 1 (
    echo ERROR: Ошибка при компиляции
    exit /b 1
)

echo Компиляция успешно завершена
endlocal 