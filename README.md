# BASIC EFI 0.1 — experimental prototype

Boot into an editable blue BASIC screen on an x64 UEFI machine. This runs
Microsoft's original 6502 BASIC source, adapted for an isolated 6502 virtual
machine inside a small EFI application. No operating system is needed underneath.

The current release is a **VM-tested prototype**, not a hardware-certified release.
Start with the included VM image. Physical firmware compatibility has not been tested.

## What works

- Microsoft BASIC programs, numbered lines, LIST, RUN, NEW, arrays, strings,
  DATA/READ, GOSUB, INPUT, GET, PEEK and POKE.
- An 80-column, 24-row editable screen, plus a status row. Move the cursor onto
  an earlier visible line, edit it, and press Enter to submit that line again.
  Program storage survives display scrolling. Logical lines may wrap to 240 characters.
- A virtual text framebuffer and 16 color values, accessed with PEEK/POKE.
- ASCII program SAVE/LOAD in the `BASIC` directory of the volume containing the
  EFI application. A USB stick works when its filesystem is exposed by the firmware.
- An exact-width integer workbench with 8, 16, 32, 64 and native-width modes.
- Esc to interrupt a running program, F9 to reset the virtual machine, EXIT to
  return to the firmware or EFI shell that launched it.

This is the Microsoft interpreter, with documented source adaptations. It is
not the C64 ROM or a full C64 emulator. C64 machine-code games, sprites, SID,
PETSCII graphics, disk images and JiffyDOS are outside this prototype.

## Try it in a VM first

The archive contains:

| Path | Purpose |
|---|---|
| `usb/EFI/BOOT/BOOTX64.EFI` | Production x64 EFI executable |
| `usb/BASIC/` | Example programs and the location for your saved programs |
| `build/basic-efi-usb.img` | Ready-to-boot 64 MiB FAT32 image for a VM |
| `tools/run_vm.py` | QEMU launcher using only an image file and private firmware variables |
| `src/`, `vendor/`, `tests/` | Full source, dependencies and tests |

On an x64 Linux system with QEMU, OVMF and Python 3 installed:

```sh
python3 tools/run_vm.py
```

The launcher copies the supplied image to `vm-state/usb.img` the first time.
Subsequent runs reuse that copy, so SAVE persists. Firmware variables are kept
in a temporary file. No host disk, host directory share or network adapter is
attached to the VM. To start over, close QEMU and move `vm-state/usb.img` aside.

On Windows, the same launcher can use an installed QEMU and an OVMF **4 MiB
code/variables pair**. Supply their actual locations, for example:

```powershell
py tools/run_vm.py --qemu "C:\Program Files\qemu\qemu-system-x86_64.exe" --ovmf-code "C:\firmware\OVMF_CODE_4M.fd" --ovmf-vars "C:\firmware\OVMF_VARS_4M.fd"
```

QEMU and OVMF are not bundled. The Windows launcher path is provided for
convenience; this release was tested with QEMU/OVMF on Linux.

## Boot from a spare USB stick

1. Use a spare USB stick already formatted as FAT32, with no existing
   `EFI/BOOT/BOOTX64.EFI` that needs preserving.
2. Copy the **contents** of `usb/` to its root. You should have
   `EFI/BOOT/BOOTX64.EFI` and `BASIC/HELLO.BAS` on that stick.
3. Use the computer's one-time firmware boot menu and choose the USB's UEFI entry.
4. Enter `HELP`, or load an example as shown below.
5. Enter `EXIT` to return to firmware. To resume the usual OS, remove the USB
   and boot normally. Firmware may present a menu or shell after EXIT.

There is no installer. The application does not change partitions, write raw
disk sectors, install a bootloader on an internal disk, or call SetVariable to
change firmware settings. Its file interface opens only the `BASIC` directory
on its own loaded-image volume. Place it on USB, not your internal EFI partition.
SAVE creates new files and refuses overwrites; LOAD replaces the BASIC program
in memory after validating the source file.

The executable is **unsigned**. Secure Boot may reject it. The VM route allows
testing without changing the physical machine's Secure Boot configuration.
Firmware bugs and untested hardware prevent a promise of zero risk on real PCs.

## First session

```basic
PRINT 2+2
10 FOR I=1 TO 5
20 PRINT I;I*I
30 NEXT I
RUN
SAVE "SQUARES.BAS",8
NEW
DIR
LOAD "SQUARES.BAS",8
LIST
RUN
```

`LOAD "HELLO.BAS"` followed by `RUN` runs the supplied introduction.
`LOAD "SCREEN.BAS"` demonstrates virtual screen bytes and colors.

`SAVE` and `LOAD` operate in direct mode, outside a running program. The optional
`,8` preserves familiar Commodore syntax; it refers to the same BASIC folder,
not a selectable hardware drive. Filenames may contain letters, digits, dots,
underscores and hyphens, up to 64 characters. Paths and dot-prefixed names are
rejected. Use a new filename when saving a revised version.

Programs use printable ASCII, one numbered line per text line, with CRLF or LF
endings. Import accepts line numbers 0–63999 and at most 240 characters per
line. It uses a conservative 30,000-byte storage budget before tokenization;
oversized or structurally invalid imports leave the existing program intact.
LOAD does not automatically RUN. SAVE exports text, not tokenized PRG files.

Ordinary BASIC syntax errors remain BASIC's responsibility. A structurally
valid source file can still contain a program that reports an error when run.
The minimal interpreter does not supply external file channels, OPEN/CLOSE,
PRINT#/INPUT#, TI/TI$, or physical peripheral access.

