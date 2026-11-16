# C64 BASIC V2 PRG interoperability

User commands and compatibility limits are in
[the wiki source](wiki/C64-tokenized-programs.md).

`src/prg.c` is an original file-format converter. The `$80`–`$CB` keyword
vocabulary and `$FF` PI constant are interoperability data, not copied ROM
implementation. The native map is built by keyword name from the assembled
Microsoft BASIC's actual RESLST table, since its minimal PET-derived build
omits several C64 keywords and has different token numbers.

Each record has a little-endian next-line address, a little-endian line number,
a zero-terminated tokenized body, and finally a two-byte zero link. The importer
compares each link with its expected contiguous position; it never follows
untrusted addresses. It translates executable tokens while tracking quotes,
REM and DATA. Output is prepared in a separate bounded buffer. Only a fully
validated program is installed at `$0401`; native CLR then resets runtime state.
Export performs the inverse translation and validates virtual pointers before
any file is created. Neither operation uses C64 KERNAL routines or host pointers.

Primary references consulted:

- Commodore, *Commodore 64 Programmer's Reference Guide* (1982), BASIC keyword,
  memory-map and disk-file sections. [Scanned manual](https://www.commodore.ca/commodore-manuals/commodore-64-programmers-reference-guide/).
- [Original C64 BASIC source transcription](https://github.com/mist64/c64rom/tree/master/basic):
  `tokens.s` blob `a046b9097259b5a4c7dc4761711f5610cc6e43e3`,
  `token2.s` blob `d6dc3212f559aeeb2a300dbe0cac4e6ed63bfbc6`,
  and `code1.s` blob `3bd3377130cedbc5c60d451b9e249abbb556b662`.
  These were consulted to verify token order and record structure, not copied
  into the project. The project does not distribute the C64 ROM.
- [IEC transport references](IEC-PROTOCOL.md) remain applicable. PRG reads use
  `0:NAME,P,R` on secondary channel 2; the first two load-address bytes are
  retained. There are no new bus commands or disk-write operations.

Tests use hand-authored C64 token bytes independently of the converter's token
table. They execute programs under the actual native ROM, compare binary
round trips, exercise every supported token and rejected token, preserve all
literal byte values, and check capacities/truncation/corrupt links. The PRG
fuzzer checks both conversion directions with guarded buffers and requires
every accepted input to survive exact inverse conversion. EFI tests inspect
persisted FAT PRGs and exercise PRG reads through a synthetic USB_IO adapter.
