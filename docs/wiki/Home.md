# BASIC EFI

A small, bootable Microsoft BASIC environment with a Commodore-inspired screen
editor. Turn on a VM or boot a spare USB stick, type a program, and RUN it.

**Start with a VM.** The current target is x64 UEFI. Physical hardware support
has not yet been established. This is an experimental interpreter environment,
with original Microsoft 6502 BASIC running inside an isolated virtual machine.

- [Getting started](Getting-started): VM setup and your first session.
- [Booting from USB](Booting-from-USB): file layout, one-time boot, Secure Boot and exit.
- [BASIC and screen editing](BASIC-and-screen-editing): how numbered programs and the editable display work.
- [Files and storage](Files-and-storage): SAVE, LOAD, DIR, file format and error recovery.
- [Character sets](Character-sets)
- [Virtual video memory](Virtual-video-memory): text bytes, colors and examples.
- [Integer widths](Integer-widths): the precise meaning of 8/16/32/64/native.
- [IEC and ZoomFloppy roadmap](IEC-and-ZoomFloppy-roadmap)
- [Graphics roadmap](Graphics-roadmap): UEFI framebuffer graphics and the GPU support boundary.
- [Building and testing](Building-and-testing): local builds, sanitizers, fuzzing and EFI boot tests.
- [Compatibility and troubleshooting](Compatibility-and-troubleshooting): current limits and common problems.
- [Architecture and attribution](Architecture-and-attribution): interpreter, platform boundary and licenses.

The [source repository](https://github.com/oodler577/basic-efi) includes these
pages under `docs/wiki`, so documentation changes can be reviewed with code.
The [Actions page](https://github.com/oodler577/basic-efi/actions) contains CI
results and downloadable build artifacts. A green run is evidence for the
tested VM configurations, not a claim that every physical PC is supported.
