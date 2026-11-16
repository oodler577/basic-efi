# BASIC EFI 0.1 validation — November 16, 2026

## Environment

- Build host: x86-64 Debian 13, GCC 14.2.0, GNU EFI 3.0.18.
- Assembler: Debian cc65 package 2.19-2 (`ca65` reports V2.18).
- Emulator: QEMU 10.0, q35 machine, 128 MiB RAM, VGA, xHCI USB keyboard and
  USB mass storage; no host disk or network adapter attached.
- Firmware: Debian OVMF 2025.02 4 MiB code and private variables images.

## Results

All 17 native integration tests pass. The same 17 pass with AddressSanitizer
and UndefinedBehaviorSanitizer enabled. LeakSanitizer was disabled because
the execution environment does not support its process inspection.

The tests cover arithmetic and strings; arrays, DATA/READ and subroutines;
numeric and mixed-case string INPUT; GET; exact ASCII SAVE/LOAD round trips;
overwrite prevention; path and symlink rejection; preservation of the current
program after invalid or oversized imports; 8/16/32/64-bit arithmetic and error
cases; virtual screen/color memory; editing an earlier visible line; wrapped
input; programs longer than the screen; and runtime file-command rejection.

The real EFI application was booted from an emulated USB mass-storage device.
QMP keyboard tests verified calculation, program entry, SAVE/LOAD/RUN, INPUT,
GET, editing a previous screen line, interrupting an infinite loop after an
ordinary key had already been pressed, and 8/64-bit calculations. The saved
program was read back from the FAT image and matched the expected ASCII exactly.

The **production executable**, without the debug-port instrumentation, was
also booted separately. Keyboard input created and ran a program, saved its
exact expected text to the USB image, and produced no debug-port output.
Screenshots were visually inspected for the BASIC screen, program output and
successful EXIT back to the OVMF firmware menu.

## Limits of these results

No physical PC or USB controller has been tested. IA32 UEFI, BIOS/DOS, 16-bit
x86 hosts, ARM hosts and Secure Boot signing are not implemented or tested.
The Windows QEMU launcher example has not been run on Windows.

The width selection is a separate integer workbench; it does not change the
original interpreter's numeric types. This is not a C64 compatibility or
game-compatibility test. The minimal BASIC configuration omits external
file channels and time variables. SAVE/LOAD support direct-mode ASCII programs.

The shipped disk image is clean: only the production EFI executable and the
two example programs are included. Test-created files and QA screenshots are
not part of the release archive. SHA-256 hashes are in `SHA256SUMS`.
