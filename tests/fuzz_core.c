/* Include the core to fuzz private parsers without exporting test-only EFI APIs. */
#include "../src/core.c"
void platform_trace(int c){(void)c;}
void platform_draw(void){}
int platform_key(int wait){(void)wait;return KEY_EXIT;}
int platform_read(const char *n,unsigned char *b,size_t c,size_t *l){(void)n;(void)b;(void)c;(void)l;return 1;}
int platform_write(const char *n,const unsigned char *b,size_t l){(void)n;(void)b;(void)l;return 0;}
void platform_directory(void){}
void platform_idle(void){}
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size){
    if(size>32768)return 0;
    exiting=0;screen_clear();reset_machine();
    size_t n=size<LINE_MAXIMUM?size:LINE_MAXIMUM;
    char line[LINE_MAXIMUM+1];memcpy(line,data,n);line[n]=0;
    normalize(line);(void)monitor(line);
    bits=8u<<(size?data[0]%4:0);calc(line);
    memcpy(filebuf,data,size);(void)validate_source(size);
    /* Corrupt virtual program links and contents, never native memory pointers. */
    memcpy(memory+0x400,data,size>0x7c00?0x7c00:size);
    if(size>=4){memcpy(memory+ROM_TXTTAB,data,2);memcpy(memory+ROM_VARTAB,data+2,2);}
    size_t length=0;(void)export_program(&length);
    return 0;
}
