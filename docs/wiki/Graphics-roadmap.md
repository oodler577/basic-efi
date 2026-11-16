# Graphics roadmap

**Planned, not implemented:** render the stable virtual display through UEFI's
Graphics Output Protocol (GOP).

The current EFI frontend uses the firmware text console. GOP offers a standard
way to query graphics modes and draw pixels before an OS boots. That is a
manageable route to more interesting graphics without maintaining native GPU
drivers for each vendor and generation.

## Proposed sequence

1. A software renderer with a licensed bitmap font, palette and integer scaling.
2. A GOP backend that validates dimensions, stride, framebuffer size and pixel
   format. Support firmware blits where direct framebuffer access is unavailable.
3. Keep the current text console as a fallback. Report the selected backend in INFO.
4. Design separate virtual bitmap/sprite memory and simple drawing operations.
   Test clipping, overlap, transparency, bounds and scrolling against an independent
   in-memory reference renderer before booting it in EFI.

This preserves existing text/color PEEK/POKE addresses. BASIC programs should
describe the virtual machine rather than depend on PCI addresses or a particular
GPU's memory layout.

## The support boundary

| Feature | Approach |
|---|---|
| Custom font and scalable text | CPU rendering into a firmware-provided surface |
| Pixels, lines, rectangles and software sprites | Portable software graphics |
| Different color layouts and padded scanlines | Explicit conversion and stride tests |
| Direct framebuffer unavailable | GOP blit fallback |
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
