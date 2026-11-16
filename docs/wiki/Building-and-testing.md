# Building and testing

On an x64 Debian/Ubuntu development system, install a C compiler, make, binutils,
Python 3, cc65 and GNU EFI. Image/boot tests also need dosfstools, mtools, QEMU,
OVMF and Pillow (`python3-pil` on Debian/Ubuntu).

```sh
git clone git@github.com:oodler577/basic-efi.git
cd basic-efi
make all
make test
make test-sanitize
make test-efi
python3 tools/package.py
```

`EFI_ROOT` defaults to `/usr`; override `EFI_LIB` if GNU EFI's linker support is
installed elsewhere. `CA65` and `LD65` environment variables select assembler
executables. `CC=clang` selects Clang. The desktop executable is
`./build/basic-efi programs`; use an 80 × 25 or larger POSIX terminal.

The vendored Microsoft source is transformed into `build/basic.s` and assembled.
The transformation and licenses are documented in NOTICE.md. No network fetch
is required to build the interpreter itself.

## What CI checks

The **Torture CI** workflow runs on pushes, pull requests and manual dispatch:

- GCC and Clang on Ubuntu 22.04 and 24.04.
- AddressSanitizer and UndefinedBehaviorSanitizer with both compilers.
- Integration tests for BASIC, files, INPUT/GET, exact-width arithmetic and editing.
- 2,400 arithmetic expressions checked against Python integer results.
- 600 seeded program insertion/replacement/deletion operations checked against a model.
- 120 malformed imports checked for preservation of the existing program.
- 10,000 screen scrolls and 200,000 seeded editing events with boundary/canary checks.
- Coverage-guided libFuzzer runs against parsers, corrupted virtual program links,
  arbitrary screen-edit sequences, IEC replies, filenames, directories and PRG conversion.
- IEC protocol tests inject failure/cancellation at every transfer boundary,
  check DOS status and read-only filenames, and exercise EOF/size boundaries.
- PRG tests execute hand-authored C64 tokens, preserve PETSCII, reject unsupported
  keywords transactionally, check exact import/export and 20,000 corrupted records.
- Pinned vendor checksums and validation of PE headers, ROM data and production
  debug-port exclusion.
- Byte-for-byte repeatability of the EFI executable within each build environment.
- Actual EFI boot on QEMU PC/qemu64 and q35/max configurations, covering writable,
  read-only and missing-folder media, plus the uninstrumented production executable.
- A sixth EFI scenario installs a synthetic ZoomFloppy USB_IO interface, exercises
  enumeration, status/DIR/LOAD/RUN, timeout/reconnect and FAT export, and checks
  that rejected IEC writes never reach FAT. This is not physical adapter testing.
- Persisted FAT file contents, reset, break handling and return to the firmware menu.

## Fuzzing locally

Install Clang, then:

```sh
make fuzz
mkdir -p build/fuzz/core build/fuzz/screen build/fuzz/iec build/fuzz/prg build/crashes
cp tests/corpus/core/* build/fuzz/core/
cp tests/corpus/screen/* build/fuzz/screen/
cp tests/corpus/iec/* build/fuzz/iec/
cp tests/corpus/prg/* build/fuzz/prg/
./build/fuzz-core -max_total_time=60 -artifact_prefix=build/crashes/ build/fuzz/core
./build/fuzz-screen -max_total_time=60 -artifact_prefix=build/crashes/ build/fuzz/screen
./build/fuzz-iec -max_total_time=60 -artifact_prefix=build/crashes/ build/fuzz/iec
./build/fuzz-prg -max_total_time=60 -artifact_prefix=build/crashes/ build/fuzz/prg
```

Reproduce a discovered failure by passing its saved input file to the affected
fuzzer executable. Keep the minimized input as a regression before changing code.
Passing a time-bounded fuzz run does not prove the absence of all bugs.

## Boot diagnostics and artifacts

`make test-efi` creates disposable images under `build/qa`. Never point tests at
an image containing valuable work. The tests keep per-scenario debug logs and
screen captures. CI uploads these even after a failed job, when they exist.

The production EFI executable omits debug-port I/O. `BOOTTEST.EFI` is explicitly
for tests; `BOOTIEC.EFI` additionally contains the synthetic adapter. The
production-image auditor rejects both mock-adapter markers and debug-port writes.
The release package contains the production executable and clean
examples, not test-created programs or diagnostic screenshots.

The package has SHA-256 checksums and a source revision. Image creation is atomic:
a failed external tool leaves an existing output image intact. `--force` is
required to replace an image, and device/symlink targets are rejected.

Character tests cover all 256 glyphs in all three modes at scales 1–4, bounds,
reverse video and controls. The EFI suite independently compares every visible
pixel in the three 256-glyph tables to the pinned X16 source data.
