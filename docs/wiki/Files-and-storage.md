# Files and storage

```basic
DIR
SAVE "MYPROG.BAS"
NEW
LOAD "MYPROG.BAS"
LIST
RUN
```

SAVE and LOAD are direct-mode commands. They are not currently supported inside
running programs. LOAD does not RUN automatically.

The familiar optional device suffix is accepted: `LOAD "MYPROG.BAS",8` and
`SAVE "MYPROG.BAS",8`. It is a compatibility spelling for the same BASIC folder,
not an implementation of a Commodore serial bus or selectable disk unit.

## File format

Programs are ordinary printable ASCII text, one numbered BASIC line per text
line, with LF or CRLF endings. SAVE expands tokens back to readable text.
Tokenized PRG files, D64 disk images and JiffyDOS commands are not supported.

| Limit | Value |
|---|---|
| Source line length | 240 characters including number |
| Line numbers | 0–63999 |
| Import storage budget | Conservative 30,000 bytes before tokenization |
| Filename length | 1–64 characters |
| Filename characters | Letters, digits, `.`, `_`, `-` |

Names cannot start with a dot or contain directories. The desktop frontend uses
its selected folder; EFI uses the BASIC folder on its loaded-image volume.

## Protecting existing work

SAVE refuses to overwrite an existing name. Save revisions as `PROG2.BAS`,
`PROG3.BAS`, and so on. File renaming and deletion can be done from your normal
OS while BASIC is not using the USB/image.

LOAD validates the source structure and budget before clearing the current
program. A rejected or oversized import leaves the current program intact.
That validation does not prove a program is semantically correct: ordinary
BASIC syntax or runtime errors can still occur when the imported program runs.

Read-only media can be listed and loaded when the firmware permits it; SAVE
reports an I/O error. A missing BASIC folder leaves the interpreter usable but
disables file access. The application does not create an alternative directory
or search other volumes.

## Common messages

| Message | Meaning / response |
|---|---|
| FILE EXISTS | Choose a new filename |
| FILE NOT FOUND | Check DIR and spelling |
| FILE I/O ERROR | Check write protection, media availability and filesystem health |
| BASIC FOLDER UNAVAILABLE | Put a BASIC directory on the boot volume |
| INVALID BASIC TEXT | Check numbering, printable ASCII, line length and total size |
| FILE COMMANDS REQUIRE DIRECT MODE | Move SAVE/LOAD outside the running program |

After an I/O error, the interpreter remains available. Keep a backup of valuable
programs using your normal OS tools; the firmware filesystem is not a versioned
storage service.
