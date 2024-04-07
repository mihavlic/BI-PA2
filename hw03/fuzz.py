import sys
import random
import string

VARS = 64
LINES = 256
strings = []

def choose_string() -> int:
    count = len(strings)
    index = random.randint(0, count - 1)
    return index

def make_variable():
    count = random.randint(1,5)
    content = random.choice(string.ascii_lowercase) * count
    i = len(strings)
    strings.append(content)

    print(f'    CPatchStr s{i}("{content}");')

def validate(a: int):
    print(f'    assert(stringMatch(s{a}, "{strings[a]}"));')

def comment(var: int):
    fmt = f's{var}'
    print(f'    // {fmt:3} = {strings[var]}')

def make_append():
    a = choose_string()
    b = choose_string()

    comment(a)
    comment(b)
    
    if len(strings[b]) == 0:
        strings[b] = 'a'
        print(f'    s{b} = "a";')

    strings[a] += strings[b]
    
    print(f'    s{a}.append(s{b});')
    validate(a)

def make_substr():
    a = choose_string()
    b = choose_string()

    comment(a)
    
    if len(strings[a]) != 0:
        start = random.randint(0, len(strings[a]) - 1)
        end = random.randint(start, len(strings[a]))
    else:
        start = 0
        end = 0

    strings[b] = strings[a][start:end]
    
    print(f'    s{b} = s{a}.subStr({start}, {end - start});')
    validate(b)

def make_insert():
    a = choose_string()
    b = choose_string()

    comment(a)
    comment(b)
    
    if len(strings[b]) == 0:
        at = 0
    else:
        at = random.randint(0, len(strings[b]) - 1)

    at = random.randint(0, len(strings[a]) - 1)

    strings[a] = strings[a][:at] + strings[b] + strings[a][at:]

    print(f'    s{a}.insert({at}, s{b});')
    validate(a)

def make_remove():
    a = choose_string()

    comment(a)

    if len(strings[a]) == 0:
        return
    
    pos = random.randint(0, len(strings[a]) - 1)
    count = random.randint(0, len(strings[a]) - pos - 1)

    strings[a] = strings[a][:pos] + strings[a][(pos + count):]

    print(f'    s{a}.remove({pos}, {count});')
    validate(a)


print('class CPatchStr;')

print('void fuzz() {')

for _ in range(VARS):
    make_variable()

for _ in range(LINES // 3):
    make_append()
    
for _ in range(LINES):
    make_insert()
    
for _ in range(LINES // 3):
    make_append()

for _ in range(LINES):
    make_substr()
    
for _ in range(LINES // 3):
    make_append()

for _ in range(LINES):
    make_remove()
    
print('}')