#!/usr/bin/env python3
"""Make user-facing release assets from a checksummed, CI-tested package."""
import argparse
import hashlib
import pathlib
import re
import shutil
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--artifact-dir', type=pathlib.Path, required=True)
p.add_argument('--tag', required=True)
p.add_argument('--revision', required=True)
p.add_argument('--output', type=pathlib.Path, required=True)
a = p.parse_args()
if not re.fullmatch(r'v[0-9]+\.[0-9]+\.[0-9]+-test\.[0-9]+', a.tag):
    p.error('expected vN.N.N-test.N')
packages = list(a.artifact_dir.rglob('basic-efi.zip'))
if len(packages) != 1:
    p.error('expected exactly one tested basic-efi.zip')
prefix = 'basic-efi/'
with zipfile.ZipFile(packages[0]) as package:
    assert package.read(prefix+'REVISION.txt').decode().strip() == a.revision, 'wrong source revision'
    for line in package.read(prefix+'SHA256SUMS').decode().splitlines():
        digest, name = line.split('  ', 1)
        assert hashlib.sha256(package.read(prefix+name)).hexdigest() == digest, name
    files = {name[len(prefix+'usb/'):]: package.read(name) for name in package.namelist()
             if name.startswith(prefix+'usb/') and not name.endswith('/')}
    assert files['EFI/BOOT/BOOTX64.EFI'] == package.read(prefix+'build/BOOTX64.EFI')
    assert 'BASIC/CHARSETS.BAS' in files
    notices = []
    for name in ('LICENSE', 'NOTICE.md', 'vendor/msbasic/LICENSE', 'licenses/chips.txt',
                 'licenses/gnu-efi.txt', 'licenses/x16-rom.md'):
        notices.append(name+'\n\n'+package.read(prefix+name).decode())
    notices = '\n\n'.join(notices).encode()
    files['NOTICES.txt'] = notices
    files['REVISION.txt'] = (a.revision+'\n').encode()
    files['README.txt'] = b'''BASIC EFI test release - x64 UEFI

Copy the EFI and BASIC directories to the root of a spare FAT32 USB stick.
Use the computer's one-time boot menu and select the UEFI USB entry.
This unsigned build needs Secure Boot disabled. Consult the wiki before
changing firmware settings, especially on a machine using BitLocker.
No installation or internal disk changes are required. EXIT returns to firmware.

Try: LOAD "CHARSETS.BAS" then RUN. Press a key between character tables.
CHARSET ISO, CHARSET PETSCII, CHARSET LOWER select the available fonts.
HELP lists commands; INFO identifies the active renderer.

Tested in QEMU/OVMF. Physical hardware compatibility is not established.
https://github.com/oodler577/basic-efi/wiki
'''
    files['SHA256SUMS.txt'] = ''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n'
                                    for name, data in sorted(files.items())).encode()
a.output.mkdir(parents=True, exist_ok=False)
shutil.copyfile(packages[0], a.output/f'basic-efi-{a.tag}-complete.zip')
with zipfile.ZipFile(a.output/f'basic-efi-{a.tag}-usb.zip', 'w', zipfile.ZIP_DEFLATED) as usb:
    for name, data in sorted(files.items()):
        info = zipfile.ZipInfo(name, date_time=(2026,1,1,0,0,0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o100644 << 16
        usb.writestr(info, data)
(a.output/'BOOTX64.EFI').write_bytes(files['EFI/BOOT/BOOTX64.EFI'])
(a.output/'NOTICES.txt').write_bytes(notices)
assets = sorted(a.output.iterdir())
(a.output/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in assets))
print(f'Verified {a.revision}; prepared {len(assets)+1} release assets in {a.output}')
