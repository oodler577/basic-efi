#!/usr/bin/env python3
import pathlib,subprocess,tempfile,unittest,os
ROOT=pathlib.Path(__file__).resolve().parent.parent
BINARY=pathlib.Path(os.environ.get('BASIC_BINARY',str(ROOT/'build/basic-efi')))

class BasicTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=pathlib.Path(self.temp.name)
    def run_basic(self,text):
        p=subprocess.run([str(BINARY),str(self.folder)],input=text+'\nEXIT\n',text=True,capture_output=True,timeout=15)
        self.assertEqual(p.returncode,0,p.stderr)
        return p.stdout
    def test_original_math_and_strings(self):
        out=self.run_basic('PRINT 2+2\nPRINT SQR(81)\nPRINT LEFT$("MICROSOFT",5)\nPRINT 1/2')
        for expected in [' 4 ',' 9 ','MICRO',' .5 ']:self.assertIn(expected,out)
    def test_program_arrays_data_and_subroutines(self):
        out=self.run_basic('10 DIM A(3)\n20 DATA 4,5,6\n30 FOR I=1 TO 3\n40 READ A(I)\n50 NEXT I\n60 GOSUB 100\n70 END\n100 PRINT A(1)+A(2)+A(3)\n110 RETURN\nRUN')
        self.assertIn(' 15 ',out);self.assertNotIn('ERROR',out)
    def test_input(self):
        out=self.run_basic('10 INPUT A\n20 PRINT A*3\nRUN\n7')
        self.assertIn(' 21 ',out)
    def test_string_input_preserves_case(self):
        out=self.run_basic('10 INPUT "NAME";A$\n20 PRINT "Hello ";A$\nRUN\nBrett')
        self.assertIn('Hello Brett',out)
    def test_file_roundtrip(self):
        program='10 REM TOKEN WORDS PRINT GOTO\n20 DATA "PRINT",12\n30 READ A$,A\n40 PRINT A$;A\n50 PRINT "Mixed case"'
        out=self.run_basic(program+'\nSAVE "ROUND.BAS"\nNEW\nLOAD "ROUND.BAS"\nRUN')
        self.assertIn('SAVED',out);self.assertIn('LOADED',out);self.assertIn('PRINT 12 ',out)
        self.assertIn('Mixed case',out)
        saved=(self.folder/'ROUND.BAS').read_text()
        self.assertEqual(saved,program+'\n')
    def test_extended_quoted_text_roundtrip(self):
        for mode in (b'ISO',b'PETSCII',b'LOWER'):
            name=mode.decode()+'.BAS'
            program=b'10 PRINT "'+bytes(range(160,256))+b'"'
            (self.folder/'SOURCE.BAS').write_bytes(program+b'\n')
            commands=b'CHARSET '+mode+b'\nLOAD "SOURCE.BAS"\nSAVE "'+name.encode()+b'"\nNEW\nLOAD "'+name.encode()+b'"\nRUN\nEXIT\n'
            p=subprocess.run([str(BINARY),str(self.folder)],input=commands,capture_output=True,timeout=15)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertIn(bytes(range(160,256)),p.stdout)
            self.assertEqual((self.folder/name).read_bytes(),program+b'\n')
    def test_character_modes_keep_program(self):
        out=self.run_basic('10 PRINT 6*7\nCHARSET PETSCII\nPOKE 32768,129\nPRINT PEEK(32768)\nCHARSET LOWER\nRUN\nCHARSET ISO\nRUN')
        self.assertEqual(out.count(' 42 '),2)
        self.assertIn(' 129 ',out)
        self.assertNotIn('ERROR',out)
    def test_no_clobber(self):
        (self.folder/'KEEP.BAS').write_text('10 PRINT "KEEP"\n')
        out=self.run_basic('10 PRINT "OTHER"\nSAVE "KEEP.BAS"')
        self.assertIn('FILE EXISTS',out)
        self.assertEqual((self.folder/'KEEP.BAS').read_text(),'10 PRINT "KEEP"\n')
    def test_reject_paths(self):
        out=self.run_basic('SAVE "../OUT.BAS"\nLOAD "/etc/passwd"')
        self.assertEqual(out.count('SIMPLE FILENAME'),2)
        self.assertEqual(list(self.folder.iterdir()),[])
    def test_reject_symlink(self):
        (self.folder/'REAL.BAS').write_text('10 PRINT "SECRET"\n')
        (self.folder/'LINK.BAS').symlink_to(self.folder/'REAL.BAS')
        out=self.run_basic('LOAD "LINK.BAS"')
        self.assertIn('FILE I/O ERROR',out)
    def test_invalid_load_preserves_program(self):
        (self.folder/'BAD.BAS').write_text('EXIT\n')
        out=self.run_basic('10 PRINT "PRESERVED"\nLOAD "BAD.BAS"\nRUN')
        self.assertIn('INVALID BASIC TEXT',out);self.assertIn('PRESERVED',out)
    def test_oversized_load_preserves_program(self):
        (self.folder/'BIG.BAS').write_text(''.join(f'{i} REM '+('X'*200)+'\n' for i in range(1,201)))
        out=self.run_basic('10 PRINT "STILL HERE"\nLOAD "BIG.BAS"\nRUN')
        self.assertIn('INVALID BASIC TEXT',out);self.assertIn('STILL HERE',out)
        self.assertNotIn('LOADED',out)
    def test_screen_reedit(self):
        out=self.run_basic('10 PRINT "FIRST"\n\x1b[A\x1b[H10 PRINT "OTHER"\nRUN')
        self.assertIn('OTHER',out);self.assertNotIn('FIRST',out)
    def test_get_character(self):
        out=self.run_basic('10 GET A$\n20 PRINT A$\nRUN\nZ')
        self.assertIn('Z',out)
    def test_get_extended_byte(self):
        p=subprocess.run([str(BINARY),str(self.folder)],input=b'10 GET A$\n20 PRINT ASC(A$)\nRUN\n\xe9EXIT\n',capture_output=True,timeout=15)
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn(b' 233 ',p.stdout)
    def test_integer_widths_and_unsigned_64(self):
        out=self.run_basic('BITS 8\nCALC 255+1\nCALC 128\nBITS 16\nCALC $FFFF+2\nBITS 32\nCALC $FFFFFFFF+1\nBITS 64\nCALC 18446744073709551615\nCALC (1<<63)+1\nCALC 1/0\nCALC 1<<64\nPRINT 300')
        for s in ['UNSIGNED 0  SIGNED 0  HEX $00','UNSIGNED 128  SIGNED -128','HEX $0001','HEX $00000000','UNSIGNED 18446744073709551615  SIGNED -1','HEX $8000000000000001',' 300 ']:self.assertIn(s,out)
        self.assertEqual(out.count('INTEGER EXPRESSION ERROR'),2)
    def test_screen_memory(self):
        out=self.run_basic('POKE 32768,90\nPRINT PEEK(32768)\nPOKE 36864,5\nPRINT PEEK(36864)')
        self.assertIn(' 90 ',out);self.assertIn(' 5 ',out)
    def test_program_survives_scrolling(self):
        program='\n'.join(f'{i*10} REM LINE {i}' for i in range(1,101))+'\n1010 PRINT "END OF LONG PROGRAM"'
        out=self.run_basic(program+'\nLIST\nSAVE "LONG.BAS"\nRUN')
        self.assertIn('END OF LONG PROGRAM',out)
        self.assertEqual((self.folder/'LONG.BAS').read_text(),program+'\n')
    def test_wrapped_line(self):
        text='X'*160
        out=self.run_basic('10 PRINT "'+text+'"\nRUN\nSAVE "WRAP.BAS"')
        self.assertIn(text,out)
        self.assertEqual((self.folder/'WRAP.BAS').read_text(),'10 PRINT "'+text+'"\n')
    def test_runtime_file_command_fails_cleanly(self):
        out=self.run_basic('10 SAVE "NO.BAS"\nRUN\nPRINT 6*7')
        self.assertIn('FILE COMMANDS REQUIRE DIRECT MODE',out);self.assertIn(' 42 ',out)
        self.assertFalse((self.folder/'NO.BAS').exists())
    def test_iec_unavailable_preserves_default_storage(self):
        out=self.run_basic('IEC ON\nIEC\nIEC DEVICE 31\nIEC DEVICE 9\nIEC STATUS\n10 PRINT 42\nSAVE "LOCAL.BAS",8\nIEC OFF\nRUN')
        self.assertIn('UNSUPPORTED',out);self.assertIn('STORAGE: FAT BASIC FOLDER; IEC OFF',out)
        self.assertIn('USE IEC DEVICE 8 THROUGH 30',out);self.assertIn('USE IEC ON FIRST',out)
        self.assertEqual((self.folder/'LOCAL.BAS').read_text(),'10 PRINT 42\n')
        self.assertIn(' 42 ',out)

if __name__=='__main__':unittest.main(verbosity=2)
