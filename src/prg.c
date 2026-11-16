#include "prg.h"
#include <string.h>
/* BASIC V2 interoperability constants, not a C64 ROM. See docs/C64-PRG.md. */
static const char c64[76][8]={
    "END","FOR","NEXT","DATA","INPUT#","INPUT","DIM","READ",
    "LET","GOTO","RUN","IF","RESTORE","GOSUB","RETURN","REM",
    "STOP","ON","WAIT","LOAD","SAVE","VERIFY","DEF","POKE",
    "PRINT#","PRINT","CONT","LIST","CLR","CMD","SYS","OPEN",
    "CLOSE","GET","NEW","TAB(","TO","FN","SPC(","THEN",
    "NOT","STEP","+","-","*","/","^","AND","OR",">","=","<",
    "SGN","INT","ABS","USR","FRE","POS","SQR","RND","LOG","EXP",
    "COS","SIN","TAN","ATN","PEEK","LEN","STR$","VAL","ASC","CHR$",
    "LEFT$","RIGHT$","MID$","GO"
};
const char *prg_keyword(unsigned token){
    return token>=0x80 && token<=0xcb?c64[token-0x80]:token==255?"PI":"UNKNOWN";
}
static int supported(unsigned t){
    switch(t){
    case 0x84:case 0x92:case 0x93:case 0x94:case 0x95:case 0x98:
    case 0x9d:case 0x9e:case 0x9f:case 0xa0:case 0xb7:return 0;
    default:return 1;
    }
}
void prg_maps(const char native[128][16],uint8_t from[256],uint8_t to[256]){
    memset(from,0,256);memset(to,0,256);
    for(unsigned i=0;i<76;++i){
        if(!supported(i+128))continue;
        for(unsigned j=0;j<127;++j){
            if(!strcmp(c64[i],native[j])){from[i+128]=(uint8_t)(j+128);to[j+128]=(uint8_t)(i+128);break;}
        }
    }
    from[255]=to[255]=255;
}
static unsigned word(const uint8_t *p){return (unsigned)p[0]+256u*p[1];}
static void putword(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static int fail(PrgError *error,int code,unsigned token){error->code=code;error->token=token;return code;}
static int convert(const uint8_t *data,size_t n,unsigned source,unsigned dest,const uint8_t map[256],
                   uint8_t *out,size_t cap,size_t *length,PrgError *error,int exporting){
    *length=0;*error=(PrgError){0,65536,0};
    if(n<2)return fail(error,PRG_STRUCTURE,0);
    if(n>PRG_CAPACITY || n>cap || source>65535 || n>65536u-source ||
       dest>65535 || n>65536u-dest)return fail(error,PRG_TOO_LARGE,0);
    size_t p=0;unsigned previous=0;int have_line=0;
    while(p+2<=n){
        unsigned link=word(data+p);
        if(!link){
            if(p+2!=n)return fail(error,PRG_TRAILING,0);
            out[p]=out[p+1]=0;*length=n;return PRG_OK;
        }
        if(n-p<5)return fail(error,PRG_STRUCTURE,0);
        unsigned line=word(data+p+2);error->line=line;
        if(line>63999 || (have_line && line<=previous))return fail(error,PRG_LINE_NUMBER,0);
        size_t end=p+4;
        while(end<n && data[end]){
            if(end-(p+4)>=PRG_LINE_MAX)return fail(error,PRG_LINE_LENGTH,0);
            ++end;
        }
        if(end==n)return fail(error,PRG_STRUCTURE,0);
        /* Validate links against the contiguous file layout; never follow them. */
        if(link!=source+end+1 || end+3>n)return fail(error,PRG_LINK,0);
        putword(out+p,dest+(unsigned)end+1);putword(out+p+2,line);
        int quote=0,remark=0,data_mode=0;
        for(size_t i=p+4;i<end;++i){
            unsigned c=data[i];out[i]=(uint8_t)c;
            if(remark)continue;
            if(c=='"'){quote=!quote;continue;}
            if(quote)continue;
            if(data_mode){if(c==':')data_mode=0;continue;}
            if(c>=128){
                if(!map[c])return fail(error,PRG_UNSUPPORTED,c);
                out[i]=map[c];unsigned semantic=exporting?map[c]:c;
                if(semantic==0x8f)remark=1;
                if(semantic==0x83)data_mode=1;
            }else if(c<32)return fail(error,PRG_STRUCTURE,c);
        }
        out[end]=0;p=end+1;previous=line;have_line=1;
    }
    return fail(error,PRG_STRUCTURE,0);
}
int prg_import(const uint8_t *file,size_t n,const uint8_t map[256],uint8_t *out,size_t cap,
                size_t *length,PrgError *error){
    *length=0;*error=(PrgError){0,65536,0};
    if(n<2 || word(file)!=0x0801)return fail(error,PRG_ADDRESS,0);
    return convert(file+2,n-2,0x0801,0x0401,map,out,cap,length,error,0);
}
int prg_export(const uint8_t *program,size_t n,unsigned base,const uint8_t map[256],uint8_t *out,
                size_t cap,size_t *length,PrgError *error){
    *length=0;*error=(PrgError){0,65536,0};
    if(cap<2)return fail(error,PRG_TOO_LARGE,0);
    size_t used;int e=convert(program,n,base,0x0801,map,out+2,cap-2,&used,error,1);
    if(!e){putword(out,0x0801);*length=used+2;}return e;
}
int prg_filename(const char *s){
    size_t n=strlen(s);if(n<4)return 0;
    const char *suffix=".PRG";s+=n-4;
    for(unsigned i=0;i<4;++i){char c=s[i];if(c>='a'&&c<='z')c=(char)(c-32);if(c!=suffix[i])return 0;}
    return 1;
}
const char *prg_error(int e){
    switch(e){
    case PRG_ADDRESS:return "EXPECTED C64 BASIC LOAD ADDRESS $0801";
    case PRG_STRUCTURE:return "TRUNCATED OR INVALID BASIC RECORD";
    case PRG_LINK:return "INVALID BASIC LINE LINK";
    case PRG_LINE_NUMBER:return "LINE NUMBERS MUST INCREASE, 0 THROUGH 63999";
    case PRG_LINE_LENGTH:return "TOKENIZED LINE EXCEEDS 240 BYTES";
    case PRG_TOO_LARGE:return "PROGRAM EXCEEDS 30000 BYTES OR OUTPUT BUFFER";
    case PRG_UNSUPPORTED:return "UNSUPPORTED KEYWORD OR EXTENSION TOKEN";
    case PRG_TRAILING:return "TRAILING DATA / MACHINE CODE NOT SUPPORTED";
    default:return "OK";
    }
}
