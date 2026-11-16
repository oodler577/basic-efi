# BASIC EFI development

Keep the original Microsoft interpreter and our host integration clearly
distinguished. Preserve upstream licenses and pinned vendor files. Explain any
ROM transformation in NOTICE.md and test its externally visible behavior.

The default branch is main. User-facing documentation lives at
https://github.com/oodler577/basic-efi/wiki; keep its source pages in docs/wiki
so documentation edits are reviewable alongside code.

File operations must stay inside the loaded-image volume's BASIC directory.
Do not add raw host disk access, firmware variable writes, implicit overwrites,
or native pointers accessible through PEEK/POKE. Test only against disposable
image files. The production binary must not contain the QEMU debug-port write.

Run make test for changes to behavior. For memory or parser changes also run
make test-sanitize and the affected fuzz target. EFI/platform or packaging
changes require make test-efi. CI must exercise actual EFI boot and persisted
file contents, not just whether compilation succeeds. Preserve diagnostic
logs and failure screenshots as CI artifacts.

BITS currently controls the separate CALC workbench. Do not claim it widens
BASIC's arithmetic. Physical hardware, IA32 and legacy BIOS support remain
unverified unless new evidence is recorded.

Graphics direction: retain stable virtual text/color memory and add UEFI GOP
rendering behind that interface. Start with portable framebuffer/blit output
and software graphics. Vendor-specific acceleration and arbitrary GPU VRAM
access require a separate proposal and compatibility plan.