## Screen editing and virtual memory

| Key or command | Action |
|---|---|
| Arrows | Move within the visible screen |
| Typing | Overwrite characters at the cursor |
| Home / End | Move to the start / end of the logical line |
| Backspace / Delete | Delete a character and shift the rest left |
| Enter | Submit the whole logical line under the cursor |
| Esc during RUN | Break into BASIC; CONT can resume when BASIC permits |
| F9 / RESET | Reset BASIC memory; unsaved program and variables are lost |
| CLS | Clear the display, preserving the stored program |
| INFO / HELP | Show environment information or commands |
| EXIT | Return from the EFI application |

There is no shell-style command history or scrollback editor. To re-edit a
stored line that has scrolled away, LIST it first. Cursor editing uses overwrite
mode, not insert mode. Text is ASCII rendered with the firmware's font and color
palette, so this is a Commodore-inspired environment rather than a pixel-exact C64.

| Virtual address | Meaning |
|---|---|
| `$0401` through below `$8000` | Original BASIC workspace; 31,743 bytes reported at startup |
| `$8000` / 32768, length 1920 | 80 × 24 printable ASCII screen cells |
| `$9000` / 36864, length 1920 | Color cells; only the low four bits are used |
| `$C000` onward | BASIC ROM, protected against emulated writes |

For example, `POKE 32768,65` writes an A in the top-left cell and
`POKE 36864,5` changes its color. Screen output can subsequently overwrite it.
PEEK and POKE operate only on the emulated machine. They do not expose host RAM,
physical disk controllers, UEFI pointers or native x86 I/O ports.

## What 8/16/32/64/native means here

There are three independent widths: the host CPU mode, the emulated CPU, and
the language's numeric types. This prototype boots in **x64 UEFI**, emulates an
**8-bit 6502**, and preserves the original BASIC numeric behavior.

The additional workbench performs exact unsigned integer arithmetic with
wrapping at the selected width, displaying unsigned, signed two's-complement
and hexadecimal interpretations:

```text
BITS 8
CALC 255+1
UNSIGNED 0  SIGNED 0  HEX $00
BITS 64
CALC $FFFFFFFFFFFFFFFF
UNSIGNED 18446744073709551615  SIGNED -1  HEX $FFFFFFFFFFFFFFFF
```

`CALC` supports decimal and `$`/`0x` hexadecimal literals, parentheses, unary
`+ - ~`, and binary `* / % + - << >> & ^ |`, using C-like precedence. Arithmetic
wraps to the selected width, right shift is logical, and division is unsigned.
Division by zero and shift counts at least as large as the width are errors.
Literals are reduced to the selected width too. `NATIVE` means host pointer
width, currently 64 in the EFI build.

**BITS does not widen BASIC variables, change floating-point precision, or
change x86 CPU modes.** A true wide-integer BASIC dialect is future work and
should use explicit numeric types without silently changing historic programs.

## x86 compatibility route

| Machine family | Status / route |
|---|---|
| x86-64 with 64-bit UEFI and an 80×25 text console | Current target; tested in QEMU/OVMF |
| x86 machine with 32-bit UEFI | Future IA32 EFI build and separate tests |
| 386 through early x86-64 with legacy BIOS only | Future BIOS/DOS platform frontend |
| 8086/8088/286 | Separate 16-bit implementation needed; current C build does not target these |
| ARM UEFI | Possible future host build; not included or tested |

One EFI executable cannot cover legacy BIOS-only PCs. The useful shared pieces
are the screen model, file commands and BASIC source; each host family needs
an appropriate boot and I/O frontend. Running an 8-bit BASIC does not require
the x86 processor to have an 8-bit execution mode.

## Build and verify

On x86-64 Debian 13, the build used GCC 14, GNU EFI 3.0.18 and cc65. Install
the corresponding development tools plus Python 3, make and binutils. Building
images additionally needs dosfstools and mtools. VM tests need QEMU and OVMF.

```sh
make all
make test
python3 tools/create_image.py
```

`EFI_ROOT` defaults to `/usr`; `EFI_LIB` may be overridden if the distribution
places GNU EFI libraries elsewhere. `CA65` and `LD65` environment variables can
select assembler binaries. `make desktop` builds a POSIX terminal frontend:

```sh
./build/basic-efi programs
```

Use at least an 80×25 terminal. This shares the EFI interpreter and screen
model; its chosen program directory replaces the boot-volume BASIC folder.

The image builder writes a regular file, rejects device targets and symlinks,
and requires `--force` to replace an existing image. It never flashes a USB.

For the automated real-EFI test, build the debug-port variant separately:

```sh
make build/BOOTTEST.EFI
python3 tools/create_image.py --efi build/BOOTTEST.EFI --output build/test-usb.img
python3 tests/qemu_smoke.py
```

The test uses QMP keyboard input and an emulated USB mass-storage device. It
writes a debug trace and QA screenshots under `build/`; the production EFI
binary does not write the QEMU debug port. `QEMU`, `OVMF_DIR`, `QEMU_DATA`,
`VGA_ROM` and `TEST_IMAGE` can override test paths. Recreate the test image with
`--force` before rerunning, because its SAVE deliberately refuses overwrites.

`tests/production_smoke.py` checks the non-debug executable using a disposable
copy of the production image selected with `TEST_IMAGE`. It verifies the saved
file with `mtype` (`MTYPE` can override its path); review its generated screen
captures for visible output and firmware return. Never run a test against an
image containing work you want to preserve.

See `VALIDATION.md` for the tested release and limits, and `NOTICE.md` for
upstream attribution and the precise interpreter adaptations.
