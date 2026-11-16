#!/usr/bin/env python3
"""Exercise the shipped non-debug EFI build; screenshots need visual review.

Set TEST_IMAGE to a disposable image containing BOOTX64.EFI. This reuses the
QMP harness setup without running its debug-trace test body.
"""
import qemu_smoke as vm
import os
import subprocess
import time

vm.start()
qmp, proc, build = vm.qmp, vm.proc, vm.BUILD
try:
    vm.read_json(); qmp('qmp_capabilities')
    vm.wait_for_blue_screen()
    qmp('screendump', {'filename': str(build/'production-boot.ppm')})
    for line in ['10 PRINT "PRODUCTION BUILD"', 'SAVE "PROD.BAS"', 'RUN']:
        vm.type_line(line)
    time.sleep(1)
    qmp('screendump', {'filename': str(build/'production-run.ppm')})
    vm.type_line('EXIT')
    vm.assert_firmware_return()
    qmp('screendump', {'filename': str(build/'production-exit.ppm')})
finally:
    if proc.poll() is None:
        try: qmp('quit')
        except Exception: proc.terminate()
    proc.wait(timeout=10)

result = subprocess.run([os.environ.get('MTYPE', 'mtype'), '-i', str(vm.image),
                         '::/BASIC/PROD.BAS'], check=True, capture_output=True, text=True)
assert result.stdout == '10 PRINT "PRODUCTION BUILD"\n', result.stdout
assert vm.log.read_bytes() == b'', 'Production binary wrote the debug port'
print('PASS: production EFI accepted keyboard input and persisted the exact program; no debug-port writes')
print('Review production-boot/run/exit.ppm for screen output and firmware return.')
