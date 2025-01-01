#!/usr/bin/python3

import re
import os

def parse_instruction_list(content, list_name):
    # Извлекаем список инструкций
    match = re.search(fr'#define\s+{list_name}\s*\(\s*X\s*\)\s*\\(.*?)(?:\\|\n\n)', content, re.DOTALL)
    if not match:
        raise Exception(f"{list_name} не найден")
    
    instructions = []
    for line in match.group(1).split('\\'):
        line = line.strip()
        if not line or line.startswith('/*') or line.endswith('*/'):
            continue
        
        # Извлекаем параметры инструкции
        match = re.match(r'X\s*\(\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)', line)
        if match:
            name, value, operands, desc, group = match.groups()
            instructions.append({
                'name': name,
                'value': int(value),
                'operands': int(operands),
                'desc': desc,
                'group': group
            })
    
    return instructions

def parse_instruction_defs(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Парсим оба списка инструкций
    basic_instructions = parse_instruction_list(content, 'BASIC_INSTRUCTION_LIST')
    pseudo_instructions = parse_instruction_list(content, 'PSEUDO_INSTRUCTION_LIST')
    
    return basic_instructions, pseudo_instructions

def generate_markdown(basic_instructions, pseudo_instructions):
    md = []
    md.append('# Система команд виртуальной машины Заря\n')
    
    # Общие сведения
    md.append('## Общие сведения\n')
    md.append('Виртуальная машина Заря использует стековую архитектуру с троичной логикой. ')
    md.append('Все операции выполняются над значениями в стеке, за исключением инструкций, ')
    md.append('которые явно работают с регистрами или памятью.\n\n')
    md.append('Система команд разделена на две категории:\n')
    md.append('1. Базовые инструкции - транслируются напрямую в машинный код\n')
    md.append('2. Псевдоинструкции - транслируются в последовательность базовых инструкций\n\n')
    
    # Базовые инструкции
    md.append('## Базовые инструкции\n')
    groups = {}
    for inst in basic_instructions:
        group = inst['group']
        if group not in groups:
            groups[group] = []
        groups[group].append(inst)
    
    for group in sorted(groups.keys()):
        md.append(f'### {group}\n')
        md.append('| Мнемоника | Опкод | Операнды | Описание |\n')
        md.append('|-----------|--------|----------|----------|\n')
        
        for inst in sorted(groups[group], key=lambda x: x['value']):
            md.append(f"| {inst['name']} | {inst['value']} | {inst['operands']} | {inst['desc']} |\n")
        md.append('\n')
    
    # Псевдоинструкции
    md.append('## Псевдоинструкции\n')
    md.append('Псевдоинструкции предоставляют более удобный синтаксис для часто используемых операций. ')
    md.append('Каждая псевдоинструкция транслируется в последовательность базовых инструкций.\n\n')
    
    groups = {}
    for inst in pseudo_instructions:
        group = inst['group']
        if group not in groups:
            groups[group] = []
        groups[group].append(inst)
    
    for group in sorted(groups.keys()):
        md.append(f'### {group}\n')
        md.append('| Мнемоника | Операнды | Описание | Трансляция |\n')
        md.append('|-----------|----------|-----------|------------|\n')
        
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
        
        for inst in sorted(groups[group], key=lambda x: x['value']):
            translation = translations.get(inst['name'], '')
            md.append(f"| {inst['name']} | {inst['operands']} | {inst['desc']} | {translation} |\n")
        md.append('\n')
    
    return ''.join(md)

def main():
    # Путь к файлам относительно корня проекта
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    instruction_defs = os.path.join(root_dir, 'include', 'instruction_defs.h')
    output_md = os.path.join(root_dir, 'docs', 'instructions.md')
    
    # Парсим определения инструкций
    basic_instructions, pseudo_instructions = parse_instruction_defs(instruction_defs)
    
    # Генерируем markdown
    markdown = generate_markdown(basic_instructions, pseudo_instructions)
    
    # Сохраняем результат
    with open(output_md, 'w', encoding='utf-8') as f:
        f.write(markdown)
    
    print(f"Документация сгенерирована в {output_md}")

if __name__ == '__main__':
    main() 