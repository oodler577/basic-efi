#!/usr/bin/env python3
"""Package tracked source, EFI binary, examples and a clean VM image."""
import argparse
import hashlib
import pathlib
import subprocess
import sys
import zipfile

ROOT=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('--output',type=pathlib.Path,default=ROOT/'dist/basic-efi.zip');args=p.parse_args()
subprocess.run([sys.executable,str(ROOT/'tools/check_efi.py'),str(ROOT/'build/BOOTX64.EFI')],check=True)
subprocess.run([sys.executable,str(ROOT/'tools/check_vendor.py')],check=True)
image=ROOT/'build/basic-efi-usb.img'
subprocess.run([sys.executable,str(ROOT/'tools/create_image.py'),'--output',str(image),'--force'],check=True)
names=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')
files={name:(ROOT/name).read_bytes() for name in names if name and not name.startswith('.github/')}
files['usb/EFI/BOOT/BOOTX64.EFI']=(ROOT/'build/BOOTX64.EFI').read_bytes()
files['build/BOOTX64.EFI']=files['usb/EFI/BOOT/BOOTX64.EFI']
files['build/basic-efi-usb.img']=image.read_bytes()
for source in sorted([*(ROOT/'programs').glob('*.BAS'),*(ROOT/'programs').glob('*.PRG')]):
    files['usb/BASIC/'+source.name]=source.read_bytes()
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip()
files['REVISION.txt']=(revision+'\n').encode()
files['SHA256SUMS']=''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(files.items())).encode()
args.output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(args.output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
    for name,data in sorted(files.items()):
        info=zipfile.ZipInfo('basic-efi/'+name,date_time=(2026,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
        info.external_attr=0o100644<<16;archive.writestr(info,data)
with zipfile.ZipFile(args.output) as archive:assert archive.testzip() is None
print(f'{args.output}: {args.output.stat().st_size} bytes; revision {revision}')
