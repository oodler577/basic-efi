import os,pathlib,struct,subprocess,tempfile,unittest
from prg_fixture import program,HELLO,MATH,BLOCKED
ROOT=pathlib.Path(__file__).resolve().parent.parent
BINARY=pathlib.Path(os.environ.get('BASIC_BINARY',str(ROOT/'build/basic-efi')))

class PrgTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=pathlib.Path(self.temp.name)
    def run_basic(self,commands):
        p=subprocess.run([str(BINARY),str(self.folder)],input=(commands+'\nEXIT\n').encode(),capture_output=True,timeout=20)
        self.assertEqual(p.returncode,0,p.stderr);return p.stdout
    def file(self,name,data):
        (self.folder/name).write_bytes(data)
    def rejected(self,data,message,name='BAD.PRG'):
        self.file(name,data)
        out=self.run_basic('10 PRINT "PRESERVED"\nA$="VARIABLE KEPT"\nLOAD "'+name+'"\nPRINT A$\nRUN')
        self.assertIn(message.encode(),out);self.assertNotIn(b'LOADED C64',out)
        self.assertIn(b'VARIABLE KEPT',out);self.assertIn(b'PRESERVED',out)
        return out
    def test_import_math_arrays_data_functions_and_subroutine(self):
        self.file('MATH.PRG',MATH)
        out=self.run_basic('LOAD "MATH.PRG"\nLIST\nRUN\nSAVEPRG "COPY.PRG"')
        self.assertIn(b' 15  6 ',out);self.assertIn(b'DEF FN F(X)=X*2',out)
        self.assertNotIn(b'ERROR',out);self.assertEqual((self.folder/'COPY.PRG').read_bytes(),MATH)
    def test_explicit_no_extension_and_signature_detection(self):
        self.file('C64',HELLO)
        out=self.run_basic('LOADPRG "C64",8\nRUN\nSAVEPRG "COPY"\nNEW\nLOAD "COPY"\nRUN')
        self.assertEqual(out.count(b'HELLO FROM C64 PRG'),2)
        self.assertEqual((self.folder/'COPY').read_bytes(),HELLO)
    def test_empty_program_and_variables_and_cont_are_cleared(self):
        self.file('EMPTY.PRG',b'\x01\x08\x00\x00')
        out=self.run_basic('10 STOP\nRUN\nA=99\nA$="OLD"\nDIM B(2)\nLOAD "EMPTY.PRG"\nPRINT A;LEN(A$)\nCONT\nSAVE "COPY.PRG"')
        self.assertIn(b' 0  0 ',out);self.assertIn(b"CAN'T CONTINUE",out)
        self.assertEqual((self.folder/'COPY.PRG').read_bytes(),b'\x01\x08\0\0')
    def test_all_supported_keyword_codes_and_pi_roundtrip(self):
        source=program([(c,b'\x8f HEADER:'+bytes([c])) for c in range(128,256)])
        # REM payload includes arbitrary keyword-looking bytes, including unsupported ones.
        self.file('REMS.PRG',source)
        self.run_basic('LOAD "REMS.PRG"\nSAVE "REMOUT.PRG"')
        self.assertEqual((self.folder/'REMOUT.PRG').read_bytes(),source)
        source=program([(c,bytes([c])) for c in list(range(0x80,0xcc))+[255] if c not in BLOCKED])
        self.file('TOKENS.PRG',source)
        out=self.run_basic('LOAD "TOKENS.PRG"\nSAVE "TOKOUT.PRG"')
        self.assertIn(b'LOADED C64 BASIC PRG',out)
        self.assertEqual((self.folder/'TOKOUT.PRG').read_bytes(),source)
        self.file('PI.PRG',program([(10,b'\x99 \xff')]))
        self.assertIn(b'3.14159265',self.run_basic('LOAD "PI.PRG"\nRUN'))
    def test_petscii_literals_data_colons_and_rem_are_preserved(self):
        raw=bytes(x for x in range(1,256) if x!=34)
        # Split across lines to respect the tokenized line limit.
        source=program([(10,b'\x8f '+raw[:150]),(20,b'\x99 "'+raw[150:]+b'"'),
                        (30,b'\x83 "\x9e:\x99",\x9e:\x99 "\x93"'),
                        (40,b'\x99 "UNCLOSED\xff')])
        self.file('BYTES.PRG',source)
        out=self.run_basic('LOAD "BYTES.PRG"\nSAVEPRG "EXACT.PRG"\nSAVE "LOSSY.BAS"')
        self.assertEqual((self.folder/'EXACT.PRG').read_bytes(),source)
        self.assertIn(b'TRY SAVEPRG',out);self.assertFalse((self.folder/'LOSSY.BAS').exists())
        self.file('STRING.PRG',program([(10,b'\x83 "\x99:\x9e"'),(20,b'\x87 A$'),(30,b'\x99 \xc3(A$);\xc6(A$)')]))
        out=self.run_basic('LOAD "STRING.PRG"\nRUN')
        self.assertIn(b' 3  153 ',out)
    def test_unsupported_tokens_and_extensions_are_transactional(self):
        for token in sorted(BLOCKED | {0xcc,0xfe}):
            with self.subTest(token=token):
                self.rejected(program([(10,b'\x99 "OK"'),(20,bytes([token])+b' 123')]),'UNSUPPORTED KEYWORD')
        out=self.rejected(program([(100,b'\x9e 2061')]),'SYS IN 100')
        self.assertNotIn(b'LOADING',out)
        self.rejected(program([(0,b'\x9e 2061')]),'SYS IN 0')
    def test_bad_addresses_links_numbers_and_trailing_payload(self):
        self.rejected(program([(10,b'\x80')],base=0x1001),'LOAD ADDRESS $0801')
        self.rejected(HELLO+b'\xea\xea','TRAILING DATA')
        for link in (0x0801,0x0800,0xffff,0x0810):
            bad=bytearray(HELLO);struct.pack_into('<H',bad,2,link)
            self.rejected(bytes(bad),'INVALID BASIC LINE LINK')
        for lines in ([(20,b'\x80'),(10,b'\x80')],[(10,b'\x80'),(10,b'\x80')],[(64000,b'\x80')]):
            self.rejected(program(lines),'LINE NUMBERS MUST INCREASE')
        self.rejected(program([(10,b'\x99\x0d1')]),'INVALID BASIC RECORD')
    def test_every_truncation_preserves_old_program(self):
        for n in range(len(HELLO)):
            self.rejected(HELLO[:n],'?PRG')
    def test_tokenized_line_and_program_size_boundaries(self):
        source=program([(0,b'\x8f'+b'X'*239),(63999,b'\x80')])
        self.file('LONG.PRG',source)
        out=self.run_basic('LOAD "LONG.PRG"\nSAVE "COPY.PRG"\nSAVE "LONG.BAS"')
        self.assertEqual((self.folder/'COPY.PRG').read_bytes(),source)
        self.assertIn(b'TRY SAVEPRG',out);self.assertFalse((self.folder/'LONG.BAS').exists())
        self.rejected(program([(10,b'\x8f'+b'X'*240)]),'EXCEEDS 240 BYTES')
        lines=[(i,b'\x8f'+b'X'*234) for i in range(124)]+[(124,b'\x8f'+b'X'*232)]
        source=program(lines);self.assertEqual(len(source),30002)
        self.file('MAX.PRG',source);self.run_basic('LOAD "MAX.PRG"\nSAVE "MAXCOPY.PRG"')
        self.assertEqual((self.folder/'MAXCOPY.PRG').read_bytes(),source)
        lines[-1]=(124,lines[-1][1]+b'X')
        self.rejected(program(lines),'EXCEEDS 30000 BYTES')
    def test_no_overwrite_and_export_rejects_unsupported_or_corrupt_program(self):
        self.file('KEEP.PRG',b'KEEP')
        out=self.run_basic('10 PRINT 42\nSAVE "KEEP.PRG"\nNEW\n10 LOAD "X"\nSAVEPRG "BAD.PRG"\nNEW\n10 PRINT 42\nPOKE 1025,1\nPOKE 1026,4\nSAVEPRG "BROKEN.PRG"')
        self.assertIn(b'FILE EXISTS',out);self.assertIn(b'UNSUPPORTED KEYWORD',out);self.assertIn(b'INVALID BASIC LINE LINK',out)
        self.assertEqual((self.folder/'KEEP.PRG').read_bytes(),b'KEEP')
        self.assertFalse((self.folder/'BAD.PRG').exists());self.assertFalse((self.folder/'BROKEN.PRG').exists())
    def test_charset_editing_text_export_and_repeated_import(self):
        self.file('HELLO.PRG',HELLO)
        out=self.run_basic('CHARSET ISO\nLOAD "HELLO.PRG"\nCHARSET\n10 PRINT "EDITED"\nSAVE "EDITED.BAS"\nRUN\nCHARSET LOWER\nLOAD "HELLO.PRG"\nCHARSET\nRUN\nNEW\nLOAD "EDITED.BAS"\nRUN')
        self.assertIn(b'CHARSET PETSCII',out);self.assertIn(b'CHARSET PETSCII LOWER',out)
        self.assertEqual(out.count(b'EDITED'),2);self.assertIn(b'HELLO FROM C64 PRG',out)
        self.assertEqual((self.folder/'EDITED.BAS').read_bytes(),b'10 PRINT "EDITED"\n20 END\n')

if __name__=='__main__':unittest.main()
