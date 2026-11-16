/* Exercise the converter against this build's actual ROM, with guarded output
   and the invariant that every accepted file survives a lossless round trip. */
#include "prg.h"
#include "rom.h"
#include <assert.h>
#include <string.h>
static uint8_t forward[256],reverse[256];
static void maps(void){
    char native[128][16]={{0}};size_t p=ROM_RESLST-0xc000;unsigned t=0,n=0;
    while(p<sizeof(basic_rom) && basic_rom[p] && t<127){
        unsigned c=basic_rom[p++];assert(n<15);native[t][n++]=(char)(c&127);
        if(c&128){++t;n=0;}
    }
    prg_maps(native,forward,reverse);
}
static void check_bytes(const uint8_t *data,size_t size,size_t cap){
    static uint8_t guarded[PRG_CAPACITY+4],other[PRG_CAPACITY+2];
    assert(cap<=PRG_CAPACITY+2);
    memset(guarded,0xa5,sizeof(guarded));size_t n=999,back=999;PrgError e;
    int result=prg_import(data,size,forward,guarded+1,cap,&n,&e);
    assert(guarded[0]==0xa5 && guarded[cap+1]==0xa5);
    if(result)assert(!n && e.code==result);
    else{
        assert(n<=cap);
        assert(!prg_export(guarded+1,n,0x0401,reverse,other,sizeof(other),&back,&e));
        assert(back==size && !memcmp(other,data,size));
    }
    memset(guarded,0xa5,sizeof(guarded));n=999;
    result=prg_export(data,size,0x0401,reverse,guarded+1,cap,&n,&e);
    assert(guarded[0]==0xa5 && guarded[cap+1]==0xa5);
    if(result)assert(!n && e.code==result);
    else{
        assert(n<=cap);
        assert(!prg_import(guarded+1,n,forward,other,sizeof(other),&back,&e));
        assert(back==size && !memcmp(other,data,size));
    }
}
