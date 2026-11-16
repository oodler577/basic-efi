# Character sets and text size

The EFI frontend includes the actual public-domain Commander X16 8x8 fonts.
The initial mode is ISO, preserving the earlier prototype's ASCII behavior.

| Direct-mode command | Characters |
|---|---|
| `CHARSET ISO` | X16 ISO-8859-15: ASCII, accented Latin letters, euro sign, ligatures and the X16 logo at 173 |
| `CHARSET PETSCII` | PETSCII uppercase and graphics |
| `CHARSET LOWER` | PETSCII lowercase and uppercase, with the remaining graphics |
| `CHARSET` | Report the current character set |

Selecting a mode with CHARSET clears the visible display, preserving the BASIC
program and variables. It does not transcode existing strings or saved files.
The X16 PET fonts include its documented one-pixel edge-block corrections to
the C64 glyphs. This is character compatibility, not X16 or C64 machine emulation.

## Size on screen

GOP rendering uses square pixels and the largest whole-number scale from 1x
through 4x that fits the current firmware resolution. The grid is 80 columns,
24 editable rows and one status row. It is centered without stretching.

| Firmware graphics resolution | Character cell |
|---|---|
| 640x480 | 8x8 pixels |
| 1280x800 | 16x16 pixels |
| 1920x1080 | 24x24 pixels |
| 2560x1440 or 3840x2160 | 32x32 pixels |

These sizes use the firmware's current graphics resolution, which can differ
from a monitor's native resolution. `INFO` reports the actual renderer and scale.
There is no 40-column mode or manual size command yet.

## PRINT codes and screen codes

PETSCII bytes sent to PRINT are translated into Commodore screen codes.
POKE writes a screen code directly, including reverse glyphs at 128–255.
In ISO mode, screen codes directly index the X16 ISO font.

| Example | Result |
|---|---|
| PETSCII: `PRINT CHR$(65)` | A |
| PETSCII: `POKE 32768,1` | A at the first cell |
| PETSCII: `POKE 32768,129` | Reverse A |
| PETSCII: `PRINT CHR$(211)` | Heart |
| ISO: `PRINT CHR$(164)` | Euro sign |
| ISO: `PRINT CHR$(233)` | e with acute accent |
| ISO: `POKE 32768,65` | A at the first cell |

Useful output controls are available inside running BASIC programs:

| `CHR$()` value | Action |
|---|---|
| 15 | Select ISO |
| 143 or 142 | Select PETSCII uppercase/graphics |
| 14 | Select PETSCII lowercase/uppercase |
| 18 / 146 | Reverse on / off; Return also ends reverse |
| 147 / 19 | Clear / home |
| 17 / 145 / 29 / 157 | Down / up / right / left |
| 128 | Print the next byte as a glyph without interpreting its control action |

The usual 16 Commodore foreground-color codes are supported: 144, 5, 28,
159, 156, 30, 31, 158, 129, 149, 150, 151, 152, 153, 154 and 155, in palette
order 0–15. These controls are a supported subset of the X16 editor, not a
complete KERNAL implementation. Quote-mode control entry, INSERT output,
background-color commands and the full X16 keyboard shortcut layout are absent.

Control-code charset changes reinterpret existing screen cells without clearing
them. Use `PRINT CHR$(15);CHR$(147)` to change to ISO and clear explicitly.

## Keyboard, files and fallbacks

In PET modes, unshifted PC letter keys generate unshifted PETSCII. In LOWER
mode, shifted letters generate PETSCII uppercase letters. In graphics mode,
both cases of PC letters enter letters; use CHR$ or POKE for graphics. Ctrl+N
selects LOWER and Ctrl+O selects ISO. The firmware must supply a character for
it to be typed; this project does not install a keyboard layout into firmware.

ISO input maps firmware Unicode to ISO-8859-15, including the euro sign and
Latin ligatures. The desktop terminal frontend accepts UTF-8 keyboard input;
batch stdin and BASIC files are byte-oriented. They are not UTF-8 files.
ASCII remains valid. Quoted strings may contain bytes 160–255. Select the
appropriate charset before LOAD and RUN; files have no encoding metadata.
Use CHR$ expressions for control bytes and portable ASCII source. PETSCII has
duplicate codes for some glyphs; re-editing a visible line canonicalizes those
aliases, just as conversion back from screen codes requires.

GOP draws the exact bundled glyphs through firmware blits. If unavailable or a
blit fails, the EFI frontend uses firmware text output. That fallback and the
desktop terminal use Unicode approximations: some graphics become `?`, and the
X16 logo becomes a diamond. Use EFI GOP for exact glyphs. No GPU-specific driver
or direct hardware VRAM access is required.

Try `LOAD "CHARSETS.BAS"` and `RUN` to view all three character tables; press a
key between tables. Source and license attribution are in the repository's
`NOTICE.md` and `licenses/x16-rom.md`.

Tokenized C64 PRG loading selects PETSCII from ISO, preserves an existing LOWER
selection, and keeps literal bytes unchanged. SAVEPRG preserves control bytes
that text files cannot represent. See [C64 tokenized programs](C64-tokenized-programs).
