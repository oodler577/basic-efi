# Compatibility and troubleshooting

## Supported target versus tested systems

The target is x64 UEFI with a text console of at least 80 × 25. Automated boot
coverage uses OVMF in QEMU. Physical PCs, IA32 UEFI, ARM UEFI, legacy BIOS and
8086/286 systems have not been validated or implemented as additional targets.
An 8-bit emulated interpreter does not imply that the EFI application runs on
an 8-bit or 16-bit host.

| Symptom | Check |
|---|---|
| USB does not appear as bootable | FAT32 volume, `EFI/BOOT/BOOTX64.EFI` path, x64 UEFI entry |
| Security violation | Unsigned executable rejected by Secure Boot; use the VM setup |
| BASIC folder unavailable | Existing BASIC directory on the executable's own volume |
| SAVE fails but LOAD works | Read-only media, firmware filesystem restrictions or a full volume |
| SAVE says FILE EXISTS | Pick a new filename; overwrites are deliberately refused |
| Boot returns immediately | Confirm firmware text-console support and read any displayed error |
| Program hangs | Esc to break; F9 if virtual BASIC memory was corrupted |
| Text looks small or centered | Run INFO: GOP scales 8x8 glyphs to fit; firmware-text fallback still owns font size |
| C64 game will not load | No C64 KERNAL, VIC-II, SID, disk-image or machine-code game compatibility |
| Large integer loses precision in PRINT | PRINT uses classic BASIC arithmetic; use CALC for exact-width integers |
| Long program disappeared from screen | LIST it; display scrolling does not erase stored program memory |

## Reporting a problem

Include the source commit or package revision, exact command/program, expected
and actual behavior, and whether it occurred in QEMU, the desktop frontend or
physical hardware. For QEMU, include its version, OVMF firmware version, machine
and CPU model, and the relevant log from `build/qa`. For hardware, include the
PC model, firmware architecture/version and how the USB was prepared.

If the issue involves storage, reproduce it with an expendable program and a
copy of the image. Avoid publishing real private files or firmware dumps.

For a source/build issue, include compiler and GNU EFI versions. The test suite
can narrow a failure: `make test` exercises the shared core; `make test-efi`
adds real firmware boot and filesystem behavior.
