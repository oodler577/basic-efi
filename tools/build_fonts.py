#!/usr/bin/env python3
"""Extract the unchanged, public-domain X16 glyph data; no assembler needed."""
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
parts = []
for filename, name in [('iso-8859-15.s', 'iso_font'), ('petscii.s', 'pet_font')]:
    source = (ROOT / 'vendor/x16-charset' / filename).read_text()
    values = [int(bits, 2) for bits in re.findall(r'^\.byte %([01]{8})\s*$', source, re.M)]
    assert len(values) == 2048, (filename, len(values))
    parts.append('static const unsigned char ' + name + '[2048] = {\n' +
                 '\n'.join(','.join(str(v) for v in values[i:i+16]) + ','
                           for i in range(0, 2048, 16)) + '\n};\n')
(ROOT / 'build').mkdir(exist_ok=True)
(ROOT / 'build/fonts.h').write_text('/* Generated from pinned X16 public-domain fonts. */\n' + ''.join(parts))
