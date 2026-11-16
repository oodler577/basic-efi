# IEC and ZoomFloppy roadmap

**Feasibility proposal only. No IEC adapter support is implemented or included
in the test release.**

ZoomFloppy is a plausible way to connect real Commodore IEC drives to BASIC
EFI. Its xum1541 microcontroller firmware implements the adapter side of the
bus protocol. The PC sends USB commands; it need not reproduce IEC timing by
toggling native PC pins. ZoomFloppy is not a USB mass-storage device and cannot
be opened through the current FAT-file backend.

UEFI's USB I/O protocol exposes control and bulk transfers. An EFI transport
could implement the xum1541 USB protocol while retaining Boot Services. The
host-side OpenCBM libusb transport cannot simply be linked into an EFI program.
This is an engineering assessment, not evidence of operation on any tested PC.

## Proposed milestones

1. Enumerate USB interfaces and identify a supported xum1541 device, firmware
   revision and endpoints. Report information without changing adapter firmware.
2. Add bounded transfers, cancellation, unplug/error recovery and IEC status.
   Test a mock transport before accessing a real adapter.
3. Detect one drive; read its status and directory with ordinary IEC commands.
   Keep the existing FAT BASIC folder as the default storage location.
4. Read files through an explicitly selected IEC device. Decide separately how
   our numbered text format and Commodore tokenized PRG files are represented.
5. Add explicit SAVE support, then consider additional drives and DOS commands.

A device selector could eventually make `LOAD "NAME",8` refer to an actual
1541. That syntax currently means the boot-volume BASIC folder. Changing its
meaning requires an explicit selection mechanism and clear HELP output.
Reading a C64 PRG does not make its BASIC dialect, machine code or games compatible.

## Boundaries and validation

- Real testing needs a ZoomFloppy, an IEC cable, a powered drive and expendable
  test media. USB passthrough in QEMU is useful, followed by physical UEFI tests.
- Firmware USB enumeration, endpoint behavior, timeouts and disconnect recovery
  are the main unknowns. Do not infer compatibility from generic UEFI support.
- Start with ordinary IEC transfers. Fast loaders, nibbling, copying protected
  disks, IEEE-488, tape, JiffyDOS timing and firmware flashing are separate work.
- OpenCBM identifies its code as GPLv2. Any source reuse needs an explicit
  licensing decision; do not silently import it into the project's MIT code.

## Primary references

- [ZoomFloppy hardware and overview](https://github.com/go4retro/ZoomFloppy)
- [OpenCBM architecture and supported devices](https://github.com/OpenCBM/OpenCBM)
- [xum1541 firmware notes](https://github.com/OpenCBM/OpenCBM/blob/master/xum1541/README.txt)
- [UEFI USB I/O protocol](https://uefi.org/specs/UEFI/2.11/17_Protocols_USB_Support.html)
