CC ?= cc
PYTHON ?= python3
EFI_ROOT ?= /usr
EFI_INC = $(EFI_ROOT)/include/efi
EFI_LIB = $(EFI_ROOT)/lib
COMMON = src/core.c src/screen.c
CFLAGS = -O2 -std=c11 -Wall -Wextra -Werror -Isrc -Ibuild
EFI_FLAGS = $(CFLAGS) -I$(EFI_INC) -I$(EFI_INC)/x86_64 -I$(EFI_INC)/protocol -ffreestanding -fpic -fshort-wchar -mno-red-zone -fno-stack-protector -fno-strict-aliasing -fno-builtin -DGNU_EFI_USE_MS_ABI -DEFI_BUILD
EFI_OBJS = build/core.efi.o build/screen.efi.o build/efi.efi.o build/runtime.efi.o

.PHONY: all rom desktop efi test clean
all: desktop efi
rom: build/rom.h
build/rom.h: vendor/msbasic/m6502-min.s tools/build_rom.py
	$(PYTHON) tools/build_rom.py
desktop: build/basic-efi
build/basic-efi: build/rom.h $(COMMON) src/desktop.c src/basic.h
	$(CC) $(CFLAGS) $(COMMON) src/desktop.c -o $@
build/%.efi.o: src/%.c src/basic.h build/rom.h
	$(CC) $(EFI_FLAGS) -c $< -o $@
build/basic.so: $(EFI_OBJS)
	ld -nostdlib -znocombreloc -T $(EFI_LIB)/elf_x86_64_efi.lds -shared -Bsymbolic $(EFI_LIB)/crt0-efi-x86_64.o $(EFI_OBJS) -L$(EFI_LIB) -lefi -lgnuefi -o $@
efi: build/BOOTX64.EFI
build/efi.debug.o: src/efi.c src/basic.h build/rom.h
	$(CC) $(EFI_FLAGS) -DBASIC_DEBUG_PORT -c $< -o $@
build/basic-debug.so: build/core.efi.o build/screen.efi.o build/efi.debug.o build/runtime.efi.o
	ld -nostdlib -znocombreloc -T $(EFI_LIB)/elf_x86_64_efi.lds -shared -Bsymbolic $(EFI_LIB)/crt0-efi-x86_64.o $^ -L$(EFI_LIB) -lefi -lgnuefi -o $@
build/BOOTTEST.EFI: build/basic-debug.so
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynstr -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $< $@
build/BOOTX64.EFI: build/basic.so
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynstr -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $< $@
test: desktop
	$(PYTHON) tests/test_basic.py
clean:
	rm -rf build
