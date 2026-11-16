# Graphics roadmap

**Implemented:** X16 bitmap fonts, a software text renderer, a consistent palette,
and 1x–4x integer scaling through UEFI Graphics Output Protocol (GOP) blits.
See [Character sets](Character-sets) for commands and cell sizes.

The backend uses GOP's standard blit pixel format; firmware handles its physical
framebuffer format, row stride and PixelBltOnly modes. BASIC EFI does not write
native framebuffer addresses. An unavailable or failing GOP falls back to the
firmware text console. INFO reports which backend is active.

## Next steps

Separate virtual bitmap/sprite memory and drawing operations can build on this
renderer. Test clipping, overlap, transparency, bounds and scrolling against an
independent in-memory reference before adding new graphics capabilities. Direct
framebuffer writes, if later needed for speed, would require explicit checks of
stride, allocation size and pixel format; they are not part of the current path.

This preserves existing text/color PEEK/POKE addresses. BASIC programs should
describe the virtual machine rather than depend on PCI addresses or a particular
GPU's memory layout.

## The support boundary

| Feature | Approach |
|---|---|
| Custom font and scalable text | CPU rendering into a firmware-provided surface |
| Pixels, lines, rectangles and software sprites | Portable software graphics |
| Different color layouts and padded scanlines | Explicit conversion and stride tests |
| All graphics output | GOP blits, without direct framebuffer access |
| Arbitrary GPU VRAM allocation | Outside this plan |
| Hardware 2D/3D acceleration or shaders | Separate vendor-driver project |
| GPU reset, power management, hot-plug | Outside this small EFI application's scope |

A framebuffer is the display surface the firmware makes available. It is not
a promise of access to all dedicated VRAM, nor a universal acceleration API.
Shared-memory and discrete GPUs can both provide a firmware display interface.

The relevant standard is the UEFI specification's
[Graphics Output Protocol](https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#graphics-output-protocol).
In particular, row stride can exceed visible width, pixel formats differ, and
PixelBltOnly modes have no directly writable linear framebuffer.
