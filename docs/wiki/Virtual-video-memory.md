# Virtual video memory

BASIC sees a deliberately simple virtual display. These are addresses inside
the emulated 64 KiB machine, not physical GPU addresses.

| Region | Decimal base | Size |
|---|---:|---:|
| Character cells | 32768 | 1920 bytes |
| Color cells | 36864 | 1920 bytes |

The display has 80 columns and 24 rows. The offset for a zero-based `(X,Y)` cell
is `Y*80+X`. For example:

```basic
X=10:Y=5
POKE 32768+Y*80+X,65
POKE 36864+Y*80+X,5
```

This writes an ASCII A and sets its color. `PRINT PEEK(32768+Y*80+X)` reads the
current visible byte. Screen output and scrolling modify the same virtual cells,
so a subsequent PRINT or scroll may replace your drawing.

Only printable ASCII is rendered as a cell character; unsupported values appear
as spaces. Color values use their low four bits. The palette is an approximation
mapped to firmware console colors, not the exact C64 palette.

## Example

```basic
10 PRINT CHR$(147)
20 FOR X=0 TO 79
30 POKE 32768+X,65+X-26*INT(X/26)
40 POKE 36864+X,X-16*INT(X/16)
50 NEXT X
```

This fills the top row with colored letters. The supplied `SCREEN.BAS` adds an
explanatory message. The control character produced by `CHR$(147)` clears the
screen; this is one of the small set of Commodore-style display controls supplied
by the host, not a complete PETSCII implementation.

## Future graphics

The text/color memory contract can remain stable when the renderer moves from
firmware text output to a pixel framebuffer. That would permit custom fonts,
scaling and a consistent palette. Separate virtual bitmap and sprite storage
can be designed later without exposing host pointers. See [Graphics roadmap](Graphics-roadmap).
