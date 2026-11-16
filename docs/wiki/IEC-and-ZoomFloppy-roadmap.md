# IEC and ZoomFloppy

**Implemented experimentally: read-only ZoomFloppy/xum1541 support in the EFI
build. Tested against a mock USB peer and inside QEMU/OVMF; no physical adapter,
drive or PC has been verified.** The earlier `v0.1.0-test.2` release predates IEC.
Choose a release whose notes explicitly include the IEC backend.

The application sends ordinary IEC operations through the adapter's USB
protocol. The adapter handles bus timing. UEFI supplies the USB controller and
bus drivers; BASIC EFI retains Boot Services and uses USB_IO control and bulk
transfers. ZoomFloppy is not a mass-storage device or a FAT volume.

## Commands

| Command | Behavior |
|---|---|
| `IEC` | Report the selected storage and connection state |
| `IEC ON` | Find exactly one supported adapter and select read-only IEC storage |
| `IEC OFF` | Finish a healthy session and select the FAT BASIC folder |
| `IEC DEVICE 9` | Select drive address 8–30; initial address is 8 |
| `IEC STATUS` | Read DOS status channel 15 from the selected drive |
| `DIR` | Read and display that drive's directory while IEC is selected |
| `LOAD "HELLO",8` | Read numbered source text from a SEQ file on the selected drive |
| `LOADPRG "HELLO",8` | Read a tokenized C64 BASIC V2 PRG from the selected drive |
| `SAVE` / `SAVEPRG` | Rejected while IEC is selected, including after a transfer failure |

These are direct-mode monitor commands. They do not add BASIC OPEN/CLOSE,
PRINT#/INPUT#, arbitrary DOS commands or a JiffyDOS command interpreter.
An optional LOAD device suffix must match `IEC DEVICE`; omitting it uses the
selected drive. `IEC DEVICE` alone does not enable IEC or change FAT routing.

The original FAT folder remains the default after boot. A failed initial
`IEC ON` leaves it selected. Once IEC is selected, errors never switch storage
silently. `IEC OFF` is the explicit way back. Resetting BASIC with F9/RESET
does not change the selected storage or drive address.

## First hardware experiment

1. Boot the production EFI executable from a spare FAT32 USB stick using the
   [USB boot instructions](Booting-from-USB). Use expendable test media.
2. Connect one ZoomFloppy/xum1541 and a powered IEC drive with its normal IEC
   cable. Connect/power the equipment according to its hardware instructions.
3. Enter `IEC ON`. Supported adapters use USB ID `16d0:0504`, interface 0,
   protocol version 7 or 8, CBM capability, and standard 64-byte bulk endpoints
   `83`/`04`. IEEE-488 and tape modes are rejected.
4. Enter `IEC STATUS`, then `DIR`. A DOS error such as `74, DRIVE NOT READY`
   is useful diagnostic information. Address selection does not auto-probe drives.
5. Load an existing SEQ test file containing numbered BASIC text:

   ```basic
   IEC DEVICE 8
   LOAD "HELLO",8
   LIST
   RUN
   ```

6. To copy the loaded source to your USB stick, enter `IEC OFF`, then
   `SAVE "FROMIEC.BAS"`. Existing FAT files are never overwritten.
7. Enter `EXIT` to return to firmware.

A suitable SEQ file contains bytes such as `10 PRINT "HELLO FROM IEC"` followed
by CR, LF or CRLF, with no load address or BASIC tokens. Use an existing trusted
Commodore/OpenCBM workflow to prepare it; this release does not write IEC media.
Filenames are 1–16 letters, digits, internal spaces, dots, underscores or
dashes. ASCII lowercase is converted to PETSCII uppercase. Extended PETSCII
filenames, wildcards, paths, drive selectors inside names and DOS command syntax
are not accepted. The drive's unit 0 is used (`0:NAME,S,R`, `0:NAME,P,R` or `$0`).

Text imports use the same 240-character line and 30,000-byte source budget as
FAT imports. Files are validated completely before the current BASIC program
is cleared. Malformed PRGs, invalid text, missing files and transfer failures
leave that program intact. Select the matching character set before LOAD/RUN;
there is no automatic text transcoding. Directory names are displayed through
a restricted printable conversion so disk bytes cannot issue screen controls.

For a Commodore PRG file use `LOADPRG "HELLO",8`, or `LOAD "HELLO.PRG",8` if
`.PRG` is part of its actual disk name. PRG reads preserve the two-byte load
address and select file type P on channel 2. Use `IEC OFF` followed by
`SAVEPRG "FROMIEC.PRG"` to export it to FAT without losing literal bytes.
See [C64 tokenized programs](C64-tokenized-programs) for supported keywords and
validation limits. C64 machine-code games and hardware are not emulated.

## Failure and recovery

Esc or F9 during IEC I/O cancels the operation at the next transfer boundary;
F9 during I/O cancels without resetting the BASIC program. USB operations have
finite timeouts (normally at most three seconds per call), status polling is
bounded, and partial command writes are never automatically replayed.

After a USB error, timeout or cancellation, IEC remains selected but offline.
Check the adapter and drive, then use `IEC ON` to reconnect. The adapter's INIT
recovery can reset the attached IEC bus after an interrupted session; this can
affect other devices sharing that bus. Failed sessions deliberately retain the
adapter's recovery flag. `IEC OFF`/EXIT do not falsely mark an interrupted
adapter session as clean; a later client initialization must recover it.

If multiple supported adapters are connected, disconnect extras. If no adapter
is found, the firmware may not expose its USB_IO interface. Hot-plug behavior,
controller differences and physical timing remain hardware-test questions.
The desktop/POSIX frontend currently reports the IEC transport as unsupported.

## Validation and next milestones

- Protocol harness: versions/capabilities, initialization recovery, exact command
  framing, channel-15 errors, directory bounds, empty/short/128-byte-boundary
  reads, oversized files, every-transfer timeout injection and cancellation.
- Booted EFI test: real USB_IO enumeration and calling convention with a
  synthetic adapter, invalid descriptors, multiple adapters, removed handles,
  SEQ/PRG LOAD/RUN, invalid-file preservation, timeout/reconnect and exact FAT export.
- Tests confirm rejected IEC SAVE never creates a FAT file. Production image
  checks reject mock-adapter code and debug-port instructions.
- GCC/Clang, ASan/UBSan and a dedicated IEC libFuzzer target run in CI.

Next: record actual adapter/firmware/PC/drive results, refine compatibility,
then add explicitly enabled SEQ writes with no-overwrite behavior and verified
disk contents. C64 game hardware, fastloaders, JiffyDOS
timing, IEEE-488, tape, nibbling and firmware flashing remain separate work.

## References

- [ZoomFloppy hardware](https://github.com/go4retro/ZoomFloppy)
- [OpenCBM and xum1541](https://github.com/OpenCBM/OpenCBM)
- [UEFI USB protocol specification](https://uefi.org/specs/UEFI/2.11/17_Protocols_USB_Support.html)
- [Implementation protocol notes](https://github.com/oodler577/basic-efi/blob/main/docs/IEC-PROTOCOL.md)

The host implementation is original project code. It does not bundle OpenCBM's
GPL implementation or adapter firmware.
