#include "charset.h"

/* Classic 16-color order, retained for the virtual color RAM. */
const TextPixel text_palette[16]={
    {0,0,0,0},{255,255,255,0},{0,0,136,0},{238,255,170,0},
    {204,68,204,0},{85,204,0,0},{170,0,0,0},{119,238,238,0},
    {85,136,221,0},{0,68,102,0},{119,119,255,0},{51,51,51,0},
    {119,119,119,0},{102,255,170,0},{255,136,0,0},{187,187,187,0}
};
unsigned text_scale(unsigned width,unsigned height){
    unsigned scale=width/(80*8),vertical=height/(25*8);
    if(scale>vertical)scale=vertical;
    return scale>TEXT_MAX_SCALE?TEXT_MAX_SCALE:scale;
}
int text_render_row(TextPixel *pixels,size_t capacity,const unsigned char *cells,
                    const unsigned char *colors,unsigned count,unsigned set,
                    unsigned scale,int cursor){
    if(!pixels||!cells||!colors||count>80||set>=CHARSET_COUNT||!scale||scale>TEXT_MAX_SCALE)return 0;
    unsigned width=count*8*scale;
    if(capacity<(size_t)width*8*scale)return 0;
    for(unsigned y=0;y<8*scale;++y)for(unsigned c=0;c<count;++c){
        unsigned char bits=charset_row(set,cells[c],y/scale);
        if(colors[c]&128)bits=(unsigned char)~bits;
        if((int)c==cursor)bits=(unsigned char)~bits;
        TextPixel fg=text_palette[colors[c]&15],bg=text_palette[6];
        for(unsigned x=0;x<8*scale;++x)
            pixels[(size_t)y*width+c*8*scale+x]=(bits&(128>>(x/scale)))?fg:bg;
    }
    return 1;
}
