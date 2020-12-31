#!/usr/bin/env python3

import sys

param = sys.argv[1]

if '=' in param:
    reg_name, hex_str = param.split('=')
else:
    reg_name = 'register'
    hex_str = param

value = int(hex_str, 0)

print(reg_name, '0x{0:x}'.format(value))
for bitn in range(64):
    mask = 1 << bitn
    bit = '1' if value & mask != 0 else ' '
    print(bitn, bit)
