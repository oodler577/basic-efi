#!/usr/bin/env python3
"""Build a FAT32 image atomically; never open a device or existing file for writing."""
import argparse
import os
import pathlib
import subprocess
import tempfile

ROOT=pathlib.Path(__file__).resolve().parent.parent

def create(efi,output,force=False,programs=None):
    target=pathlib.Path(output).absolute();efi=pathlib.Path(efi).absolute()
    if target.is_symlink() or (target.exists() and not target.is_file()) or str(target.resolve()).startswith('/dev/'):
        raise ValueError('Refusing non-regular image target')
    if target.exists() and not force:raise ValueError('Image already exists; use --force to replace it')
    if not efi.is_file():raise ValueError('EFI executable not found; build it first')
    if target.exists() and os.path.samefile(target,efi):raise ValueError('Image target is the EFI input file')
    target.parent.mkdir(parents=True,exist_ok=True)
    fd,name=tempfile.mkstemp(prefix='.basic-efi-',suffix='.img',dir=target.parent)
    temp=pathlib.Path(name)
    try:
        with os.fdopen(fd,'wb') as f:f.truncate(64*1024*1024)
        def run(tool,args):subprocess.run([os.environ.get(tool,{'MKFS_FAT':'mkfs.fat','MMD':'mmd','MCOPY':'mcopy'}[tool]),*args],check=True)
        run('MKFS_FAT',['-F','32','-n','BASICEFI',str(temp)])
        run('MMD',['-i',str(temp),'::/EFI','::/EFI/BOOT','::/BASIC'])
        run('MCOPY',['-i',str(temp),str(efi),'::/EFI/BOOT/BOOTX64.EFI'])
        for source in sorted(pathlib.Path(programs or ROOT/'programs').glob('*.BAS')):
            run('MCOPY',['-i',str(temp),str(source),'::/BASIC/'+source.name])
        if force:os.replace(temp,target)
        else:
            # Atomic no-clobber publication, even if another process creates target.
            os.link(temp,target)
    finally:
        temp.unlink(missing_ok=True)
    return target

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--efi',type=pathlib.Path,default=ROOT/'build/BOOTX64.EFI')
    p.add_argument('--output',type=pathlib.Path,default=ROOT/'build/basic-efi-usb.img')
    p.add_argument('--force',action='store_true',help='atomically replace an existing regular image')
    a=p.parse_args()
    try:print(create(a.efi,a.output,a.force))
    except (ValueError,OSError,subprocess.CalledProcessError) as error:raise SystemExit(str(error))
