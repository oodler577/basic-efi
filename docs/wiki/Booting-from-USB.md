# Booting from USB

This is optional after VM testing. Physical PCs are still untested.

Use a spare FAT32 USB stick. Copy the contents of the packaged `usb` directory
to its root, producing these paths:

| USB path | Purpose |
|---|---|
| `EFI/BOOT/BOOTX64.EFI` | x64 removable-media boot application |
| `BASIC/HELLO.BAS` | Introductory BASIC program |
| `BASIC/SCREEN.BAS` | Virtual text/color memory example |

Do not overwrite an existing EFI application on a stick you already use for
something else. There is no need to flash the VM image or write raw disk sectors
for this copy-based setup.

Use the PC's **one-time UEFI boot menu** to select the USB. The precise key is
machine-specific. A legacy BIOS entry will not run an EFI executable.

The application is unsigned. If Secure Boot rejects it, use the VM path; this
guide does not require changing your physical machine's Secure Boot settings.
An x64 CPU alone is insufficient if its firmware is IA32 or BIOS-only.

## What is written

The application opens the existing `BASIC` directory on the same volume from
which its executable was loaded. SAVE creates a new file there. It does not
enumerate other filesystems, expose physical disks to BASIC, modify partitions,
or install anything on your OS disk. It does not call UEFI SetVariable to change
boot entries or firmware configuration.

The location matters: if you deliberately put the application on an internal
volume, that volume becomes its file-access volume. Use a spare USB, not your
internal EFI partition.

## Leaving BASIC

`EXIT` returns control to the firmware or EFI shell that launched the application.
That may show a menu rather than automatically starting the usual OS. Remove
the USB and restart or select your normal OS entry.

F9 resets only the virtual BASIC machine. It is not a PC reboot, and it discards
the unsaved BASIC program and variables.
