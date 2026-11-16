#include "basic.h"
#include <string.h>
static void emulator_fault(void) {exiting=1;}
#define CHIPS_ASSERT(c) do {if(!(c))emulator_fault();} while(0)
#define assert(c) CHIPS_ASSERT(c)
#define CHIPS_IMPL
#include "../vendor/chips/m6502.h"
#include "rom.h"

int exiting;
static unsigned char memory[65536], filebuf[FILE_CAPACITY];
static m6502_t cpu;
static uint64_t pins;
static char input[LINE_MAXIMUM+2];
static size_t input_pos, input_len, load_pos, load_len;
static int loading, load_new, break_requested, reset_requested;
static int direct_input, last_cr, suppress_input_newline;
static unsigned bits=64;
static unsigned short word(unsigned a) {return (unsigned short)(memory[a]|(memory[a+1]<<8));}
static int same(const char *a,const char *b) {return strcmp(a,b)==0;}
static char upper(char c) {return c>='a' && c<='z'?(char)(c-'a'+'A'):c;}
static void normalize(char *s) {
    int quoted=0;
    for(;*s;++s) {if(*s=='"') quoted=!quoted; else if(!quoted)*s=upper(*s);}
}
static void decimal(uint64_t value,char *out) {
    char temp[21];int n=0;
    do {temp[n++]=(char)('0'+value%10);value/=10;}while(value);
    for(int i=0;i<n;++i)out[i]=temp[n-i-1];
    out[n]=0;
}
int valid_filename(const char *s) {
    size_t n=strlen(s);
    if(!n || n>64 || s[0]=='.')return 0;
    for(size_t i=0;i<n;++i) {
        char c=s[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;
    }
    return 1;
}
static int filename(const char *p,char *name) {
    while(*p==' ')++p;
    if(*p++!='"')return 0;
    unsigned n=0;
    while(*p && *p!='"') {if(n==64)return 0;name[n++]=*p++;}
    if(*p++!='"')return 0;
    name[n]=0;while(*p==' ')++p;
    if(*p==','){++p;while(*p==' ')++p;if(*p++!='8')return 0;while(*p==' ')++p;}
    return *p==0 && valid_filename(name);
}
static void storage_error(int e) {
    const char *s=e==1?"?FILE NOT FOUND":e==2?"?FILE EXISTS (USE A NEW NAME)":e==4?"?FILE TOO LARGE":"?FILE I/O ERROR";
    screen_text(s);screen_text("\r");
}
static char tokens[128][16];
static void init_tokens(void) {
    unsigned p=ROM_RESLST,t=0,n=0;
    memset(tokens,0,sizeof(tokens));
    while(p<sizeof(memory) && memory[p] && t<127) {
        unsigned char c=memory[p++];
        if(n<15)tokens[t][n++]=(char)(c&127);
        if(c&128){++t;n=0;}
    }
}
static int append(unsigned char *buf,size_t *used,const char *s) {
    size_t n=strlen(s);if(*used+n>=FILE_CAPACITY)return 0;
    memcpy(buf+*used,s,n);*used+=n;return 1;
}
static int export_program(size_t *length) {
    unsigned p=word(ROM_TXTTAB),end=word(ROM_VARTAB);size_t used=0;
    if(p<0x0401 || end>0x8000 || p+2>end)return 0;
    while(p+2<=end && word(p)) {
        unsigned next=word(p);char number[21];
        if(next<=p+4 || next>end-2 || next>0x8000)return 0;
        decimal(word(p+2),number);
        if(!append(filebuf,&used,number)||!append(filebuf,&used," "))return 0;
        int quoted=0,remark=0,data=0;
        unsigned a=p+4;
        while(a<next && memory[a]) {
            unsigned char c=memory[a++];
            if(c=='"')quoted=!quoted;
            if(!quoted && c==':')data=0;
            if(c>=128 && !quoted && !remark && !data) {
                if(c==255) {if(!append(filebuf,&used,"3.141592653589793"))return 0;}
                else {
                    if(!tokens[c-128][0] || !append(filebuf,&used,tokens[c-128]))return 0;
                    if(same(tokens[c-128],"REM"))remark=1;
                    if(same(tokens[c-128],"DATA"))data=1;
                }
            }else {
                if(c<32 || c>126 || used+1>=FILE_CAPACITY)return 0;
                filebuf[used++]=c;
            }
        }
        if(a!=next-1 || memory[a]!=0 || !append(filebuf,&used,"\n"))return 0;
        p=next;
    }
    *length=used;return 1;
}
static int validate_source(size_t n) {
    size_t p=0,stored=2;
    while(p<n) {
        size_t start=p;while(p<n && filebuf[p]!='\n' && filebuf[p]!='\r') {
            if(filebuf[p]<32 || filebuf[p]>126)return 0;
            ++p;
        }
        if(p-start>LINE_MAXIMUM)return 0;
        size_t q=start;while(q<p && filebuf[q]==' ')++q;
        if(q<p) {
            unsigned number=0,count=0;
            while(q<p && filebuf[q]>='0' && filebuf[q]<='9') {
                if(++count>5)return 0;
                number=number*10+filebuf[q++]-'0';
            }
            if(!count || number>63999)return 0;
            /* Upper bound before tokenization; reserve room for runtime variables.
             * Reject before NEW so a large import cannot destroy the old program. */
            stored+=5+p-q;
            if(stored>30000)return 0;
        }
        if(p<n && filebuf[p++]=='\r' && p<n && filebuf[p]=='\n')++p;
    }
    return 1;
}

/* Exact-width unsigned integer workbench, deliberately separate from MS floats. */
typedef struct {const char *p;int error;uint64_t mask;} Expr;
static void spaces(Expr *e){while(*e->p==' ')++e->p;}
static uint64_t expression(Expr *e,int level);
static uint64_t atom(Expr *e) {
    spaces(e);uint64_t v=0;unsigned base=10,count=0;
    if(*e->p=='('){++e->p;v=expression(e,0);spaces(e);if(*e->p!=')')e->error=1;else ++e->p;return v;}
    if(*e->p=='-'||*e->p=='~'||*e->p=='+'){char op=*e->p++;v=atom(e);return (op=='-'?0-v:op=='~'?~v:v)&e->mask;}
    if(*e->p=='$'){base=16;++e->p;}
    else if(e->p[0]=='0'&&(e->p[1]=='X'||e->p[1]=='x')){base=16;e->p+=2;}
    for(;;) {
        char c=upper(*e->p);unsigned d=c>='0'&&c<='9'?(unsigned)(c-'0'):c>='A'&&c<='F'?(unsigned)(c-'A'+10):99;
        if(d>=base)break;
        v=(v*base+d)&e->mask;++e->p;++count;
    }
    if(!count)e->error=1;
    return v;
}
static uint64_t expression(Expr *e,int level) {
    if(level==6)return atom(e);
    uint64_t v=expression(e,level+1);
    while(!e->error) {
        spaces(e);char op=*e->p;int shift=0;
        if(level==0 && op!='|')break;
        if(level==1 && op!='^')break;
        if(level==2 && op!='&')break;
        if(level==3) {if((op!='<'&&op!='>')||e->p[1]!=op)break;shift=1;}
        if(level==4 && op!='+'&&op!='-')break;
        if(level==5 && op!='*'&&op!='/'&&op!='%')break;
        e->p+=1+shift;uint64_t b=expression(e,level+1);
        if((op=='/'||op=='%')&&!b){e->error=1;break;}
        if(shift && b>=bits){e->error=1;break;}
        switch(op){case '|':v|=b;break;case '^':v^=b;break;case '&':v&=b;break;
        case '<':v<<=b;break;case '>':v>>=b;break;case '+':v+=b;break;case '-':v-=b;break;
        case '*':v*=b;break;case '/':v/=b;break;case '%':v%=b;break;}
        v&=e->mask;
    }
    return v;
}
static void calc(const char *text) {
    Expr e={text,0,bits==64?UINT64_MAX:(UINT64_C(1)<<bits)-1};
    uint64_t v=expression(&e,0);spaces(&e);
    if(e.error||*e.p){screen_text("?INTEGER EXPRESSION ERROR\r");return;}
    char number[21],hex[17];decimal(v,number);screen_text("UNSIGNED ");screen_text(number);
    screen_text("  SIGNED ");
    if(v&(UINT64_C(1)<<(bits-1))){screen_text("-");decimal((0-v)&e.mask,number);}else decimal(v,number);
    screen_text(number);screen_text("  HEX $");
    unsigned digits=bits/4;
    for(unsigned i=0;i<digits;++i)hex[digits-i-1]="0123456789ABCDEF"[(v>>(i*4))&15];
    hex[digits]=0;screen_text(hex);screen_text("\r");
}
static int monitor(char *line) {
    char *p=line;while(*p==' ')++p;
    if(same(p,"HELP")) {
        screen_text("MS BASIC: numbered lines, LIST, RUN, NEW, PRINT, INPUT, PEEK, POKE.\r"
                    "Arrow keys edit the visible screen. Enter submits that logical line.\r"
                    "SAVE/LOAD \"NAME.BAS\" : ASCII programs in the BASIC folder; direct mode.\r"
                    "DIR  CLS  RESET  EXIT  INFO. Esc stops RUN; F9 resets BASIC.\r"
                    "BITS 8|16|32|64|NATIVE, CALC expression: separate integer workbench.\r"
                    "BASIC keeps its original floating-point arithmetic in every BITS mode.\r"
                    "Virtual screen: POKE 32768,65; colors start at 36864 (0-15).\r");return 1;
    }
    if(same(p,"INFO")) {
        screen_text("BASIC EFI 0.1 / ORIGINAL MICROSOFT 6502 BASIC 1.1\r"
                    "64 KiB virtual machine; original BASIC workspace below $8000.\r"
                    "80x24 editable ASCII screen; no C64 hardware or game emulation.\r"
                    "File access: BASIC folder on boot volume (desktop: selected folder).\r");return 1;
    }
    if(same(p,"EXIT")){exiting=1;return 1;}
    if(same(p,"CLS")){screen_clear();return 1;}
    if(same(p,"RESET")){reset_requested=1;return 1;}
    if(same(p,"DIR")){platform_directory();return 1;}
    if(!strncmp(p,"BITS",4) && (p[4]==0 || p[4]==' ')) {
        p+=4;while(*p==' ')++p;
        if(*p) {
            if(same(p,"8"))bits=8;else if(same(p,"16"))bits=16;
            else if(same(p,"32"))bits=32;else if(same(p,"64"))bits=64;
            else if(same(p,"NATIVE"))bits=sizeof(void*)*8;
            else {screen_text("?BITS MUST BE 8, 16, 32, 64 OR NATIVE\r");return 1;}
        }
        char number[21];decimal(bits,number);screen_text("INTEGER WORKBENCH: ");screen_text(number);
        screen_text(" BITS; MS BASIC FLOATS UNCHANGED\r");return 1;
    }
    if(!strncmp(p,"CALC ",5)){calc(p+5);return 1;}
    int save=!strncmp(p,"SAVE",4)&&(p[4]==' '||p[4]=='"');
    int load=!strncmp(p,"LOAD",4)&&(p[4]==' '||p[4]=='"');
    if(save||load) {
        char name[65];size_t n;
        if(!filename(p+4,name)){screen_text("?USE SAVE/LOAD \"NAME.BAS\" (SIMPLE FILENAME)\r");return 1;}
        if(save) {
            if(!export_program(&n)){screen_text("?INVALID PROGRAM MEMORY OR UNSUPPORTED TEXT\r");return 1;}
            int e=platform_write(name,filebuf,n);if(e)storage_error(e);else screen_text("SAVED\r");
        }else {
            int e=platform_read(name,filebuf,sizeof(filebuf),&n);
            if(e)storage_error(e);
            else if(!validate_source(n))screen_text("?INVALID BASIC TEXT (NUMBERED ASCII; 240 CHARS/LINE; 30000 BYTE BUDGET)\r");
            else {loading=1;load_new=1;load_pos=0;load_len=n;screen_text("LOADING\r");}
        }
        return 1;
    }
    return 0;
}
static void jump(unsigned address) {
    pins=M6502_SYNC;
    M6502_SET_ADDR(pins,address);M6502_SET_DATA(pins,memory[address]);
    m6502_set_pc(&cpu,(uint16_t)address);
}
static void reset_machine(void) {
    memset(memory,0,sizeof(memory));memcpy(memory+0xc000,basic_rom,sizeof(basic_rom));
    for(unsigned a=0xffc0;a<=0xffe7;a+=3)memory[a]=0x60;
    memory[0xfffc]=ROM_INIT&255;memory[0xfffd]=ROM_INIT>>8;
    const m6502_desc_t desc={0};pins=m6502_init(&cpu,&desc);
    memory[ROM_CURLIN+1]=255;
    input_pos=input_len=load_pos=load_len=0;loading=load_new=break_requested=reset_requested=0;
    direct_input=last_cr=suppress_input_newline=0;
    init_tokens();
    for(unsigned i=0;i<COLS*ROWS;++i){memory[0x8000+i]=' ';memory[0x9000+i]=15;}
}
static int get_input(void) {
    while(input_pos>=input_len) {
        if(loading) {
            if(load_new){strcpy(input,"NEW");load_new=0;}
            else if(load_pos<load_len) {
                size_t n=0;
                while(load_pos<load_len && filebuf[load_pos]!='\r' && filebuf[load_pos]!='\n')input[n++]=(char)filebuf[load_pos++];
                input[n]=0;
                if(load_pos<load_len && filebuf[load_pos++]=='\r' && load_pos<load_len && filebuf[load_pos]=='\n')++load_pos;
            }else {loading=0;screen_text("LOADED\r");continue;}
        }else {
            int n=screen_edit(input,sizeof(input)-1);
            if(n==-1){exiting=1;return 13;}
            if(n==-2){reset_requested=1;return 13;}
            if(direct_input)normalize(input);
            if(direct_input && monitor(input)) {
                if(exiting || reset_requested)return 13;
                continue;
            }
        }
        if(loading)normalize(input);
        input_len=strlen(input);input[input_len++]=13;input_pos=0;
    }
    int c=(unsigned char)input[input_pos++];
    if(c==13){direct_input=0;suppress_input_newline=1;}
    return c;
}
static void trap(unsigned address) {
    if(address==0xffd2) {
        unsigned c=m6502_a(&cpu);
        if(c==10 && last_cr){last_cr=0;return;}
        last_cr=c==13;
        if(c==13 && suppress_input_newline){suppress_input_newline=0;return;}
        suppress_input_newline=0;screen_put(c);platform_trace(c);
        memory[ROM_TRMPOS]=(unsigned char)screen.x;
    }else if(address==0xffcf) {
        unsigned c=(unsigned)get_input();m6502_set_a(&cpu,(uint8_t)c);
        m6502_set_p(&cpu,(uint8_t)((m6502_p(&cpu)&~(M6502_ZF|M6502_NF))|(c==0?M6502_ZF:0)|(c&128)));
    }else if(address==0xffe1) {
        if(break_requested){
            break_requested=0;
            m6502_set_p(&cpu,(uint8_t)(m6502_p(&cpu)|M6502_ZF|M6502_CF));
            jump(ROM_STOP);
        }
    }else if(address==0xffe4) {
        int key=platform_key(-1);
        if(key==KEY_ESCAPE)break_requested=1;
        if(key==KEY_RESET)reset_requested=1;
        if(key==KEY_EXIT)exiting=1;
        unsigned c=key>0 && key<128?(unsigned)key:0;
        m6502_set_a(&cpu,(uint8_t)c);
        m6502_set_p(&cpu,(uint8_t)((m6502_p(&cpu)&~(M6502_ZF|M6502_NF))|(c?0:M6502_ZF)));
    }
    else if(address==0xffd5 || address==0xffd8 || address==0xffdb) {
        screen_text("\r?FILE COMMANDS REQUIRE DIRECT MODE\r");
        m6502_set_s(&cpu,0xfb);jump(ROM_READY);
    }
}
int basic_run(void) {
    screen_clear();screen_text("*** BASIC EFI 0.1 ***\rTYPE HELP FOR COMMANDS\r\r");reset_machine();
    unsigned ticks=0;
    while(!exiting) {
        pins=m6502_tick(&cpu,pins);
        unsigned address=M6502_GET_ADDR(pins);
        if((pins&M6502_SYNC) && address==ROM_MAIN)direct_input=1;
        if((pins&M6502_SYNC) && address>=0xffc0 && address<=0xffe7 && (address-0xffc0)%3==0) {
            trap(address);
            if(exiting)break;
            if(reset_requested){screen_text("\rRESET\r");reset_machine();continue;}
            address=M6502_GET_ADDR(pins);
        }
        if(pins&M6502_RW) {
            unsigned char value=memory[address];
            if(address>=0x8000 && address<0x8000+COLS*ROWS)value=(unsigned char)screen.cells[(address-0x8000)/COLS][(address-0x8000)%COLS];
            if(address>=0x9000 && address<0x9000+COLS*ROWS)value=screen.color[(address-0x9000)/COLS][(address-0x9000)%COLS];
            M6502_SET_DATA(pins,value);
        }else if(address<0xc000) {
            unsigned char value=M6502_GET_DATA(pins);memory[address]=value;
            if(address>=0x8000 && address<0x8000+COLS*ROWS)screen_set_cell(address-0x8000,value);
            if(address>=0x9000 && address<0x9000+COLS*ROWS)screen_set_color(address-0x9000,value);
        }
        if((++ticks&16383)==0) {
            platform_draw();int key=platform_key(0);
            if(key==KEY_ESCAPE)break_requested=1;
            else if(key==KEY_RESET){screen_text("\rRESET\r");reset_machine();}
            else if(key==KEY_EXIT)exiting=1;
            platform_idle();
        }
    }
    platform_draw();return 0;
}
