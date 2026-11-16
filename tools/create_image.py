#!/usr/bin/env python3
"""Create a disposable FAT32 USB image. Never accepts a block-device target."""
import pathlib,subprocess,os,argparse
root=pathlib.Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser()
p.add_argument('--efi',type=pathlib.Path,default=root/'build/BOOTX64.EFI')
p.add_argument('--output',type=pathlib.Path,default=root/'build/basic-efi-usb.img')
p.add_argument('--force',action='store_true',help='replace an existing regular image file')
a=p.parse_args()
target=a.output.absolute()
if target.is_symlink() or (target.exists() and not target.is_file()) or str(target).startswith('/dev/'):
    raise SystemExit('Refusing non-regular image target')
if target.exists() and not a.force:
    raise SystemExit('Image already exists; use --force to replace it')
if not a.efi.is_file():
    raise SystemExit('EFI executable not found; build it first')
target.parent.mkdir(parents=True,exist_ok=True)
with target.open('wb') as f:f.truncate(64*1024*1024)
def run(cmd):subprocess.run(cmd,check=True)
run([os.environ.get('MKFS_FAT','mkfs.fat'),'-F','32','-n','BASICEFI',str(target)])
run([os.environ.get('MMD','mmd'),'-i',str(target),'::/EFI','::/EFI/BOOT','::/BASIC'])
run([os.environ.get('MCOPY','mcopy'),'-i',str(target),str(a.efi),'::/EFI/BOOT/BOOTX64.EFI'])
for source in sorted((root/'programs').glob('*.BAS')):
    run([os.environ.get('MCOPY','mcopy'),'-i',str(target),str(source),'::/BASIC/'+source.name])
print(target)
