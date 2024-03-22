import sys
import math

def dbg(a):
    print(a)
    return a

def mul(x: int, y: int, pad: str):
    if x > y:
        x,y = y,x

    print(f"{pad}:: {x} * {y}", end="")

    if x.bit_length() <= 2:
        print(f" = {x * y}")
        return x * y
    else:
        print()

        l = x.bit_length()
        b = l // 2
        b2 = l - b
        mask = (1 << b) - 1

        x0 = x & mask
        x1 = x >> b

        y0 = y & mask
        y1 = y >> b

        print(f"{pad} b = {b}")
        print(f"{pad} x = {x:0{l}b}")
        print(f"{pad}  x0 = {x0:0{b}b}")
        print(f"{pad}  x1 = {x1:0{b2}b}")
        print(f"{pad} y = {y:0{l}b}")
        print(f"{pad}  y0 = {y0:0{b}b}")
        print(f"{pad}  y1 = {y1:0{b2}b}")

        newpad = pad + "  "
        z2 = mul(x1, y1, newpad)
        z0 = mul(x0, y0, newpad)
        z3 = mul((x1 + x0), (y1 + y0), newpad)
        z1 = z3 - z2 - z0

        out = z0
        out += z1 << b
        out += z2 << (b * 2)
        
        print(f"{pad} z0 = {z0}")
        print(f"{pad} z1 = {z1}")
        print(f"{pad} z2 = {z2}")
        print(f"= {out}")

        return out
    
out = mul(5, 6, "")
print(f"Result {out}")
