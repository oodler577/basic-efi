# IEC transport implementation notes

The implementation is split into `src/iec.c` (portable wire/state handling),
`src/efi_usb.c` (UEFI USB_IO), and `src/iec_monitor.c` (storage selection and
screen messages). The interpreter's existing text validation remains the
transaction boundary before replacing a BASIC program. These notes describe
the initial read-only milestone, not measured hardware compatibility.

## Wire contract

Numbers below are hexadecimal unless stated otherwise. USB ID is `16d0:0504`;
configuration 1, interface 0/alternate 0, vendor class FF/subclass 0/protocol 0.
Bulk IN is endpoint `83`, OUT `04`; both descriptor packet sizes are 64 bytes.
Enumeration reads descriptors only and refuses ambiguous or unsupported matches.
The application does not reconfigure USB, reset host ports or detach drivers.

| Operation | Packet / response |
|---|---|
| INIT | Control type A0, request 01, value/index 0, length 8 |
| INIT reply | Byte 0 version (decimal 7/8); byte 1 capabilities; byte 2 state |
| CBM capability | Bit 0 of capability byte |
| Unsupported modes | State bits 10 (IEEE-488), 20 (tape) |
| Recovery required | State bit 01; clear endpoint halt with type 02, request 01, value 0 and endpoint index |
| SHUTDOWN | Control type 20, request 03, no data |
| READ | Bulk OUT `08 10 length-lo length-hi`, then bulk IN data; no completion status |
| WRITE | Bulk OUT `09 mode length-lo length-hi`, then OUT bytes, then IN status |
| GET_EOI | Bulk OUT `17 00 00 00`, then IN status value 0/1 |
| CLEAR_EOI | Bulk OUT `18 00 00 00`, then IN status |
| Status | Three bytes: state, value-lo, value-hi; states 01 BUSY, 02 READY, 03 ERROR |

WRITE here sends bus addressing and filenames only; it is not a public disk
write operation. Mode 10 sends filename bytes; 12 asserts ATN; 13 asserts ATN
and performs the TALK turnaround. Address bytes are `20|device` for LISTEN,
`40|device` for TALK. Secondary bytes are `F0|channel` for OPEN, `E0|channel`
for CLOSE and `60|channel` for TALK. UNLISTEN is `3F`; UNTALK is `5F`.
Every WRITE completion must report exactly the number of bytes sent.

SEQ reads OPEN channel 2 with `0:NAME,S,R`; directory reads OPEN channel 0 with
`$0`. After UNLISTEN, DOS channel 15 is read before attempting file TALK.
Read errors with DOS codes >=20 are reported. CLEAR_EOI precedes each TALK;
128-byte chunks are followed by GET_EOI, including full-length final chunks.
A short read without EOI is a protocol failure, not a successful truncated file.
Healthy operations UNTALK and CLOSE/UNLISTEN. DOS status is checked again after
file closure. Disk directories contain a load address, then sequential BASIC
records; link values are treated as terminator/nonterminator, never pointers.

Transfers request a nonzero three-second timeout. Status loops allow at most
eight replies with 10 ms pacing; all loops are bounded by status count, file
capacity or input length. Esc/F9 are polled between transfers. There are no
blind USB retries. Transport failures invalidate the session while retaining
the monitor's IEC selection. The next INIT performs firmware recovery; do not
send SHUTDOWN after an interrupted transaction because it clears that flag.

USB_IO is looked up from the selected handle for each transfer, and the protocol
instance must still match. The UEFI transport uses the firmware's returned byte
count and result flags as well as EFI_STATUS. Clear-halt control requests also
reset endpoint data toggles in EDK II's USB bus implementation. Real firmware
behavior and unplug races still require hardware validation.

## Test architecture

`tests/mock_xum.c` is a test-only packet-consuming peer. Unit tests inject a
timeout at every USB call in a complete read and cancellation at every poll.
They also test firmware modes, short writes/status packets, status errors,
oversize files and all lengths 0–257 (including exact packet/chunk boundaries).

`BOOTIEC.EFI` installs a synthetic USB_IO interface, so enumeration, descriptor
checks, function-pointer ABI, monitor routing and the real interpreter run
inside OVMF. Its fixture is uninstalled before returning to firmware. It is
never the production executable and never included as a boot file in releases.
The fixture has no external file or device access. QEMU uses disposable FAT
images; no physical USB passthrough is used by CI. Dedicated fuzzing mutates
directory bytes, filenames, adapter replies and file data.

## Protocol references inspected

No OpenCBM implementation code is vendored or linked. Wire behavior was checked
against the following files in [OpenCBM](https://github.com/OpenCBM/OpenCBM):

| Path | Inspected Git blob |
|---|---|
| `opencbm/lib/plugin/xum1541/xum1541.c` | `b77528e1d3e276680870165064c1c72a79554fa7` |
| `opencbm/lib/plugin/xum1541/archlib.c` | `261e15e1cec149aa9b463aa0060eca6c37da8dfc` |
| `xum1541/commands.c` | `654e26395b928678672a76037238df23e460221e` |
| `xum1541/descriptor.c` | `6797f3da2703abf63b10fb7a263c44078329af1a` |
| `xum1541/iec.c` | `d3cb555e2c88817347f71eda2510b64636d9b837` |

Constants are also described in `xum1541/xum1541_types.h`. UEFI ABI declarations
come from the public [UEFI 2.11 USB specification, section 17.2](https://uefi.org/specs/UEFI/2.11/17_Protocols_USB_Support.html).
ABI order was cross-checked with EDK II `MdePkg/Include/Protocol/UsbIo.h`, and
clear-halt behavior with `MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBus.c` (blob
`b0dd19444e5b0159be052aaa4214f81cad68b414`). Protocol descriptions and numeric
interoperability constants are used by independently written project code.
