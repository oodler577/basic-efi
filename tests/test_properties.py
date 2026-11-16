"""Seeded behavioral tests with independent numeric and program-store oracles."""
import os
import pathlib
import random
import re
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parent.parent
BINARY=pathlib.Path(os.environ.get('BASIC_BINARY',str(ROOT/'build/basic-efi')))

class PropertyTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=pathlib.Path(self.temp.name)
    def run_basic(self,lines):
        result=subprocess.run([str(BINARY),str(self.folder)],input='\n'.join(lines)+'\nEXIT\n',
                              text=True,capture_output=True,timeout=60)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(result.stderr,'')
        return result.stdout
    def test_2400_integer_expressions_against_python(self):
        rng=random.Random(6502);lines=[];expected=[]
        for bits in (8,16,32,64):
            lines.append(f'BITS {bits}');mask=(1<<bits)-1
            for _ in range(600):
                a=rng.getrandbits(bits);b=rng.getrandbits(bits);op=rng.choice(['+','-','*','/','%','&','|','^','<<','>>'])
                if op in ('<<','>>'):b=rng.randrange(bits)
                if op in ('/','%'):b=b or 1
                value={'+':lambda:a+b,'-':lambda:a-b,'*':lambda:a*b,'/':lambda:a//b,'%':lambda:a%b,
                       '&':lambda:a&b,'|':lambda:a|b,'^':lambda:a^b,'<<':lambda:a<<b,'>>':lambda:a>>b}[op]()&mask
                lines.append(f'CALC ${a:X}{op}${b:X}')
                signed=value-(1<<bits) if value&(1<<(bits-1)) else value
                expected.append((str(value),str(signed),f'{value:0{bits//4}X}'))
        actual=re.findall(r'UNSIGNED (\d+)  SIGNED (-?\d+)  HEX \$([0-9A-F]+)',self.run_basic(lines))
        self.assertEqual(actual,expected)
    def test_600_program_insert_replace_delete_operations(self):
        rng=random.Random(8086);program={};lines=[]
        for _ in range(600):
            number=rng.randrange(1,151)*10
            if rng.randrange(5)==0:
                lines.append(str(number));program.pop(number,None)
            else:
                body='REM '+''.join(rng.choices('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ',k=rng.randrange(1,190))).rstrip()
                program[number]=body;lines.append(f'{number} {body}')
        lines+=['SAVE "MODEL.BAS"','NEW','LOAD "MODEL.BAS"','SAVE "COPY.BAS"']
        out=self.run_basic(lines);self.assertNotIn('ERROR',out)
        expected=''.join(f'{n} {body}\n' for n,body in sorted(program.items()))
        self.assertEqual((self.folder/'MODEL.BAS').read_text(),expected)
        self.assertEqual((self.folder/'COPY.BAS').read_text(),expected)
    def test_random_invalid_imports_are_transactional(self):
        rng=random.Random(286);lines=['10 PRINT "PRESERVED"']
        for i in range(120):
            bad=bytearray(b'20 REM '+b'A'*rng.randrange(1,220))
            bad.insert(rng.randrange(len(bad)),rng.choice([0,1,9,27,127,128,255]))
            (self.folder/f'BAD{i}.BAS').write_bytes(bad+b'\n')
            lines.append(f'LOAD "BAD{i}.BAS"')
        lines+=['SAVE "FINAL.BAS"','RUN'];out=self.run_basic(lines)
        self.assertEqual(out.count('INVALID BASIC TEXT'),120)
        self.assertEqual((self.folder/'FINAL.BAS').read_text(),'10 PRINT "PRESERVED"\n')
        self.assertIn('PRESERVED',out)
    def test_full_length_lines_and_line_number_boundaries(self):
        program='0 REM ZERO\n10 REM '+('X'*233)+'\n63999 PRINT "LAST"\n'
        self.assertEqual(len(program.splitlines()[1]),240)
        (self.folder/'LIMIT.BAS').write_text(program)
        out=self.run_basic(['LOAD "LIMIT.BAS"','SAVE "ROUND.BAS"','RUN'])
        self.assertNotIn('ERROR',out);self.assertIn('LAST',out)
        self.assertEqual((self.folder/'ROUND.BAS').read_text(),program)
    def test_malformed_calculator_expressions_recover(self):
        cases=['','(',')','1+','1/0','1%0','1<<64','1>>64','0x','$','1 2','1**2','1&&2','((3)','A','1<2']
        out=self.run_basic(['BITS 64']+['CALC '+s for s in cases if s]+['PRINT 6*7'])
        self.assertEqual(out.count('INTEGER EXPRESSION ERROR'),len(cases)-1)
        self.assertIn(' 42 ',out)
    def test_fifo_directory_and_symlink_cannot_be_loaded(self):
        os.mkfifo(self.folder/'PIPE.BAS');(self.folder/'DIR.BAS').mkdir()
        (self.folder/'KEEP.BAS').write_text('10 PRINT "KEEP"\n')
        (self.folder/'LINK.BAS').symlink_to(self.folder/'KEEP.BAS')
        out=self.run_basic(['LOAD "PIPE.BAS"','LOAD "DIR.BAS"','SAVE "LINK.BAS"'])
        self.assertEqual(out.count('FILE I/O ERROR'),2);self.assertIn('FILE EXISTS',out)
        self.assertEqual((self.folder/'KEEP.BAS').read_text(),'10 PRINT "KEEP"\n')

if __name__=='__main__':unittest.main(verbosity=2)
