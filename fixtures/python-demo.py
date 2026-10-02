import math

def squares(count):
    return [i * i for i in range(count)]

print("Hello from InkPy MicroPython")
print("squares:", squares(6))
print("sqrt:", math.sqrt(81))
print("big integer:", 2 ** 80)

try:
    1 / 0
except ZeroDivisionError:
    print("caught division by zero")

# Globals remain available when launched with -i.
answer = 42
