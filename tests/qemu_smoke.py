#!/usr/bin/env python3
"""Boot a disposable USB disk in OVMF and exercise the real EFI application."""
import os,pathlib,subprocess,json,time,select,shutil
ROOT=pathlib.Path(__file__).resolve().parent.parent
BUILD=ROOT/'build'
QEMU=os.environ.get('QEMU','qemu-system-x86_64')
OVMF=pathlib.Path(os.environ.get('OVMF_DIR','/usr/share/OVMF'))
image=pathlib.Path(os.environ.get('TEST_IMAGE',str(BUILD/'test-usb.img')))
log=BUILD/'efi-debug.log'
vars_file=BUILD/f'test-vars-{os.getpid()}.fd'
cmd=[QEMU,'-machine','q35','-m','128','-nodefaults','-device','VGA'+(',romfile='+os.environ['VGA_ROM'] if 'VGA_ROM' in os.environ else ''),
     '-device','qemu-xhci','-device','usb-kbd',
     '-drive',f'if=pflash,format=raw,readonly=on,file={OVMF}/OVMF_CODE_4M.fd',
     '-drive',f'if=pflash,format=raw,file={vars_file}',
     '-drive',f'if=none,id=stick,format=raw,file={image}','-device','usb-storage,drive=stick',
     '-display','none','-qmp','stdio','-debugcon',f'file:{log}','-no-reboot']
if 'QEMU_DATA' in os.environ:cmd+=['-L',os.environ['QEMU_DATA']]
proc=None
def start():
    global proc
    shutil.copyfile(OVMF/'OVMF_VARS_4M.fd',vars_file)
    proc=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=(BUILD/'qemu-stderr.log').open('w'),bufsize=0)
def read_json(timeout=30):
    if not select.select([proc.stdout],[],[],timeout)[0]:raise TimeoutError('QMP timeout')
    s=proc.stdout.readline()
    if not s:raise RuntimeError('QEMU exited: '+(BUILD/'qemu-stderr.log').read_text())
    return json.loads(s)
def qmp(name,args=None):
    obj={'execute':name}
    if args is not None:obj['arguments']=args
    proc.stdin.write((json.dumps(obj)+'\n').encode());proc.stdin.flush()
    while True:
        result=read_json()
        if 'error' in result:raise RuntimeError(result)
        if 'return' in result:return result['return']
def trace():return log.read_bytes().decode('ascii',errors='replace') if log.exists() else ''
def wait_for(text,timeout=30):
    end=time.monotonic()+timeout
    while time.monotonic()<end:
        if text in trace():return
        if proc.poll() is not None:raise RuntimeError('QEMU exited')
        time.sleep(.05)
    qmp('screendump',{'filename':str(BUILD/'failure.ppm')})
    raise AssertionError('Did not see '+repr(text)+'; log tail: '+repr(trace()[-400:]))
shifted={'"':'apostrophe',':':'semicolon','+':'equal','(':'9',')':'0','$':'4','*':'8','<':'comma','>':'dot','?':'slash','_':'minus'}
normal={' ':'spc','\n':'ret','.':'dot',',':'comma',';':'semicolon','=':'equal','-':'minus','/':'slash'}
def key(code):
    keys=[{'type':'qcode','data':x} for x in code]
    qmp('send-key',{'keys':keys,'hold-time':15});time.sleep(.035)
def type_line(s):
    for c in s+'\n':
        if c in shifted:key(['shift',shifted[c]])
        elif c.isupper():key(['shift',c.lower()])
        else:key([normal.get(c,c)])
    time.sleep(.15)
def main():
    start()
    try:
        read_json();qmp('qmp_capabilities');wait_for('READY.')
        print('PASS: x64 EFI boots from an emulated USB mass-storage device',flush=True)
        qmp('screendump',{'filename':str(BUILD/'boot.ppm')})
        type_line('PRINT 6*7');wait_for(' 42 ')
        type_line('10 PRINT "USB ROUNDTRIP"')
        type_line('SAVE "VMTEST.BAS"');wait_for('SAVED')
        type_line('NEW');type_line('LOAD "VMTEST.BAS"');wait_for('LOADED')
        type_line('RUN');wait_for('USB ROUNDTRIP')
        type_line('NEW');type_line('10 INPUT A');type_line('20 PRINT A*3')
        type_line('RUN');type_line('7');wait_for(' 21 ')
        type_line('NEW');type_line('10 PRINT "FIRST"')
        key(['up']);key(['home']);type_line('10 PRINT "OTHER"')
        type_line('RUN');wait_for('OTHER');assert 'FIRST' not in trace()
        type_line('NEW');type_line('10 GOTO 10');type_line('RUN')
        key(['x']);key(['esc']);wait_for('BREAK')
        type_line('NEW');type_line('10 GET A$:IF A$="" THEN 10')
        type_line('20 PRINT "KEY=";A$');type_line('RUN')
        key(['shift','z']);wait_for('KEY=Z')
        type_line('BITS 8');type_line('CALC 255+1');wait_for('HEX $00')
        type_line('BITS 64');type_line('CALC $FFFFFFFFFFFFFFFF');wait_for('UNSIGNED 18446744073709551615')
        time.sleep(.5)
        qmp('screendump',{'filename':str(BUILD/'working.ppm')})
        print('PASS: keyboard, screen editing, INPUT, GET, break, USB save/load, integer workbench',flush=True)
        type_line('EXIT')
        time.sleep(3)
        print('EXIT command submitted; capturing firmware return for visual inspection',flush=True)
        qmp('screendump',{'filename':str(BUILD/'exit.ppm')})
    finally:
        if proc.poll() is None:
            try:qmp('quit')
            except Exception:proc.terminate()
        proc.wait(timeout=10)

if __name__=="__main__":main()
