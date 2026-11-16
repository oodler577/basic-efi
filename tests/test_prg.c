#include "prg_checks.h"
#include <stdio.h>
static const uint8_t hello[]={1,8,12,8,10,0,0x99,' ','"','H','I','"',0,0,0};
static unsigned random_state=0x6438;
static unsigned random_byte(void){random_state=random_state*1664525u+1013904223u;return random_state>>24;}
int main(void){
    maps();
    /* Independently established native PRINT=$97 vs C64 PRINT=$99. */
    assert(forward[0x99]==0x97 && reverse[0x97]==0x99);
    assert(!forward[0x9e] && !forward[0x84] && forward[255]==255);
    assert(prg_filename("x.PrG") && !prg_filename("PRG") && !prg_filename("X.PRG.BAS"));
    uint8_t native[32],saved[32];size_t n=0,m=0;PrgError e;
    assert(!prg_import(hello,sizeof(hello),forward,native,sizeof(native),&n,&e));
    assert(native[0]==12 && native[1]==4 && native[4]==0x97);
    assert(!prg_export(native,n,0x0401,reverse,saved,sizeof(saved),&m,&e));
    assert(m==sizeof(hello) && !memcmp(saved,hello,m));
    for(size_t cap=0;cap<=sizeof(hello)+2;++cap){
        check_bytes(hello,sizeof(hello),cap);check_bytes(native,n,cap);
    }
    for(size_t cut=0;cut<sizeof(hello);++cut){
        assert(prg_import(hello,cut,forward,saved,sizeof(saved),&m,&e) && !m);
        check_bytes(hello,cut,sizeof(saved));
    }
    assert(prg_export(native,n,65535,reverse,saved,sizeof(saved),&m,&e)==PRG_TOO_LARGE && !m);
    assert(prg_export(native,n,65536,reverse,saved,sizeof(saved),&m,&e)==PRG_TOO_LARGE && !m);
    uint8_t mutated[sizeof(hello)];
    for(unsigned trial=0;trial<20000;++trial){
        memcpy(mutated,hello,sizeof(hello));
        unsigned edits=1+random_byte()%5;
        for(unsigned j=0;j<edits;++j){unsigned index=random_byte()%sizeof(mutated);mutated[index]=(uint8_t)random_byte();}
        check_bytes(mutated,sizeof(mutated),random_byte()%32);
    }
    puts("PASS: PRG guards, exact inverse conversion, capacity/truncation boundaries and 20,000 corrupted records");
    return 0;
}
