#include "charset.h"
#include "fonts.h"

const char *charset_name(unsigned set) {
    return set==CHARSET_PET?"PETSCII":set==CHARSET_PET_LOWER?"PETSCII LOWER":"ISO";
}
unsigned char charset_screen(unsigned set, unsigned char c) {
    if(set==CHARSET_ISO)return c;
    if(c<32)return (unsigned char)(c+128);
    if(c<64)return c;
    if(c<96)return (unsigned char)(c-64);
    if(c<128)return (unsigned char)(c-32);
    if(c<160)return (unsigned char)(c+64);
    if(c<192)return (unsigned char)(c-64);
    if(c<255)return (unsigned char)(c-128);
    return 94;
}
unsigned char charset_byte(unsigned set, unsigned char c) {
    if(set==CHARSET_ISO)return c;
    c&=127;
    if(c<32)return (unsigned char)(c+64);
    if(c<64)return c;
    if(c<96)return (unsigned char)(c+128);
    return (unsigned char)(c+64);
}
unsigned char charset_row(unsigned set, unsigned char c, unsigned row) {
    if(row>=8 || set>=CHARSET_COUNT)return 0;
    if(set==CHARSET_ISO)return iso_font[(unsigned)c*8+row];
    unsigned offset=set==CHARSET_PET_LOWER?1024:0;
    unsigned char bits=pet_font[offset+(c&127)*8+row];
    return c&128?(unsigned char)~bits:bits;
}
/* Unicode approximations are used only by terminal/firmware-text fallbacks.
   GOP uses the original X16 bitmap for every cell, including its logo. */
uint32_t charset_unicode(unsigned set, unsigned char c) {
    static const uint32_t iso_special[8]={0x20ac,0x160,0x161,0x17d,0x17e,0x152,0x153,0x178};
    static const unsigned char iso_bytes[8]={0xa4,0xa6,0xa8,0xb4,0xb8,0xbc,0xbd,0xbe};
    if(set==CHARSET_ISO){
        if(c<32)c+=64;
        else if(c>=128&&c<160)c-=32;
        for(unsigned i=0;i<8;++i)if(c==iso_bytes[i])return iso_special[i];
        if(c==0xad)return 0x25c6; /* X16 logo has no Unicode equivalent. */
        return c==127?' ':c;
    }
    c&=127;
    if(c==0)return '@';
    if(c<=26)return (set==CHARSET_PET_LOWER?'a':'A')+c-1;
    if(c==27)return '[';
    if(c==28)return 0xa3;
    if(c==29)return ']';
    if(c==30)return 0x2191;
    if(c==31)return 0x2190;
    if(c<64)return c;
    if(set==CHARSET_PET_LOWER&&c>=65&&c<=90)return 'A'+c-65;
    switch(c){case 64:case 67:case 68:case 69:case 70:case 82:return 0x2500;
    case 65:return 0x2660;case 81:return 0x25cf;case 83:return 0x2665;
    case 87:return 0x25cb;case 88:return 0x2663;case 90:return 0x2666;
    case 91:return 0x253c;case 96:return ' ';case 97:return 0x258c;
    case 98:return 0x2584;case 99:return 0x2594;case 100:return 0x2581;
    case 101:return 0x258f;case 103:return 0x2595;case 106:return 0x2590;
    case 111:return 0x2582;case 116:return 0x258e;case 119:return 0x2580;
    case 120:return 0x2580;case 121:return 0x2583;default:return '?';}
}
int charset_input(unsigned set, uint32_t u) {
    if(set!=CHARSET_ISO){
        if(u>='a'&&u<='z')return (int)(u-'a'+'A');
        if(u>='A'&&u<='Z')return (int)(u+(set==CHARSET_PET_LOWER?128:0));
        if(u<96)return (int)u;
        for(unsigned c=64;c<128;++c)if(charset_unicode(set,(unsigned char)c)==u)
            return charset_byte(set,(unsigned char)c);
        return 0;
    }
    if(u<128 || u==0xad)return (int)u;
    for(unsigned c=160;c<256;++c)if(charset_unicode(set,(unsigned char)c)==u)return (int)c;
    return 0;
}
