#include "basic.h"
#include <assert.h>
#include <string.h>
static const uint8_t *bytes;
static size_t remaining;
void platform_trace(int c){(void)c;}
void platform_draw(void){assert(screen.x>=0&&screen.x<COLS&&screen.y>=0&&screen.y<ROWS);}
int platform_key(int wait){
    (void)wait;if(!remaining)return KEY_EXIT;
    unsigned c=*bytes++;--remaining;
    static const int special[]={KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_HOME,KEY_END,KEY_DELETE,8,13,KEY_ESCAPE};
    return c<10?special[c]:(int)(32+c%95);
}
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size){
    if(size>4096)return 0;
    screen_clear();bytes=data;remaining=size;
    if(size){screen.x=data[0]%COLS;screen.y=data[size-1]%ROWS;}
    struct {char line[241];unsigned char canary[16];}out;memset(&out,0xa5,sizeof(out));
    (void)screen_edit(out.line,sizeof(out.line));platform_draw();
    for(unsigned i=0;i<16;++i)assert(out.canary[i]==0xa5);
    return 0;
}
