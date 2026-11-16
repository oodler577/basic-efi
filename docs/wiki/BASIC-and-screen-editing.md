# BASIC and screen editing

The display is an editable 80 × 24 character surface, with a separate status
row. It is inspired by Commodore screen editing: cursor position determines
which visible line Enter submits.

## Direct commands and stored lines

```basic
PRINT 2+2
10 PRINT "FIRST VERSION"
RUN
LIST
```

An unnumbered BASIC statement runs immediately. A numbered line is stored.
Entering the same number replaces that line; entering only the number deletes
it. `RUN` starts the program, `LIST` displays it, and `NEW` clears it.

The interpreter supports its original arithmetic, strings, arrays, DATA/READ,
GOSUB/RETURN, loops, INPUT and GET. It retains historic BASIC limitations,
including the significance of the first two characters of variable names.
Use short, distinct names rather than relying on modern long-name semantics.

## Editing something already on the screen

After LIST, move up to the desired numbered line. Use Home to reach its beginning,
move to the character to change, type the replacement, then press Enter. BASIC
stores the edited line under its number. Editing a displayed line does not change
the stored program until Enter is pressed.

Typing uses overwrite mode. Backspace and Delete shift the rest of the logical
line left. Home and End move to logical-line boundaries. Wrapped lines may use
up to 240 characters, including the line number and spaces.

The program is stored independently of the visible screen. A long program can
scroll off the display without being lost. There is no shell history or scrollback
editor; LIST the needed lines again when they are no longer visible.

## Keyboard and environment commands

| Input | Effect |
|---|---|
| Arrows | Move through visible cells |
| Home / End | Beginning / end of logical line |
| Backspace / Delete | Delete left / at cursor |
| Enter | Submit the logical line under the cursor |
| Esc | Interrupt RUN, or abandon the current editor entry |
| F9 or RESET | Reset BASIC; unsaved program and variables are lost |
| CLS | Clear only the display |
| HELP | Show environment command help |
| INFO | Show implementation and storage information |
| EXIT | Leave the application |

`INPUT` is line-oriented and preserves the case of the response. `GET A$` reads
a single available key without waiting. For example:

```basic
10 GET A$:IF A$="" THEN 10
20 PRINT "KEY=";A$
```

Esc breaks running loops; CONT uses the original interpreter's continuation
rules. F9 is available if a program damages its own virtual BASIC workspace.
PEEK/POKE can corrupt that virtual workspace, but cannot address native host RAM.
