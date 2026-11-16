"""Hand-authored BASIC V2 token bytes; only the link envelope is generated."""
import struct

BLOCKED={0x84,0x92,0x93,0x94,0x95,0x98,0x9d,0x9e,0x9f,0xa0,0xb7}
def program(lines,base=0x0801):
    out=bytearray(struct.pack('<H',base));address=base
    for number,body in lines:
        assert b'\0' not in body
        address+=5+len(body)
        out+=struct.pack('<HH',address,number)+body+b'\0'
    return bytes(out)+b'\0\0'

HELLO=program([(10,b'\x99 "HELLO FROM C64 PRG"'),(20,b'\x80')])
IEC_HELLO=program([(10,b'\x99 "IEC C64 PRG"'),(20,b'\x80')])
MATH=program([
    (10,b'\x86 A(2)'),(20,b'\x83 4,5,6'),(30,b'\x81 I\xb20\xa42'),
    (40,b'\x87 A(I)'),(50,b'\x82 I'),(60,b'\x96 \xa5 F(X)\xb2X\xac2'),
    (70,b'\x8d 100'),(80,b'\x80'),
    (100,b'\x99 A(0)\xaaA(1)\xaaA(2);\xa5 F(3)'),(110,b'\x8e')])
