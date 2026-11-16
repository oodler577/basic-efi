Bootable Microsoft BASIC with selectable X16 PETSCII graphics, PETSCII
upper/lowercase and ISO-8859-15 character sets. EFI graphics use the original
8x8 glyphs scaled 1x–4x to fit the firmware display.

**Choose the download:**

- `*-usb.zip`: ready-to-copy `EFI` and `BASIC` folders, examples and instructions.
- `*-complete.zip`: full source, tools, EFI executable and a 64 MiB VM disk image.
- `BOOTX64.EFI`: standalone x64 EFI executable; see `NOTICES.txt` for licenses.
- `SHA256SUMS.txt`: checksums of the release files.

For a spare FAT32 USB stick, copy the USB ZIP's folders to its root and choose
that USB's UEFI entry in the one-time boot menu. The executable belongs at
`EFI/BOOT/BOOTX64.EFI`. This is unsigned; read
[Booting from USB](https://github.com/oodler577/basic-efi/wiki/Booting-from-USB)
before changing Secure Boot settings. Nothing needs to be installed on the
internal disk. `EXIT` returns control to firmware.

Try `LOAD "CHARSETS.BAS"`, then `RUN`. Press a key between tables.
`CHARSET ISO`, `CHARSET PETSCII` and `CHARSET LOWER` select modes directly.
`INFO` shows the display backend and scale.

Publication requires all nine CI jobs to pass: GCC/Clang builds and tests,
ASan/UBSan, coverage-guided fuzzing, and real QEMU/OVMF EFI tests on two machine
configurations. The glyph tests compare every rendered pixel of all 256 codes
in each of the three character sets with the upstream X16 source. The published
assets come from the tested build, with revision and package checksums verified.

This is a **test prerelease**. Physical PCs remain untested. The target is x64
UEFI, with no IA32 or legacy BIOS build. Exact fonts require GOP; firmware-text
and desktop terminal fallbacks approximate some graphics. The full X16 keyboard
layout and KERNAL control-code behavior are not implemented. BASIC keeps its
original arithmetic; `BITS` controls the separate `CALC` workbench.

[Getting started](https://github.com/oodler577/basic-efi/wiki/Getting-started) ·
[Character sets](https://github.com/oodler577/basic-efi/wiki/Character-sets)
