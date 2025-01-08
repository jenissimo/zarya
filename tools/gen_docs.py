#!/usr/bin/python3

import re
import os

# Словарь режимов адресации
ADDR_MODES = {
    'ADDR_MODE_NONE': 0,      # Нет операндов
    'ADDR_MODE_IMM': 1,       # Непосредственный режим
    'ADDR_MODE_REG': 2,       # Регистровый режим
    'ADDR_MODE_IND': 4,       # Косвенный режим
    'ADDR_MODE_ALL': 7,       # Все режимы (IMM | REG | IND)
    'ADDR_MODE_REG_IND': 6,   # Регистровый и косвенный (REG | IND)
    'ADDR_MODE_IMM_REG': 3    # Непосредственный и регистровый (IMM | REG)
}

def get_addr_mode_desc(mode_name):
    mode = ADDR_MODES.get(mode_name, 0)
    modes = []
    if mode == 0:
        return "Нет"
    if mode & 1:  # ADDR_MODE_IMM
        modes.append("Непосредственный (#value)")
    if mode & 2:  # ADDR_MODE_REG
        modes.append("Регистровый (Rn)")
    if mode & 4:  # ADDR_MODE_IND
        modes.append("Косвенный (@Rn)")
    return ", ".join(modes)

def parse_instruction_table(content):
    # Ищем таблицу инструкций
    match = re.search(r'static\s+const\s+instruction_info_t\s+instruction_table\[\]\s*=\s*{(.*?)};', content, re.DOTALL)
    if not match:
        raise Exception("Таблица инструкций не найдена")
    
    table_content = match.group(1)
    instructions = []
    
    # Разбираем каждую запись в таблице
    for entry in re.finditer(r'{\s*"([^"]+)",\s*OP_(\w+),\s*(\d+),\s*"([^"]+)",\s*"([^"]+)",\s*(\w+)\s*}', table_content):
        name, op_name, operands, desc, group, addr_mode = entry.groups()
        
        # Ищем значение опкода в enum
        op_match = re.search(rf'OP_{op_name}\s*=\s*(\d+)', content)
        if not op_match:
            print(f"Warning: не найдено значение для OP_{op_name}")
            continue
            
        value = int(op_match.group(1))
        instructions.append({
            'name': name,
            'value': value,
            'operands': int(operands),
            'desc': desc,
            'group': group,
            'addr_mode': addr_mode
        })
    
    return instructions

def generate_markdown(instructions):
    md = []
    md.append('# Система команд виртуальной машины Заря\n')
    
    # Общие сведения
    md.append('## Общие сведения\n')
    md.append('Виртуальная машина Заря использует стековую архитектуру с троичной логикой. ')
    md.append('Все операции выполняются над значениями в стеке, за исключением инструкций, ')
    md.append('которые явно работают с регистрами или памятью.\n\n')
    
    # Режимы адресации
    md.append('## Режимы адресации\n')
    md.append('Машина поддерживает следующие режимы адресации операндов:\n')
    md.append('1. Непосредственный (#value):\n')
    md.append('   ```asm\n')
    md.append('   PUSH #42    ; Положить число 42 в стек\n')
    md.append('   MOV R0, #5  ; Записать 5 в R0\n')
    md.append('   ```\n\n')
    md.append('2. Регистровый (Rn):\n')
    md.append('   ```asm\n')
    md.append('   PUSH R0     ; Положить значение из R0 в стек\n')
    md.append('   MOV R1, R0  ; Копировать из R0 в R1\n')
    md.append('   ```\n\n')
    md.append('3. Косвенный через регистр (@Rn):\n')
    md.append('   ```asm\n')
    md.append('   PUSH @R0    ; Положить значение из памяти по адресу в R0\n')
    md.append('   MOV R1, @R0 ; Загрузить в R1 значение из памяти по адресу в R0\n')
    md.append('   ```\n\n')
    
    # Группируем инструкции по группам
    groups = {}
    for inst in instructions:
        group = inst['group']
        if group not in groups:
            groups[group] = []
        groups[group].append(inst)
    
    # Базовые инструкции
    md.append('## Базовые инструкции\n')
    basic_groups = ['Системные', 'Стек', 'Арифметика', 'Логика', 'Сравнение', 'Управление', 'Память']
    for group in basic_groups:
        if group not in groups:
            continue
        md.append(f'### {group}\n')
        md.append('| Мнемоника | Опкод | Операнды | Режимы адресации | Описание |\n')
        md.append('|-----------|--------|----------|------------------|----------|\n')
        
        for inst in sorted(groups[group], key=lambda x: x['value']):
            addr_modes = get_addr_mode_desc(inst['addr_mode'])
            md.append(f"| {inst['name']} | {inst['value']} | {inst['operands']} | {addr_modes} | {inst['desc']} |\n")
        md.append('\n')
    
    # Псевдоинструкции
    md.append('## Псевдоинструкции\n')
    md.append('Псевдоинструкции предоставляют более удобный синтаксис для часто используемых операций. ')
    md.append('Каждая псевдоинструкция транслируется в последовательность базовых инструкций.\n\n')
    
    if 'Псевдоинструкции' in groups:
        md.append('| Мнемоника | Операнды | Режимы адресации | Описание | Трансляция |\n')
        md.append('|-----------|----------|------------------|-----------|------------|\n')
        
        translations = {
            'MOV': 'PUSH src; POP dst',
            'INC': 'PUSH reg; PUSH 1; ADD; POP reg',
            'DEC': 'PUSH reg; PUSH 1; SUB; POP reg',
            'PUSHR': 'PUSH reg',
            'POPR': 'POP reg',
            'CLEAR': 'DROP × n',
            'CMP': 'PUSH a; PUSH b; SUB',
            'TEST': 'PUSH val; DUP; AND'
        }
        
        for inst in sorted(groups['Псевдоинструкции'], key=lambda x: x['value']):
            addr_modes = get_addr_mode_desc(inst['addr_mode'])
            translation = translations.get(inst['name'], '')
            md.append(f"| {inst['name']} | {inst['operands']} | {addr_modes} | {inst['desc']} | {translation} |\n")
        md.append('\n')
    
    # Формат инструкций
    md.append('## Формат инструкций\n\n')
    md.append('Каждая инструкция состоит из одного или нескольких трайтов:\n')
    md.append('1. Опкод (1 трайт)\n')
    md.append('2. Операнды (0-2 трайта)\n\n')
    md.append('### Структура опкода\n')
    md.append('```\n')
    md.append('+-------------+----------------+\n')
    md.append('| Режим адр.  | Базовый опкод |\n')
    md.append('| (1 трит)    | (остальные)   |\n')
    md.append('+-------------+----------------+\n')
    md.append('```\n')
    
    return ''.join(md)

def main():
    # Путь к файлам относительно корня проекта
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    instruction_defs = os.path.join(root_dir, 'include', 'instruction_defs.h')
    output_md = os.path.join(root_dir, 'docs', 'instructions.md')
    
    # Читаем файл определений
    with open(instruction_defs, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Парсим таблицу инструкций
    instructions = parse_instruction_table(content)
    
    # Генерируем markdown
    markdown = generate_markdown(instructions)
    
    # Сохраняем результат
    with open(output_md, 'w', encoding='utf-8') as f:
        f.write(markdown)
    
    print(f"Документация сгенерирована в {output_md}")

if __name__ == '__main__':
    main() 