# BASIC EFI 0.1 validation — November 16, 2026

## Freestanding runtime fix

The first graphics release candidate exposed Ubuntu 22.04's injected
`__memcpy_chk` dependency in EFI's status-line rendering. EFI has no Linux
runtime to resolve it. EFI builds now explicitly disable libc fortification,
link with `--no-undefined`, and check the PE image's ELF dynamic-symbol records
for unresolved symbols. The new image check rejects the captured failing CI
binary before boot. The failed test tag is retained; corrected candidates use
a new tag rather than changing the identity of an existing one.

## X16 character-set milestone

The native suite now has 33 Python tests plus screen and character/renderer C
harnesses. ISO quoted bytes 160–255 and both PET modes round-trip through text
files. Charset switches preserve BASIC programs. The renderer harness covers
all 3 x 256 glyphs at each scale 1–4, buffer guards, reverse video and controls.

Five local real-EFI scenarios pass, including a new glyph-table test which
compares every displayed pixel of all 256 glyphs in all three modes against the
independently parsed upstream X16 font data. In OVMF at 1280x800, the actual
cells are 16x16 pixels. The physical-hardware caveat still applies. Hosted CI
repeats these checks for both EFI matrix entries.

GOP output uses firmware blits, without direct framebuffer pointer arithmetic.
Firmware text and terminal fallbacks approximate glyphs. The full X16 keyboard
layout and complete KERNAL control-code semantics are not implemented.

## Earlier hardening milestone

The expanded suite now has 30 Python tests, plus a C screen harness performing
10,000 scrolls and 200,000 seeded edit events. It includes 2,400 integer
expressions checked against Python, 600 model-checked program edits and 120
malformed-import cases. All pass locally, including AddressSanitizer and
UndefinedBehaviorSanitizer runs (LeakSanitizer disabled only in this local
environment because process inspection is unavailable).

All four local real-EFI scenarios pass: writable USB, production executable,
read-only USB, and missing BASIC folder. The tests check persisted files, reset,
break, error recovery, and return to the OVMF menu. The hosted workflow expands
this to GCC/Clang, Ubuntu 22.04/24.04 and PC/qemu64 versus q35/max machines;
consult the linked commit's Actions results for hosted status.

Defects found by the expanded checks include character deletion leaving colors
behind, and a Clang warning in the freestanding strcpy implementation. Both
are fixed. Image creation now publishes atomically and preserves an old image
when an external formatting/copy tool fails.

The original prototype validation below is retained as its historical baseline.

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
