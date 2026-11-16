#!/usr/bin/env python3
"""Build the Microsoft interpreter and export only symbols needed by the host."""
import pathlib, re, subprocess, os

root = pathlib.Path(__file__).resolve().parent.parent
build = root / 'build'
build.mkdir(exist_ok=True)
source = (root / 'vendor/msbasic/m6502-min.s').read_text()
source = source.replace('LINLEN=40', 'LINLEN=80').replace('BUFLEN=81', 'BUFLEN=241')
# The original signed-Y copy loop assumes a tokenized line shorter than 129 bytes.
# Compare against the unsigned sentinel so the editor's 240-character lines work.
source = source.replace('        BPL     STOLOP', '        CPY     #255\n        BNE     STOLOP')
source = source.replace('DT"### COMMODORE BASIC ###"', 'DT"MICROSOFT 6502 BASIC 1.1"')
symbols = ['INIT', 'READY', 'MAIN', 'TXTPTR', 'TXTTAB', 'VARTAB', 'ARYTAB', 'STREND',
           'CURLIN', 'RESLST', 'LINWID', 'TRMPOS', 'MEMSIZ', 'FRETOP', 'SCRTCH', 'BUF', 'STOP']
source += '\n.export ' + ','.join(symbols) + '\n'
(build / 'basic.s').write_text(source)
subprocess.run([os.environ.get('CA65', 'ca65'), '--feature', 'force_range', '-o', str(build/'basic.o'), str(build/'basic.s')], check=True)
subprocess.run([os.environ.get('LD65', 'ld65'), '-C', str(root/'vendor/msbasic/m6502-cbm.cfg'), '-Ln', str(build/'basic.lbl'), '-o', str(build/'basic.rom'), str(build/'basic.o')], check=True)
labels = {}
for line in (build/'basic.lbl').read_text().splitlines():
    match = re.match(r'al ([0-9A-Fa-f]+) \.(\w+)', line)
    if match:
        labels[match[2]] = int(match[1], 16)
data = (build/'basic.rom').read_bytes()
text = '/* Generated from the attributed Microsoft BASIC source. */\n'
text += ''.join(f'#define ROM_{s} 0x{labels[s]:04x}\n' for s in symbols)
text += 'static const unsigned char basic_rom[] = {\n'
text += '\n'.join(','.join(f'0x{b:02x}' for b in data[i:i+16])+',' for i in range(0,len(data),16))
text += '\n};\n'
(build/'rom.h').write_text(text)
print(f'Built Microsoft BASIC: {len(data)} bytes; entry ${labels["INIT"]:04X}')
