#include "basic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void platform_trace(int c){(void)c;}
void platform_draw(void){}
int platform_key(int wait){(void)wait;return KEY_EXIT;}
static TextPixel pixels[TEXT_ROW_PIXELS+2];
int main(void){
    assert(text_scale(639,800)==0 && text_scale(640,199)==0);
    assert(text_scale(640,480)==1 && text_scale(1280,800)==2);
    assert(text_scale(1920,1080)==3 && text_scale(3840,2160)==4);
    for(unsigned set=0;set<CHARSET_COUNT;++set){
        for(unsigned c=0;c<256;++c){
            unsigned char cell=(unsigned char)c;
            if(set!=CHARSET_ISO){
                unsigned char byte=charset_byte(set,cell);
                assert(charset_screen(set,byte)==(cell&127));
                for(unsigned y=0;y<8;++y)
                    assert((charset_row(set,cell,y)^charset_row(set,cell^128,y))==255);
            }
            unsigned char colors[1]={15};
            for(unsigned scale=1;scale<=TEXT_MAX_SCALE;++scale){
                size_t n=64*scale*scale;
                memset(pixels,0x55,sizeof(pixels));
                assert(text_render_row(pixels+1,n,&cell,colors,1,set,scale,-1));
                for(unsigned y=0;y<8*scale;++y)for(unsigned x=0;x<8*scale;++x){
                    unsigned bit=charset_row(set,cell,y/scale)&(128>>(x/scale));
                    assert(!memcmp(pixels+1+y*8*scale+x,&text_palette[bit?15:6],sizeof(TextPixel)));
                }
                const unsigned char *first=(const unsigned char*)pixels,*last=(const unsigned char*)(pixels+n+1);
                for(unsigned i=0;i<sizeof(TextPixel);++i)assert(first[i]==0x55 && last[i]==0x55);
                assert(!text_render_row(pixels,n-1,&cell,colors,1,set,scale,-1));
            }
        }
    }
    unsigned char cells[80],colors[80];memset(cells,65,80);memset(colors,15,80);
    assert(text_render_row(pixels,TEXT_ROW_PIXELS,cells,colors,80,CHARSET_ISO,4,79));
    assert(!text_render_row(pixels,TEXT_ROW_PIXELS,cells,colors,81,CHARSET_ISO,4,0));
    assert(!text_render_row(pixels,TEXT_ROW_PIXELS,cells,colors,80,CHARSET_COUNT,4,0));
    assert(!text_render_row(pixels,TEXT_ROW_PIXELS,cells,colors,80,0,0,0));
    assert(!text_render_row(pixels,TEXT_ROW_PIXELS,cells,colors,80,0,5,0));
    for(unsigned c=160;c<256;++c)
        assert(charset_input(CHARSET_ISO,charset_unicode(CHARSET_ISO,(unsigned char)c))==(int)c);
    assert(charset_input(CHARSET_ISO,0x20ac)==164);
    assert(charset_input(CHARSET_ISO,0x152)==188);
    assert(charset_input(CHARSET_ISO,0x1f600)==0);
    assert(charset_screen(CHARSET_PET,65)==1);
    assert(charset_screen(CHARSET_PET,193)==65);
    assert(charset_screen(CHARSET_PET,255)==94);
    assert(charset_input(CHARSET_PET_LOWER,'a')==65);
    assert(charset_input(CHARSET_PET_LOWER,'A')==193);
    screen_charset(CHARSET_PET);screen_clear();screen_put(65);
    assert(screen.cells[0][0]==1);
    screen_put(18);screen_put(65);assert(screen.cells[0][1]==129);
    screen_put(146);screen_put(28);screen_put(193);
    assert(screen.cells[0][2]==65 && screen.color[0][2]==2);
    screen_put(14);assert(screen.charset==CHARSET_PET_LOWER);
    screen_put(15);screen_put(164);assert(screen.charset==CHARSET_ISO && screen.cells[0][3]==164);
    screen_put(18);screen_put(233);assert(screen.cells[0][4]==233 && screen.color[0][4]==130);
    screen_put(13);assert(!screen.reverse);
    screen_set_cell(0,0);assert(screen.cells[0][0]==0);
    screen_put(128);screen_put(13);assert(screen.cells[1][0]==13 && screen.y==1);
    assert(charset_row(CHARSET_PET,1,0)==24); /* Original X16 A raster. */
    assert(charset_row(CHARSET_PET,83,1)==127); /* Heart raster. */
    puts("PASS: 3 charsets x 256 codes x 4 scales, guards, reverse, controls, ISO input");
    return 0;
}
