#include "basic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int keys[8192];
static size_t count,position;
static unsigned checks;
void platform_trace(int c){(void)c;}
void platform_draw(void){
    assert(screen.x>=0&&screen.x<COLS);
    assert(screen.y>=0&&screen.y<ROWS);
    ++checks;
}
int platform_key(int wait){(void)wait;return position<count?keys[position++]:KEY_EXIT;}
static void begin(void){screen_clear();count=position=0;}
static void key(int k){assert(count<sizeof(keys)/sizeof(keys[0]));keys[count++]=k;}
static void text(const char *s){while(*s)key((unsigned char)*s++);}
static void expect(const char *wanted){
    struct {unsigned char before[16];char line[241];unsigned char after[16];}guard;
    memset(&guard,0xa5,sizeof(guard));
    assert(screen_edit(guard.line,sizeof(guard.line))==(int)strlen(wanted));
    assert(!strcmp(guard.line,wanted));
    for(unsigned i=0;i<16;++i)assert(guard.before[i]==0xa5&&guard.after[i]==0xa5);
}
static unsigned random_state=0x6502;
static unsigned random_value(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
int main(void){
    begin();text("ABCD");key(KEY_LEFT);key(8);key(13);expect("ABD");
    begin();text("ABCD");key(KEY_HOME);key(KEY_RIGHT);key(KEY_DELETE);key(13);expect("ACD");
    begin();text("ABC");key(KEY_HOME);key(KEY_END);text("D");key(13);expect("ABCD");
    begin();screen_text("10 PRINT 1\r");key(KEY_UP);key(KEY_HOME);text("20");key(13);expect("20 PRINT 1");
    begin();screen_text("NAME? ");text("Brett");key(13);expect("Brett");
    begin();screen.y=ROWS-1;screen_text("? ");
    char full[241];memset(full,'X',240);full[240]=0;
    for(int i=0;i<238;++i)key('X');
    key(13);char response[239];memset(response,'X',238);response[238]=0;expect(response);
    begin();screen.y=ROWS-1;text(full);key(13);expect(full);
    begin();text("cancel");key(KEY_ESCAPE);expect("");
    begin();key(KEY_RESET);char line[241];assert(screen_edit(line,sizeof(line))==-2);
    begin();key(KEY_EXIT);assert(screen_edit(line,sizeof(line))==-1);
    begin();screen_set_cell(0,'A');screen_set_cell(1,'B');screen_set_cell(2,'C');
    screen_set_color(0,1);screen_set_color(1,2);screen_set_color(2,3);
    key(KEY_DELETE);key(13);expect("BC");assert(screen.color[0][0]==2&&screen.color[0][1]==3);
    begin();screen_set_cell(COLS*ROWS,88);screen_set_color(COLS*ROWS,5);assert(screen.cells[0][0]==' ');
    /* Scroll cell and color data together, exercising the bottom edge repeatedly. */
    for(unsigned i=0;i<10000;++i){
        unsigned row=(unsigned)screen.y;
        screen_set_cell(row*COLS,(unsigned char)('A'+i%26));screen_set_color(row*COLS,(unsigned char)(i%16));
        screen_put(13);platform_draw();
        assert(screen.cells[screen.y-1][0]==(char)('A'+i%26));
        assert(screen.color[screen.y-1][0]==i%16);
    }
    /* Arbitrary cursor/input sequences must preserve bounds and caller canaries. */
    const int special[]={KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_HOME,KEY_END,KEY_DELETE,8};
    for(unsigned trial=0;trial<1000;++trial){
        begin();screen.y=(int)(random_value()%ROWS);
        for(unsigned i=0;i<200;++i){unsigned r=random_value();key(r%3?' '+(int)(r%95):special[r%8]);}
        key(13);key(KEY_EXIT);
        struct {char line[241];unsigned char guard[16];}out;memset(&out,0xa5,sizeof(out));
        int n=screen_edit(out.line,sizeof(out.line));assert(n>=-1&&n<=240);
        for(unsigned i=0;i<16;++i)assert(out.guard[i]==0xa5);
        platform_draw();
    }
    printf("PASS: screen regressions, 10,000 scrolls, 200,000 seeded edit events (%u bound checks)\n",checks);
    return 0;
}
