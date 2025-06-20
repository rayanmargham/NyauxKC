#!/usr/bin/env python3
import os
import re
import subprocess

with open('dump.txt', 'r') as f:
    text = f.read()

def translate(matchobj):
    addr = matchobj.group(0)
    out = subprocess.check_output(['addr2line', '-f', '-e', 'builds/kernel/NyauxKC', addr], stderr=subprocess.DEVNULL).decode().strip().split('\n')
    func = out[0]
    path = out[1].split(':')[0]
    file = os.path.basename(path)
    line = out[1].split(':')[1]
    return f'{func} ({file}:{line} {addr})'

text = re.sub(r'(0x[0-9a-f]*)', translate, text)
print(text)
