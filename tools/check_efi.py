#!/usr/bin/env python3
"""Validate PE layout and ROM content before an EFI binary reaches boot tests."""
import argparse
import pathlib
import struct
import subprocess

def check(path,production=True):
    data=pathlib.Path(path).read_bytes()
    def u16(offset):return struct.unpack_from('<H',data,offset)[0]
    def u32(offset):return struct.unpack_from('<I',data,offset)[0]
    assert data[:2]==b'MZ','missing DOS header'
    pe=u32(0x3c);assert data[pe:pe+4]==b'PE\0\0','missing PE signature'
    assert u16(pe+4)==0x8664,'not an x64 image'
    optional=pe+24
    assert u16(optional)==0x20b,'not PE32+'
    assert u16(optional+68)==10,'not an EFI application'
    header_size=u32(optional+60);entry=u32(optional+16)
    count=u16(pe+6);table=optional+u16(pe+20);sections=[]
    for i in range(count):
        at=table+i*40;name=data[at:at+8].rstrip(b'\0').decode()
        virtual_size,rva,raw_size,offset=struct.unpack_from('<IIII',data,at+8)
        assert rva>=header_size,f'{name} overlaps the PE header (RVA {rva:#x})'
        assert offset+raw_size<=len(data),f'{name} exceeds file'
        sections.append((name,rva,max(virtual_size,raw_size)))
    assert any(name=='.text' and rva<=entry<rva+size for name,rva,size in sections),'entry outside code'
    assert any(name=='.reloc' for name,_,_ in sections),'missing relocation section'
    assert b'MICROSOFT 6502 BASIC 1.1' in data,'missing BASIC ROM data'
    assert b'TYPE HELP FOR COMMANDS' in data,'missing native string constants'
    if production:
        disassembly=subprocess.check_output(['objdump','-d',str(path)],text=True)
        assert not any('out' in line and '$0xe9' in line for line in disassembly.splitlines()),'debug port in production'
    print(f'PASS: {path}: x64 EFI, {count} valid sections, ROM and strings present')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('path',type=pathlib.Path);p.add_argument('--debug',action='store_true')
    args=p.parse_args();check(args.path,not args.debug)
