# Getting started

Use the packaged VM image first. It runs without attaching your computer's
physical disks to the guest.

## Obtain a build

Open the repository's [Actions page](https://github.com/oodler577/basic-efi/actions),
select a successful **Torture CI** run, and download the Ubuntu 24.04 EFI artifact.
Inside is `dist/basic-efi.zip`, containing the source, example programs, USB
folder layout and a clean 64 MiB FAT32 VM image. GitHub may require you to sign
in to download Actions artifacts. Builds are retained for 30 days.

Alternatively, build from source using [Building and testing](Building-and-testing).

## Launch QEMU

Install QEMU, Python 3 and an OVMF 4 MiB firmware code/variables pair. On a Linux
system with the standard OVMF paths, run from the extracted package:

```sh
python3 tools/run_vm.py
```

For Windows, supply the actual paths to QEMU and your matching firmware pair:

```powershell
py tools/run_vm.py --qemu "C:\Program Files\qemu\qemu-system-x86_64.exe" --ovmf-code "C:\firmware\OVMF_CODE_4M.fd" --ovmf-vars "C:\firmware\OVMF_VARS_4M.fd"
```

Windows itself has not been included in the test matrix yet. QEMU and OVMF are
separate dependencies, not files supplied in this project archive.

The launcher creates `vm-state/usb.img` on the first run and reuses it thereafter.
Your SAVE files live inside that image. Firmware variables are copied to a
temporary file. There is no guest network adapter, host directory share or
physical host disk attached.

## First program

At READY, type:

```basic
10 INPUT "YOUR NAME";N$
20 PRINT "HELLO ";N$
30 FOR I=1 TO 5
40 PRINT I;I*I
50 NEXT I
RUN
```

Supply your name when asked. To retain the program:

```basic
SAVE "HELLOME.BAS",8
```

Try `DIR`, `HELP`, and `INFO`. To run the supplied example, enter
`LOAD "HELLO.BAS"`, then `RUN`.

## Stop and resume later

Esc interrupts a running BASIC program. `EXIT` returns to the firmware menu.
Close the QEMU window to end the VM. A stored file survives because it is in
`vm-state/usb.img`; an unsaved in-memory program does not.

To reset the whole VM disk to the original examples, close QEMU and move
`vm-state/usb.img` aside. The next launch creates a new copy. Keep the old image
if it contains programs you may want later.
