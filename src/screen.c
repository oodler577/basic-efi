#include "basic.h"
#include <string.h>
Screen screen={.ink=15};

void screen_charset(unsigned set) {
    if(set>=CHARSET_COUNT)return;
    screen.charset=set;screen.reverse=screen.verbatim=0;screen.dirty=1;
}

void screen_clear(void) {
    memset(screen.cells,' ',sizeof(screen.cells));
    memset(screen.color,screen.ink,sizeof(screen.color));
    memset(screen.continued,0,sizeof(screen.continued));
    screen.x=screen.y=0; screen.dirty=1;
}
static void next_row(int continued) {
    screen.x=0;
    if (++screen.y==ROWS) {
        ++screen.scrolls;
        memmove(screen.cells,screen.cells[1],(ROWS-1)*COLS);
        memmove(screen.color,screen.color[1],(ROWS-1)*COLS);
        memmove(screen.continued,screen.continued+1,ROWS-1);
        memset(screen.cells[ROWS-1],' ',COLS);
        memset(screen.color[ROWS-1],screen.ink,COLS);
        screen.y=ROWS-1;
    }
    screen.continued[screen.y]=(unsigned char)continued;
    screen.dirty=1;
}
void screen_put(int c) {
    if(c<0 || c>255)return;
    if(screen.verbatim){screen.verbatim=0;goto printable;}
    if(c==128){screen.verbatim=1;return;}
    if(c==15){screen_charset(CHARSET_ISO);return;}
    if(c==143 || c==142){screen_charset(CHARSET_PET);return;}
    if(c==14){screen_charset(CHARSET_PET_LOWER);return;}
    if(c==18){screen.reverse=1;return;}
    if(c==146){screen.reverse=0;return;}
    static const unsigned char ink_codes[16]={144,5,28,159,156,30,31,158,129,149,150,151,152,153,154,155};
    for(unsigned i=0;i<16;++i)if(c==ink_codes[i]){screen.ink=(unsigned char)i;return;}
    if (c==13 || c==10) { screen.reverse=0;next_row(0); return; }
    if (c==147 || c==12) { screen_clear(); return; }
    if (c==19) { screen.x=screen.y=0; screen.dirty=1; return; }
    if (c==157) { if (screen.x) --screen.x; screen.dirty=1; return; }
    if (c==29) { if (++screen.x==COLS) next_row(1); screen.dirty=1; return; }
    if (c==145) { if(screen.y) --screen.y; screen.dirty=1; return; }
    if (c==17) { if(++screen.y==ROWS) {--screen.y; next_row(0);} screen.dirty=1; return; }
    if(c<32 || (c>=128 && c<160))return;
printable:
    screen.cells[screen.y][screen.x]=charset_screen(screen.charset,(unsigned char)c);
    screen.color[screen.y][screen.x]=screen.ink;
    if(screen.reverse){
        if(screen.charset==CHARSET_ISO)screen.color[screen.y][screen.x]|=128;
        else screen.cells[screen.y][screen.x]^=128;
    }
    screen.dirty=1;
    if (++screen.x==COLS) next_row(1);
}
void screen_text(const char *s) {
    while(*s) {
        unsigned char c=(unsigned char)*s++;
        screen_put(c>=32?charset_input(screen.charset,c):c);platform_trace(c);
    }
}
void screen_set_cell(unsigned offset, unsigned char value) {
    if(offset>=COLS*ROWS) return;
    screen.cells[offset/COLS][offset%COLS]=value;
    screen.dirty=1;
}
void screen_set_color(unsigned offset, unsigned char value) {
    if(offset>=COLS*ROWS) return;
    screen.color[offset/COLS][offset%COLS]=(screen.color[offset/COLS][offset%COLS]&128)|(value&15);
    screen.dirty=1;
}
static int logical_start(int row) {
    while(row>0 && screen.continued[row]) --row;
    return row;
}
static int logical_end(int row) {
    while(row+1<ROWS && screen.continued[row+1]) ++row;
    return row;
}
int screen_edit(char *line, size_t cap) {
    int origin_y=screen.y,origin_x=screen.x;
    unsigned origin_scroll=screen.scrolls;
    for(;;) {
        platform_draw();
        int key=platform_key(1);
        if(key==KEY_EXIT || key<0) return -1;
        if(key==KEY_RESET) return -2;
        if(key==KEY_ESCAPE) { screen_put(13); line[0]=0; return 0; }
        if(key==KEY_UP) { if(screen.y) --screen.y; }
        else if(key==KEY_DOWN) { if(screen.y+1<ROWS) ++screen.y; }
        else if(key==KEY_LEFT) { if(screen.x) --screen.x; else if(screen.y){--screen.y;screen.x=COLS-1;} }
        else if(key==KEY_RIGHT) { if(screen.x+1<COLS) ++screen.x; else if(screen.y+1<ROWS){++screen.y;screen.x=0;} }
        else if(key==KEY_HOME) {screen.y=logical_start(screen.y);screen.x=0;}
        else if(key==KEY_END) {
            screen.y=logical_end(screen.y);screen.x=COLS-1;
            while(screen.x>0 && screen.cells[screen.y][screen.x-1]==' ') --screen.x;
        }
        else if(key==8 || key==127 || key==KEY_DELETE) {
            if(key!=KEY_DELETE) {
                if(screen.x) --screen.x;
                else if(screen.y && screen.continued[screen.y]) {--screen.y;screen.x=COLS-1;}
                else continue;
            }
            int end=logical_end(screen.y)*COLS+COLS;
            int pos=screen.y*COLS+screen.x;
            unsigned char *cells=&screen.cells[0][0];
            memmove(cells+pos,cells+pos+1,(size_t)(end-pos-1)); cells[end-1]=' ';
            unsigned char *colors=&screen.color[0][0];
            memmove(colors+pos,colors+pos+1,(size_t)(end-pos-1)); colors[end-1]=15;
        }
        else if(key==13 || key==10) {
            int first=logical_start(screen.y),last=logical_end(screen.y);
            size_t n=(size_t)(last-first+1)*COLS;
            const unsigned char *src=screen.cells[first];
            int original_row=origin_y-(int)(screen.scrolls-origin_scroll);
            if(origin_x && original_row>=first && original_row<=last) {
                size_t prefix=(size_t)(original_row-first)*COLS+(unsigned)origin_x;
                src+=prefix;n-=prefix;
            }
            while(n && charset_byte(screen.charset,src[n-1])==' ') --n;
            if(n>=cap) {screen_text("\r?LINE TOO LONG\r");continue;}
            int invalid=0;
            for(size_t i=0;i<n;++i){
                unsigned char c=charset_byte(screen.charset,src[i]);
                if(c<32 || (c>=127 && c<160))invalid=1;
                line[i]=(char)c;
            }
            if(invalid){screen_text("\r?NON-TEXT SCREEN LINE\r");continue;}
            line[n]=0;
            screen.y=last; screen_put(13);
            platform_trace('\n');
            return (int)n;
        }
        else if(key>=32 && (key<=126 || (key>=160 && key<=255))) {
            int count=(screen.y-logical_start(screen.y))*COLS+screen.x;
            if(count>=LINE_MAXIMUM) continue;
            screen_put(key);
        }
        else if(key==14 || key==15 || key==142 || key==143)screen_put(key);
        screen.dirty=1;
    }
}
