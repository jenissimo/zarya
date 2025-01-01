# Обновленная документация и система команд для компьютера "Заря"

## Соглашения по коду и рекомендации по использованию

### Таксономия типов данных

#### Базовые типы

1. **Трит (`trit_t`)**
   - Базовая единица данных
   - Представлен как `int8_t`
   - Допустимые значения: -1, 0, 1
   - Константы: `TRIT_NEGATIVE`, `TRIT_ZERO`, `TRIT_POSITIVE`
   - Проверка корректности: использовать функцию `is_valid_trit(trit_t)`

2. **Трайт (`tryte_t`)**
   - Состоит из 6 тритов
   - Структура с массивом `trits[TRITS_PER_TRYTE]`
   - Дополнительно хранит целочисленный эквивалент `int32_t` для ускорения операций
   - Конструктор: `create_tryte_from_int(int value)`

3. **Машинное слово (`word_t`)**
   - Состоит из 18 тритов
   - Структура с массивом `trits[TRITS_PER_WORD]`
   - Дополнительно хранит целочисленный эквивалент `int64_t` для быстрого доступа
   - Конструктор: `create_word_from_int(int value)`

#### Порядок хранения

- Триты хранятся в порядке от младшего к старшему.
- Важно: порядок сохраняется одинаковым для `tryte_t` и `word_t` для унификации.

### Работа с памятью

#### Адресация

- Линейное адресное пространство
- Размер памяти: 65536 трайтов
- Адресация: помарочная (1 трайт = 1 ячейка)

#### Выравнивание

- Трайты должны быть выровнены по границе 6 тритов
- Слова должны быть выровнены по границе 18 тритов
- При нарушении выравнивания вызывается ошибка `ERROR_UNALIGNED_ACCESS`

#### Функции работы с памятью

- `memory_read(address)`
- `memory_write(address, value)`
- Все операции проверяют границы памяти и корректность адресации.

### Стеки

#### Стек данных

- Используется для хранения операндов
- Растет вниз
- Единица хранения: `tryte_t`
- Операции:
  - `stack_push(value)`
  - `stack_pop()`
  - `stack_dup()`
  - `stack_swap()`

#### Стек вызовов

- Используется для адресов возврата
- Растет вниз
- Единица хранения: `word_t`
- Операции:
  - `call(address)`
  - `return()`

### Флаги

#### CPU-флаги

- `zero`: результат равен нулю (0: не равно, +1: равно)
- `negative`: знак результата (-1: отрицательное, +1: положительное)
- `overflow`: переполнение (-1: недополнение, +1: переполнение)
- `carry`: перенос (-1: заем, +1: перенос)

#### VM-флаги

- `FLAG_Z`: нулевой результат
- `FLAG_N`: отрицательный результат
- `FLAG_O`: переполнение
- `FLAG_I`: прерывания разрешены

#### Примеры работы с флагами

При сложении двух трайтов:
```c
tryte_t result = add_tryte(a, b);
set_flag_zero(result == TRIT_ZERO);
set_flag_negative(result < TRIT_ZERO);
```

### Соглашения по коду

#### Именование

- Функции: snake_case
- Типы: snake_case с суффиксом _t
- Константы: UPPER_SNAKE_CASE
- Структуры: snake_case

#### Комментарии

- Использовать стиль `/* */` для многострочных комментариев
- Разрешены однострочные комментарии `//`

#### Форматирование

- Отступы: 4 пробела
- Открывающая скобка на той же строке
- Пробел после if, for, while
- Пробелы вокруг операторов

#### Коды ошибок

- Все ошибки перечислены в `errors.h`
- Примеры:
  - `ERROR_OUT_OF_MEMORY`
  - `ERROR_INVALID_ADDRESS`
  - `ERROR_UNALIGNED_ACCESS`

### Рекомендации по использованию

1. **Проверка значений**
   - Использовать функции `is_valid_trit`, `is_valid_tryte` для проверки корректности данных.
   - Проверять указатели на NULL перед использованием.

2. **Работа с памятью**
   - Использовать функции `memory_read` и `memory_write` вместо прямого обращения к памяти.
   - Проверять границы памяти перед записью/чтением.

3. **Примеры кода**

```c
tryte_t a = create_tryte_from_int(15);
tryte_t b = create_tryte_from_int(-3);
tryte_t result = add_tryte(a, b);
if (result == TRIT_ZERO) {
    // Обработка нулевого результата
}
```

### Расширение функциональности

#### Добавление инструкций

1. Определить опкод в `instructions.h`.
2. Реализовать логику в `instructions.c`.
3. Добавить инструкцию в таблицу диспетчера.
4. Обновить документацию с описанием новой инструкции.

#### Добавление регистров

1. Увеличить константу `CPU_NUM_REGISTERS`.
2. Добавить новые регистры в `cpu_registers.h`.
3. Обновить функции работы с регистрами.

#### Добавление флагов

1. Определить флаг в `flags.h`.
2. Обновить функции установки/проверки флагов.
3. Добавить примеры использования в документацию.

### Примеры использования системы команд

#### Пример: сложение двух трайтов

```assembly
LOAD R1, [100]     // Загрузить значение из памяти в регистр R1
LOAD R2, [101]     // Загрузить значение из памяти в регистр R2
ADD R1, R2         // Сложить R1 и R2, результат в R1
STORE R1, [102]    // Сохранить результат обратно в память
```

#### Пример: вызов подпрограммы

```assembly
CALL [200]         // Вызвать подпрограмму по адресу 200
...                // Код подпрограммы
RETURN             // Вернуться в точку вызова
```

### Графика через прерывания

#### Специализированные регистры для графики

