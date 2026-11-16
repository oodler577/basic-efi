#!/usr/bin/env python3
"""Run BASIC EFI with a file-backed USB disk and private firmware variables."""
import argparse
import os
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--qemu', default=os.environ.get('QEMU', 'qemu-system-x86_64'))
parser.add_argument('--ovmf-code', type=pathlib.Path,
                    default=pathlib.Path('/usr/share/OVMF/OVMF_CODE_4M.fd'))
parser.add_argument('--ovmf-vars', type=pathlib.Path,
                    default=pathlib.Path('/usr/share/OVMF/OVMF_VARS_4M.fd'))
parser.add_argument('--image', type=pathlib.Path, help='regular image file; never a host disk')
args = parser.parse_args()

def regular(path):
    if path.is_symlink() or not path.is_file():
        parser.error(f'Expected a regular file: {path}')
    path = path.resolve()
    if ',' in str(path):
        parser.error('QEMU image paths must not contain commas')
    return path

code = regular(args.ovmf_code)
template = regular(args.ovmf_vars)
if args.image:
    image = regular(args.image)
else:
    state = ROOT / 'vm-state'
    if state.is_symlink():
        parser.error('vm-state must not be a symlink')
    state.mkdir(exist_ok=True)
    image = state / 'usb.img'
    if not image.exists():
        shutil.copyfile(regular(ROOT / 'build/basic-efi-usb.img'), image)
    image = regular(image)

with tempfile.TemporaryDirectory(prefix='basic-efi-') as temporary:
    variables = pathlib.Path(temporary) / 'vars.fd'
    shutil.copyfile(template, variables)
    command = [args.qemu, '-machine', 'q35', '-m', '128', '-nodefaults',
               '-device', 'VGA', '-device', 'qemu-xhci', '-device', 'usb-kbd',
               '-drive', f'if=pflash,format=raw,readonly=on,file={code}',
               '-drive', f'if=pflash,format=raw,file={variables}',
               '-drive', f'if=none,id=stick,format=raw,file={image}',
               '-device', 'usb-storage,drive=stick', '-no-reboot']
    print(f'BASIC saves are kept inside: {image}', flush=True)
    print('No host disks or network devices are attached. Close the VM window to quit.', flush=True)
    try:
        raise SystemExit(subprocess.call(command))
    except FileNotFoundError:
        parser.error('QEMU not found; install it or supply --qemu')
