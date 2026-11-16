CC ?= cc
PYTHON ?= python3
EFI_ROOT ?= /usr
EFI_INC = $(EFI_ROOT)/include/efi
EFI_LIB = $(EFI_ROOT)/lib
TEXT = src/charset.c src/render.c
IEC = src/iec.c src/iec_monitor.c
COMMON = src/core.c src/screen.c src/prg.c $(TEXT) $(IEC)
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror
CPPFLAGS += -Isrc -Ibuild
SANITIZERS ?= address,undefined
SAN_FLAGS = -O1 -g -std=c11 -Wall -Wextra -Werror -fno-omit-frame-pointer -fsanitize=$(SANITIZERS)
EFI_FLAGS = $(CFLAGS) $(CPPFLAGS) -I$(EFI_INC) -I$(EFI_INC)/x86_64 -I$(EFI_INC)/protocol -ffreestanding -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -fpic -fshort-wchar -mno-red-zone -fno-stack-protector -fno-strict-aliasing -fno-builtin -DGNU_EFI_USE_MS_ABI -DEFI_BUILD
EFI_COMMON = build/core.efi.o build/screen.efi.o build/prg.efi.o build/charset.efi.o build/render.efi.o build/iec.efi.o build/iec_monitor.efi.o build/efi_usb.efi.o build/runtime.efi.o
EFI_OBJS = $(EFI_COMMON) build/efi.efi.o

.PHONY: all rom desktop efi test test-sanitize test-efi fuzz clean
all: desktop efi
rom: build/rom.h
build/fonts.h: tools/build_fonts.py vendor/x16-charset/petscii.s vendor/x16-charset/iso-8859-15.s
	$(PYTHON) tools/build_fonts.py
build/rom.h: vendor/msbasic/m6502-min.s vendor/msbasic/m6502-cbm.cfg tools/build_rom.py
	$(PYTHON) tools/build_rom.py
desktop: build/basic-efi
build/basic-efi: build/rom.h build/fonts.h $(COMMON) src/desktop.c src/basic.h src/charset.h src/iec.h vendor/chips/m6502.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(COMMON) src/desktop.c -o $@
build/core.efi.o: vendor/chips/m6502.h
build/basic-efi build/basic-efi-sanitized build/core.efi.o build/prg.efi.o build/fuzz-core: src/prg.h
build/%.efi.o: src/%.c src/basic.h src/charset.h src/iec.h src/efi_usb.h build/rom.h build/fonts.h
	$(CC) $(EFI_FLAGS) -c $< -o $@
build/basic.so: $(EFI_OBJS)
	ld -nostdlib --no-undefined -znocombreloc -T $(EFI_LIB)/elf_x86_64_efi.lds -shared -Bsymbolic $(EFI_LIB)/crt0-efi-x86_64.o $(EFI_OBJS) -L$(EFI_LIB) -lefi -lgnuefi -o $@
efi: build/BOOTX64.EFI
build/efi.debug.o: src/efi.c src/basic.h src/charset.h src/iec.h src/efi_usb.h build/rom.h build/fonts.h
	$(CC) $(EFI_FLAGS) -DBASIC_DEBUG_PORT -c $< -o $@
build/basic-debug.so: $(EFI_COMMON) build/efi.debug.o
	ld -nostdlib --no-undefined -znocombreloc -T $(EFI_LIB)/elf_x86_64_efi.lds -shared -Bsymbolic $(EFI_LIB)/crt0-efi-x86_64.o $^ -L$(EFI_LIB) -lefi -lgnuefi -o $@
build/BOOTTEST.EFI: build/basic-debug.so
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynstr -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $< $@
build/BOOTX64.EFI: build/basic.so
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynstr -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $< $@
build/test-screen: tests/test_screen.c src/screen.c $(TEXT) build/fonts.h src/basic.h src/charset.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_screen.c src/screen.c $(TEXT) -o $@
test: desktop build/test-screen build/test-charset build/test-iec build/test-prg
	./build/test-screen
	./build/test-charset
	./build/test-iec
	./build/test-prg
	$(PYTHON) -m unittest discover -s tests -p 'test_*.py' -v
build/basic-efi-sanitized: build/rom.h build/fonts.h $(COMMON) src/desktop.c src/basic.h src/charset.h src/iec.h vendor/chips/m6502.h
	$(CC) $(SAN_FLAGS) $(CPPFLAGS) $(COMMON) src/desktop.c -o $@
build/test-screen-sanitized: tests/test_screen.c src/screen.c $(TEXT) build/fonts.h src/basic.h src/charset.h
	mkdir -p build
	$(CC) $(SAN_FLAGS) $(CPPFLAGS) tests/test_screen.c src/screen.c $(TEXT) -o $@
