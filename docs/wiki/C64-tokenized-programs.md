# C64 tokenized BASIC programs

BASIC EFI imports and exports standard C64 BASIC V2 PRG files for the supported
language subset. It translates C64 keyword tokens into this build's Microsoft
BASIC tokens and relocates line links into the virtual BASIC workspace.
It does not contain the C64 ROM, KERNAL, VIC-II or SID.

## Try the included program

```basic
LOAD "C64HELLO.PRG"
LIST
RUN
SAVE "COPY.PRG"
```

Put other PRGs in the boot USB's `BASIC` folder. Use `LOADPRG "NAME"` to explicitly
select PRG format when the filename has no `.PRG` extension. `SAVEPRG "NAME"`
exports that format too. The extension check is case-insensitive. Plain LOAD
also recognizes a file beginning with the standard `$0801` load address.
FAT names retain the normal 64-character/simple-filename rules.

LOAD never runs the program automatically. A successful PRG load clears old
variables, arrays, DATA position and CONT state, and selects PETSCII if the
screen was in ISO mode. An existing PETSCII LOWER selection is preserved.
The file is validated before replacing the current program or variables.

## Read a PRG from an IEC drive

```basic
IEC ON
IEC STATUS
DIR
LOADPRG "HELLO",8
LIST
RUN
IEC OFF
SAVEPRG "FROMIEC.PRG"
```

Commodore disk filenames do not normally have extensions: LOADPRG explicitly
opens a **PRG** file as `0:NAME,P,R`, on secondary channel 2. `LOAD "NAME.PRG"`
does the same, but the `.PRG` characters must actually be part of its disk name.
Plain `LOAD "NAME"` still requests a **SEQ** text file (`0:NAME,S,R`); byte-level
format detection happens only after that read and cannot change its file type.
There is no `,8,1` mode. A device suffix must match `IEC DEVICE` (8–30).

IEC stays **read-only**, including SAVEPRG. Use IEC OFF before exporting to FAT.
Missing files, malformed PRGs and transfer failures leave the current BASIC
program intact. The adapter/drive path is mock-tested inside real EFI, with
physical hardware still unverified. See [IEC and ZoomFloppy](IEC-and-ZoomFloppy-roadmap).

## Supported language and limits

| Feature | Behavior |
|---|---|
| BASIC loops, arrays, strings, DATA/READ, DEF FN, GOSUB | Supported by the existing interpreter; tokens translated by keyword name |
| Quoted text, DATA fields and REM text | Nonzero bytes preserved exactly, including PETSCII controls and bytes resembling tokens |
| PI token `$FF` | Preserved |
| PEEK / POKE | Operate on BASIC EFI's virtual map; C64 addresses are **not** translated |
| INPUT#, PRINT#, OPEN, CLOSE, CMD | Rejected outside literal text: no runtime file channels |
| SYS, USR, WAIT | Rejected outside literal text: C64 machine code and hardware timing unavailable |
| Tokenized LOAD, SAVE, VERIFY | Rejected in PRGs; use the direct-mode monitor commands |
| Extension tokens `$CC`–`$FE` | Rejected outside literal text |
| TI, TI$, ST | C64 clock/status behavior is not implemented; these names are not special variables here |
| C64 screen/color/SID/VIC addresses | No hardware emulation or memory remapping |
| Load address | Exactly `$0801`, stored as bytes `01 08` |
| Lines | Strictly increasing numbers 0–63999; at most 240 tokenized body bytes each |
| Size | At most 30,000 bytes of linked program data, plus the two-byte file header |
| Layout | Contiguous, correctly linked lines and one final zero link; no trailing payload |

A valid PRG can still report a BASIC syntax/runtime error or depend on
unimplemented C64 behavior. This feature supports portable BASIC programs;
it does not make C64 games compatible. Machine-code loaders, appended binaries,
relocated/obfuscated programs, cartridge ROMs and D64 images are not supported.
No hardware is accessed merely by importing a PRG.

## Saving and editing

SAVEPRG rewrites links for `$0801` and translates executable tokens back to C64
values. Accepted imports can be exported byte-for-byte before editing. Writes
refuse existing names, and conversion errors create no file. Strings and other
literal bytes are never transcoded: choose PETSCII when authoring a C64 PRG.
The format has no metadata to distinguish PET upper/graphics from PET lower.

Use `SAVE "NAME.BAS"` for readable text when representable. Text export rejects
embedded control bytes or lines that exceed text-import limits and suggests
SAVEPRG. Text export is not a byte-preserving PRG backup (for example, PI becomes
a numeric expression). Keep a PRG copy when exact bytes matter.

The screen is an 80-column Microsoft BASIC editor, not the C64 screen editor.
LIST can execute display controls embedded in strings, and high bytes in DATA
or REM may be rendered as keyword text by the original interpreter's lister.
Re-entering that displayed line can change it. Binary saving preserves stored
bytes without going through LIST or the screen.

The supplied `C64HELLO.PRG` is an original two-line example, generated from
hand-authored BASIC V2 tokens in `tests/prg_fixture.py`. No third-party program
or C64 ROM image is bundled.
