# Architecture and attribution

The original Microsoft 6502 BASIC interpreter runs on a cycle-stepped 6502 core
inside a host application. Its virtual address space is 64 KiB, with BASIC ROM
at `$C000`, workspace below `$8000`, and separate virtual text/color regions.

The shared host code provides the editable screen, interpreter external-call
traps, program text import/export and the CALC workbench. A platform frontend
supplies keyboard, rendering and constrained filesystem operations. There are
currently EFI and POSIX-terminal frontends.

This separation is useful for portability: another host does not require a
second BASIC implementation. It still requires a suitable compiler, boot path,
memory model and platform adapter. Legacy BIOS and 16-bit x86 are meaningful
projects, not compile flags already supported here.

## Source lineage

- [Microsoft BASIC-M6502](https://github.com/microsoft/BASIC-M6502), MIT license.
- [rene6502's cc65 adaptation](https://github.com/rene6502/BASIC-M6502-CC65),
  with the upstream minimal Commodore-derived configuration.
- [floooh/chips](https://github.com/floooh/chips), 6502 CPU core under the zlib license.
- GNU EFI initialization/support code, with notices in `licenses/gnu-efi.txt`.

Exact upstream revisions and transformations are recorded in NOTICE.md and
`vendor/*/UPSTREAM_COMMIT`. CI verifies vendored file checksums offline.

The minimal BASIC configuration omits external file channels, time variables
and hardware-dependent random-number seeding. Our build changes line width,
input capacity and the banner, fixes a signed-index limitation for long stored
lines, and exports integration symbols. The original arithmetic is preserved.

This is an independent project, not an official Microsoft or Commodore product.
It includes no downloaded C64 ROM image. Project additions are under MIT; each
upstream component retains its own license and attribution.
