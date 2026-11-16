#!/usr/bin/env python3
"""Exercise EFI with unavailable storage and read-only USB media."""
import os
import qemu_smoke as vm

vm.start()
try:
    vm.read_json();vm.qmp('qmp_capabilities');vm.wait_for('READY.')
    vm.type_line('DIR')
    if os.environ.get('MISSING_BASIC_FOLDER'):
        vm.wait_for('BASIC FOLDER UNAVAILABLE')
    else:
        vm.wait_for('HELLO.BAS')
        vm.type_line('LOAD "HELLO.BAS"');vm.wait_for('LOADED')
        vm.type_line('RUN');vm.wait_for('HELLO FROM MICROSOFT BASIC')
    vm.type_line('SAVE "DENIED.BAS"');vm.wait_for('FILE I/O ERROR')
    vm.type_line('PRINT 7*7');vm.wait_for(' 49 ')
    vm.type_line('EXIT');vm.wait_for('BASIC EFI EXIT RETURN');vm.assert_firmware_return()
    print('PASS: storage failure is recoverable; BASIC and EXIT still work')
finally:
    if vm.proc.poll() is None:
        try:vm.qmp('quit')
        except Exception:vm.proc.terminate()
    vm.proc.wait(timeout=10)
