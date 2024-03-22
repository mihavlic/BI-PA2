import sys
import random

MAX_BITS = 64
LINES = 64

def make_number():
    bits = random.randint(2, MAX_BITS)
    return random.getrandbits(bits)

def test_case(op_str, op_lambda):
    a = make_number()
    b = make_number()
    
    if random.getrandbits(1):
        a = -a
    if random.getrandbits(1):
        b = -b

    print(f' a = "{a}";')
    print(f' b = "{b}";')
    print(f' assert(equal(a {op_str} b, "{op_lambda(a, b)}"));')

sys.set_int_max_str_digits(2000000000)

print('void fuzz() {')
print('  CBigInt a{};')
print('  CBigInt b{};')
for _ in range(LINES):
    test_case('+', lambda a, b: a + b)
for _ in range(LINES):
    test_case('*', lambda a, b: a * b)
print('}')