# Sources and attribution

This is an independent experimental project, not a Microsoft or Commodore product.

## Microsoft BASIC

- Original source: https://github.com/microsoft/BASIC-M6502
- cc65 adaptation used here: https://github.com/rene6502/BASIC-M6502-CC65
- Pinned adaptation commit: `fa31daddafa6f1280efce9d66f60a4d2bf4ea474`
- License: `vendor/msbasic/LICENSE` (MIT, Microsoft Corporation).

The supplied `m6502-min.s` is the upstream minimal Commodore-derived build
with external file channels, time variables and hardware-dependent random
number seeding removed. Its lineage is PET BASIC; this is not the C64 KERNAL
or a claim of C64 binary compatibility.

The upstream files are retained unchanged. `tools/build_rom.py` creates a
derived assembly file with these changes:

1. Screen width 40 → 80 and input buffer size 81 → 241.
2. Banner changed to identify Microsoft 6502 BASIC 1.1.
3. The program-line copy loop uses an unsigned end comparison, permitting
   tokenized lines longer than the original signed-index limit.
4. Host integration symbols are exported for the emulator and storage adapter.

The host supplies keyboard, output and break routines at the external vectors,
an isolated RAM/ROM map, text import/export and the screen editor. BASIC's
arithmetic implementation is retained. No downloaded C64 ROM image is included.

## 6502 CPU emulation

- Source: https://github.com/floooh/chips
- Pinned commit: `7ef8776f637a6424ac615da64de6b968b06de791`
- File: `vendor/chips/m6502.h`, unchanged.
- License: zlib, included in that file and copied to `licenses/chips.txt`.

## GNU EFI

The EFI executable links GNU EFI initialization and support code. The build
used GNU EFI 3.0.18 as packaged by Debian. Its copyright notices and license
texts are included in `licenses/gnu-efi.txt`.

This product includes software developed by Intel Corporation and its contributors.

## Project additions

The host C code, build scripts, tests, examples and project documentation are
provided under the MIT license in `LICENSE`. Upstream licenses continue to
apply to the vendored components. QEMU, OVMF, the compiler and assembler are
build/test dependencies and are not redistributed in this archive.
