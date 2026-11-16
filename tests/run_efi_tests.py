#!/usr/bin/env python3
"""Create isolated disks and run six real EFI boot scenarios."""
import os
import pathlib
import subprocess
import sys
from prg_fixture import HELLO,IEC_HELLO

ROOT=pathlib.Path(__file__).resolve().parent.parent
QA=ROOT/'build/qa'
QA.mkdir(parents=True,exist_ok=True)
def run(command,**kwargs):subprocess.run(command,check=True,cwd=ROOT,**kwargs)
for scenario in ('debug','production','readonly','missing-folder','charsets','iec'):
    image=QA/(scenario+'.img');debug=scenario!='production'
    binary='build/BOOTIEC.EFI' if scenario=='iec' else 'build/BOOTTEST.EFI' if debug else 'build/BOOTX64.EFI'
    run([sys.executable,'tools/create_image.py','--efi',binary,
         '--output',str(image),'--force'])
    env=dict(os.environ,TEST_IMAGE=str(image),QEMU_OUTPUT_DIR=str(QA/scenario))
    if scenario=='readonly':env['READ_ONLY_IMAGE']='1'
    if scenario=='missing-folder':
        run([os.environ.get('MDELTREE','mdeltree'),'-i',str(image),'::/BASIC'])
        env['MISSING_BASIC_FOLDER']='1'
    script='iec_efi.py' if scenario=='iec' else 'charset_efi.py' if scenario=='charsets' else 'qemu_smoke.py' if scenario=='debug' else 'production_smoke.py' if scenario=='production' else 'storage_efi.py'
    run([sys.executable,'tests/'+script],env=env,timeout=180)
    if scenario=='debug':
        saved=subprocess.check_output([os.environ.get('MTYPE','mtype'),'-i',str(image),'::/BASIC/VMTEST.BAS'])
        assert saved==b'10 PRINT "USB ROUNDTRIP"\n',saved
        assert subprocess.check_output([os.environ.get('MTYPE','mtype'),'-i',str(image),'::/BASIC/C64COPY.PRG'])==HELLO
    elif scenario=='iec':
        saved=subprocess.check_output([os.environ.get('MTYPE','mtype'),'-i',str(image),'::/BASIC/FROMIEC.BAS'])
        assert saved==b'10 PRINT "IEC MOCK READ"\n20 END\n',saved
        assert subprocess.check_output([os.environ.get('MTYPE','mtype'),'-i',str(image),'::/BASIC/IECC64.PRG'])==IEC_HELLO
        for name in ('IECWRITE.BAS','FAILWRITE.BAS','C64WRITE.PRG'):
            denied=subprocess.run([os.environ.get('MTYPE','mtype'),'-i',str(image),'::/BASIC/'+name],capture_output=True)
            assert denied.returncode!=0,'IEC SAVE leaked to FAT'
    elif scenario in ('readonly','missing-folder'):
        denied=subprocess.run([os.environ.get('MTYPE','mtype'),'-i',str(image),'::/BASIC/DENIED.BAS'],capture_output=True)
        assert denied.returncode!=0,'Failed SAVE unexpectedly created a file'
print('PASS: all six EFI boot scenarios')
