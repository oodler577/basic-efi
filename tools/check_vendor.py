#!/usr/bin/env python3
"""Verify pinned upstream files without network access."""
import hashlib
import pathlib
ROOT=pathlib.Path(__file__).resolve().parent.parent
for line in (ROOT/'vendor/SHA256SUMS').read_text().splitlines():
    expected,name=line.split('  ',1)
    actual=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
    if actual!=expected:raise SystemExit(f'Vendor checksum mismatch: {name}')
print('PASS: pinned upstream source checksums')