1. **VR (Video Register)**
   - Назначение: хранит указатель на текущий видеобуфер.
   - Тип данных: `word_t` (указатель на память).

2. **CR (Control Register)**
   - Назначение: управление режимами CPU, включая настройку графики (разрешение, глубина цвета и т.д.).
   - Пример битового назначения:
     - 0-2: режим графики (текстовый, графический, смешанный);
     - 3-4: глубина цвета;
     - 5: флаг вертикальной синхронизации (вкл/выкл).

3. **IR (Interrupt Register)**
   - Назначение: указывает текущий тип прерывания.
   - Примеры:
     - 0: нет прерывания;
     - 1: таймер;
     - 2: ввод с клавиатуры;
     - 3: обновление видеобуфера.

4. **SR (Sprite Register)**
   - Назначение: используется для указания текущего спрайта в видеопамяти.
   - Ускоряет операции с графическими объектами, такими как перемещение, отрисовка и коллизии.

#### Типы графических прерываний

1. **VBLANK (Вертикальная синхронизация)**
   - Условие: вызывается после завершения отрисовки кадра.
   - Используется для обновления графики в буфере и синхронизации анимации.

2. **DRAW (Запрос на отрисовку)**
   - Условие: вызывается при отправке данных в видеобуфер.
   - Используется для отрисовки графических примитивов (линии, пиксели, спрайты).

3. **HBLANK (Горизонтальная синхронизация)**
   - Условие: вызывается после завершения строки.
   - Используется для построчной обработки изображений и эффектов.

4. **VSYNC (Синхронизация кадров)**
   - Условие: вызывается для предотвращения разрывов изображения.
   - Используется для обеспечения плавной графики в играх.

#### Примеры взаимодействия с графикой

1. **Инициализация видеорежима:**
```assembly
LOAD VR, [VIDEO_BUFFER]    // Указать начальный адрес видеобуфера
LOAD CR, 0x03              // Установить графический режим (например, 640x480, 16 цветов)
```

2. **Прерывание VBLANK для обновления кадра:**
```assembly
VBLANK_HANDLER:
    CALL update_sprites       // Обновить позиции спрайтов
    CALL draw_frame           // Переключить видеобуферы
    IRET                     // Вернуться из прерывания
```

3. **Рисование спрайта:**
```assembly
LOAD SR, [SPRITE_DATA]      // Указать текущий спрайт
DRAW SR, [X_COORD], [Y_COORD] // Нарисовать спрайт по указанным координатам
```

### Структура системы команд

#### Формат машинного слова

1. **Длина команды**: 18 тритов (1 машинное слово).
2. **Структура:**
   - Опкод: 6 тритов (до 729 уникальных команд).
   - Операнд 1: 6 тритов (регистры, адреса, константы).
   - Операнд 2/Дополнительные данные: 6 тритов (регистры, адреса или флаги).

#### Пример кодировки

- **ADD R1, R2**:
  - Опкод (ADD): `001 001`
  - Операнд 1 (R1): `010 000`
  - Операнд 2 (R2): `010 001`

#### Расширенные команды

Для операций с большими данными используются команды из нескольких машинных слов:

- **LOADI R1, #42**:
  - Первое слово: Опкод и регистр (`LOADI R1`).
  - Второе слово: Значение (#42).

#### Основные категории команд

1. **Арифметические команды**
   - ADD R1, R2: Сложение значений.
   - SUB R1, R2: Вычитание.
   - MUL R1, R2: Умножение.
   - DIV R1, R2: Деление.

2. **Логические команды**
   - AND R1, R2: Потритовое И.
   - OR R1, R2: Потритовое ИЛИ.
   - NOT R1: Потритовое НЕ.

3. **Управление потоком**
   - JMP addr: Переход на адрес.
   - JZ addr: Переход, если флаг zero установлен.
   - JNZ addr: Переход, если флаг zero не установлен.
   - JG addr: Переход, если значение положительное.
   - JL addr: Переход, если значение отрицательное.
   - CALL addr: Вызов подпрограммы.
   - RET: Возврат из подпрограммы.

4. **Работа с памятью**
   - LOAD R1, [addr]: Загрузка в регистр.
   - STORE R1, [addr]: Сохранение из регистра.

5. **Прерывания**
   - INT type: Вызов прерывания.

### Использование ПОЛИЗ (постфиксная запись)

#### Обоснование использования

Постфиксная запись (ПОЛИЗ) используется для упрощения парсинга выражений и эффективного выполнения вычислений. Она исключает необходимость скобок и строгого порядка операций, что упрощает работу с выражениями в троичной системе.

#### Преобразование выражений в ПОЛИЗ

1. **Инициальное выражение**:
   `(A + B) * C`

2. **Преобразование в ПОЛИЗ**:
   `A B + C *`

#### Выполнение ПОЛИЗ с помощью стека

1. **Пошаговая обработка**:
   - Поместить операнд `A` в стек.
   - Поместить операнд `B` в стек.
   - Выполнить операцию `+` над верхними элементами стека и вернуть результат в стек.
   - Поместить операнд `C` в стек.
   - Выполнить операцию `*` над верхними элементами стека и вернуть результат в стек.

2. **Пример выполнения:**
```assembly
PUSH A
PUSH B
ADD        // Сложение верхних элементов стека
PUSH C
MUL        // Умножение верхних элементов стека
```

3. **Результат**: Значение `((A + B) * C)` остаётся на вершине стека.

#### Интеграция в систему команд

- Команды `PUSH` и `POP` используются для работы со стеком.
- Арифметические и логические команды (`ADD`, `MUL`, `AND`, и т.д.) автоматически работают с верхними элементами стека.
