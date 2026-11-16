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

In ISO mode this writes an A and sets its color. `PRINT PEEK(32768+Y*80+X)` reads the
current visible byte. Screen output and scrolling modify the same virtual cells,
so a subsequent PRINT or scroll may replace your drawing.

All 256 screen codes are retained and rendered. ISO codes index the X16 ISO font;
PET modes use Commodore screen codes (A is 1; reverse A is 129), distinct from
PRINT's PETSCII bytes. Color RAM exposes the low four bits, with a consistent
16-color palette in GOP and approximate firmware colors in text fallback.
See [Character sets](Character-sets) for mappings and reverse-video behavior.

## Example

```basic
10 PRINT CHR$(147)
20 FOR X=0 TO 79
30 POKE 32768+X,65+X-26*INT(X/26)
40 POKE 36864+X,X-16*INT(X/16)
50 NEXT X
```

In ISO mode this fills the top row with colored letters. The supplied `SCREEN.BAS` adds an
explanatory message. The control character produced by `CHR$(147)` clears the
screen. The host implements a documented subset of Commodore/X16 display controls.

## Future graphics

The renderer now draws X16 bitmap glyphs through GOP blits, with integer scaling
and a consistent palette. Separate virtual bitmap and sprite storage
can be designed later without exposing host pointers. See [Graphics roadmap](Graphics-roadmap).
