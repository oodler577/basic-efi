#!/usr/bin/env python3
"""Verify every actual GOP glyph against the independently parsed X16 source."""
import re
import time
import qemu_smoke as vm

fonts = {}
for name in ('iso-8859-15', 'petscii'):
    source = (vm.ROOT/'vendor/x16-charset'/f'{name}.s').read_text()
    fonts[name] = [int(bits, 2) for bits in re.findall(r'^\.byte %([01]{8})\s*$', source, re.M)]

vm.start()
try:
    vm.read_json(); vm.qmp('qmp_capabilities'); vm.wait_for('READY.')
    vm.type_line('INFO'); vm.wait_for('DISPLAY: GOP BLIT')
    for mode in ('ISO', 'PETSCII', 'LOWER'):
        # Lowercase physical keys generate unshifted PETSCII in both PET modes.
        vm.type_line('charset '+mode.lower())
        start = len(vm.trace())
        vm.type_line('for i=0 to255:poke 33408+int(i/32)*80+(i and31),i:next')
        vm.wait_for('READY.', after=start); time.sleep(.2)
        picture = vm.screenshot('charset-'+mode.lower())
        w, h = picture.size
        scale = min(w//640, h//200, 4)
        assert scale >= 1
        left, top = (w-640*scale)//2, (h-200*scale)//2
        for code in range(256):
            if mode == 'ISO':
                glyph = fonts['iso-8859-15'][code*8:code*8+8]
            else:
                base = (1024 if mode == 'LOWER' else 0)+(code&127)*8
                glyph = [v^(255 if code&128 else 0) for v in fonts['petscii'][base:base+8]]
            x0 = left+(code%32)*8*scale
            y0 = top+(8+code//32)*8*scale
            for y in range(8*scale):
                for x in range(8*scale):
                    expected = (187,187,187) if glyph[y//scale]&(128>>(x//scale)) else (0,0,170)
                    actual = picture.getpixel((x0+x,y0+y))
                    assert actual == expected, (mode, code, x, y, actual, expected)
        print(f'PASS: GOP {mode}, all 256 glyphs and scaled pixels match X16 source', flush=True)
    vm.type_line('charset iso');vm.type_line('print 6*7');vm.wait_for(' 42 ')
    vm.type_line('EXIT');vm.wait_for('BASIC EFI EXIT RETURN');vm.assert_firmware_return()
finally:
    if vm.proc.poll() is None:
        try: vm.qmp('quit')
        except Exception: vm.proc.terminate()
    vm.proc.wait(timeout=10)
