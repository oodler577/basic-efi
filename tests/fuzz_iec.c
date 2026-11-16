#include "iec.h"
#include "mock_xum.h"
#include <string.h>
typedef struct {const uint8_t *data;size_t size,pos;} Input;
static unsigned char byte(Input *i){return i->pos<i->size?i->data[i->pos++]:0;}
static int control(void *ctx,unsigned t,unsigned r,unsigned v,unsigned x,unsigned char *b,size_t n,unsigned ms){
    (void)ctx;(void)t;(void)r;(void)v;(void)x;(void)ms;
    if(n){memset(b,0,n);b[0]=8;b[1]=1;}return IEC_OK;
}
static int bulk(void *ctx,unsigned ep,unsigned char *b,size_t *n,unsigned ms){
    (void)ms;Input *i=ctx;if(i->pos>=i->size)return IEC_USB;
    if(ep==IEC_OUT){if(byte(i)==255){*n=0;}return IEC_OK;}
    size_t amount=byte(i);if(amount>*n)amount=*n;
    if(amount>i->size-i->pos)amount=i->size-i->pos;
    memcpy(b,i->data+i->pos,amount);i->pos+=amount;*n=amount;return IEC_OK;
}
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size){
    if(size>IEC_LIMIT)return 0;
    (void)iec_parse_directory(data,size,NULL,NULL);
    Input input={data,size,0};IecTransport io={&input,control,bulk,NULL,NULL};Iec s;
    unsigned char buffer[512];size_t n;unsigned code;
    (void)iec_connect(&s,&io);(void)iec_read_file(&s,8,"FUZZ",buffer,sizeof(buffer),&n);iec_disconnect(&s);
    input.pos=0;(void)iec_connect(&s,&io);(void)iec_status(&s,8,&code);iec_disconnect(&s);
    char name[65];size_t len=size<64?size:64;memcpy(name,data,len);name[len]=0;(void)iec_valid_name(name);
    MockXum peer;mock_xum_init(&peer);peer.file=data;peer.file_length=size;
    io=mock_xum_transport(&peer);(void)iec_connect(&s,&io);
    (void)iec_read_file(&s,8,"FUZZ",buffer,sizeof(buffer),&n);iec_disconnect(&s);
    return 0;
}