test-sanitize: build/basic-efi-sanitized build/test-screen-sanitized build/test-charset-sanitized build/test-iec-sanitized build/test-prg-sanitized
	./build/test-screen-sanitized
	./build/test-charset-sanitized
	./build/test-iec-sanitized
	./build/test-prg-sanitized
	BASIC_BINARY=$(CURDIR)/build/basic-efi-sanitized $(PYTHON) -m unittest discover -s tests -p 'test_*.py' -v
build/fuzz-core: tests/fuzz_core.c build/rom.h build/fonts.h $(COMMON) src/basic.h src/charset.h src/iec.h
	clang $(SAN_FLAGS) $(CPPFLAGS) -fsanitize=fuzzer tests/fuzz_core.c src/screen.c src/prg.c $(TEXT) $(IEC) -o $@
build/fuzz-screen: tests/fuzz_screen.c src/screen.c $(TEXT) build/fonts.h src/basic.h src/charset.h
	mkdir -p build
	clang $(SAN_FLAGS) $(CPPFLAGS) -fsanitize=fuzzer tests/fuzz_screen.c src/screen.c $(TEXT) -o $@
fuzz: build/fuzz-core build/fuzz-screen build/fuzz-iec build/fuzz-prg
test-efi: efi build/BOOTTEST.EFI build/BOOTIEC.EFI
	$(PYTHON) tests/run_efi_tests.py
clean:
	rm -rf build

build/test-charset: tests/test_charset.c src/screen.c $(TEXT) build/fonts.h src/basic.h src/charset.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_charset.c src/screen.c $(TEXT) -o $@
build/test-charset-sanitized: tests/test_charset.c src/screen.c $(TEXT) build/fonts.h src/basic.h src/charset.h
	$(CC) $(SAN_FLAGS) $(CPPFLAGS) tests/test_charset.c src/screen.c $(TEXT) -o $@

build/test-iec: tests/test_iec.c tests/mock_xum.c src/iec.c src/iec.h tests/mock_xum.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_iec.c tests/mock_xum.c src/iec.c -o $@
build/test-iec-sanitized: tests/test_iec.c tests/mock_xum.c src/iec.c src/iec.h tests/mock_xum.h
	mkdir -p build
	$(CC) $(SAN_FLAGS) $(CPPFLAGS) tests/test_iec.c tests/mock_xum.c src/iec.c -o $@
build/fuzz-iec: tests/fuzz_iec.c src/iec.c src/iec.h tests/mock_xum.c tests/mock_xum.h
	mkdir -p build
	clang $(SAN_FLAGS) $(CPPFLAGS) -fsanitize=fuzzer tests/fuzz_iec.c tests/mock_xum.c src/iec.c -o $@
build/efi.iec.o: src/efi.c src/basic.h src/charset.h src/efi_usb.h src/iec.h build/rom.h build/fonts.h
	$(CC) $(EFI_FLAGS) -DBASIC_DEBUG_PORT -DBASIC_IEC_FIXTURE -c $< -o $@
build/fixture.efi.o: tests/efi_usb_fixture.c tests/mock_xum.h src/efi_usb.h src/iec.h build/rom.h
	$(CC) $(EFI_FLAGS) -c $< -o $@
build/mock_xum.efi.o: tests/mock_xum.c tests/mock_xum.h src/iec.h build/rom.h
	$(CC) $(EFI_FLAGS) -c $< -o $@
build/basic-iec-test.so: $(EFI_COMMON) build/efi.iec.o build/fixture.efi.o build/mock_xum.efi.o
	ld -nostdlib --no-undefined -znocombreloc -T $(EFI_LIB)/elf_x86_64_efi.lds -shared -Bsymbolic $(EFI_LIB)/crt0-efi-x86_64.o $^ -L$(EFI_LIB) -lefi -lgnuefi -o $@
build/BOOTIEC.EFI: build/basic-iec-test.so
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynstr -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $< $@

build/test-prg: tests/test_prg.c tests/prg_checks.h src/prg.c src/prg.h build/rom.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_prg.c src/prg.c -o $@
build/test-prg-sanitized: tests/test_prg.c tests/prg_checks.h src/prg.c src/prg.h build/rom.h
	$(CC) $(SAN_FLAGS) $(CPPFLAGS) tests/test_prg.c src/prg.c -o $@
build/fuzz-prg: tests/fuzz_prg.c tests/prg_checks.h src/prg.c src/prg.h build/rom.h
	clang $(SAN_FLAGS) $(CPPFLAGS) -fsanitize=fuzzer tests/fuzz_prg.c src/prg.c -o $@
