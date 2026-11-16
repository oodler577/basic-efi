#include "mock_xum.h"
#include <string.h>
static const unsigned char file_data[]="10 PRINT \"IEC MOCK READ\"\n20 END\n";
static const unsigned char directory_data[]={
    1,8, 1,1, 0,0, 18,'"','I','E','C',' ','T','E','S','T','"',' ','I','D',' ','2','A',0,
    1,1, 1,0, ' ','"','H','E','L','L','O','"',' ','S','E','Q',0,
    1,1, 152,2, 'B','L','O','C','K','S',' ','F','R','E','E','.',0, 0,0
};
static int bad(MockXum *m){++m->bad;return IEC_PROTOCOL;}
void mock_xum_init(MockXum *m){
    memset(m,0,sizeof(*m));m->version=8;m->capabilities=1;
    m->file=file_data;m->file_length=sizeof(file_data)-1;
    m->directory=directory_data;m->directory_length=sizeof(directory_data);
    m->dos_status="00, OK,00,00\r";
}
int mock_xum_control(void *ctx,unsigned type,unsigned request,unsigned value,unsigned index,
                     unsigned char *data,size_t n,unsigned timeout){
    MockXum *m=ctx;if(!timeout || timeout>3000 || value)return bad(m);
    if(type==0xa0 && request==1 && !index && n==8){
        memset(data,0,n);data[0]=(unsigned char)m->version;data[1]=(unsigned char)m->capabilities;
        data[2]=(unsigned char)m->state;m->initialized=1;m->pending=0;return IEC_OK;
    }
    if(type==0x20 && request==3 && !index && !n){++m->shutdowns;m->initialized=0;return IEC_OK;}
    if(type==2 && request==1 && !n && (index==IEC_IN || index==IEC_OUT)){
        m->clear_halts|=index==IEC_IN?1:2;return IEC_OK;
    }
    return bad(m);
}
static int bus_write(MockXum *m,const unsigned char *b,size_t n){
    if(m->mode==0x10){
        if(n>=sizeof(m->filename))return bad(m);
        memcpy(m->filename,b,n);m->filename[n]=0;
        if((m->channel==0 && strcmp(m->filename,"$0")) ||
           (m->channel==2 && (n<7 || memcmp(b,"0:",2) || memcmp(b+n-4,",S,R",4))) ||
           (m->channel!=0 && m->channel!=2))return bad(m);
        if(!strcmp(m->filename,"0:MISSING,S,R"))m->dos_status="62, FILE NOT FOUND,00,00\r";
        else m->dos_status="00, OK,00,00\r";
        return IEC_OK;
    }
    if(m->mode!=0x12 && m->mode!=0x13)return bad(m);
    if(n==1){
        if(b[0]!=0x3f && b[0]!=0x5f)return bad(m);
        if(b[0]==0x5f)m->talking=0;
        return IEC_OK;
    }
    if(n!=2 || (b[0]&31)<8 || (b[0]&31)>30)return bad(m);
    unsigned secondary=b[1]&15;
    if(m->mode==0x13){
        if((b[0]&0xe0)!=0x40 || (b[1]&0xf0)!=0x60)return bad(m);
        m->talking=1;m->position=0;
        if(secondary==15){m->stream=(const unsigned char*)m->dos_status;m->stream_length=strlen(m->dos_status);}
        else if(secondary==0){m->stream=m->directory;m->stream_length=m->directory_length;}
        else if(secondary==2){m->stream=m->file;m->stream_length=m->file_length;}
        else return bad(m);
    }else{
        if((b[0]&0xe0)!=0x20)return bad(m);
        if((b[1]&0xf0)==0xf0){m->channel=secondary;++m->opens;}
        else if((b[1]&0xf0)==0xe0){++m->closed;}
        else return bad(m);
    }
    return IEC_OK;
}
int mock_xum_bulk(void *ctx,unsigned ep,unsigned char *b,size_t *length,unsigned timeout){
    MockXum *m=ctx;size_t n=*length;
    if(!m->initialized || !timeout || timeout>3000)return bad(m);
    ++m->calls;
    if(m->calls==m->fail_at)return IEC_TIMEOUT;
    if(m->calls==m->short_at){*length=n?n-1:0;return IEC_OK;}
    if(ep==IEC_OUT){
        if(m->pending==1){
            if(n!=m->want)return bad(m);
            int e=bus_write(m,b,n);if(e)return e;
            m->value=(unsigned)n+(m->wrong_count?1:0);m->pending=2;return IEC_OK;
        }
        if(m->pending || n!=4)return bad(m);
        if(b[0]==9){m->mode=b[1];m->want=(unsigned)b[2]+256u*b[3];m->pending=1;return IEC_OK;}
        if(b[0]==8){
            if(b[1]!=0x10 || b[2]!=128 || b[3] || !m->talking)return bad(m);
            m->pending=3;return IEC_OK;
        }
        if((b[0]!=23 && b[0]!=24) || b[1] || b[2] || b[3])return bad(m);
        if(b[0]==24)m->eoi=0;
        m->value=b[0]==23?m->eoi:0;m->pending=2;return IEC_OK;
    }
    if(ep!=IEC_IN)return bad(m);
    if(m->pending==2){
        if(n!=3)return bad(m);
        b[0]=(unsigned char)(m->status_mode?m->status_mode:2);
        b[1]=(unsigned char)m->value;b[2]=(unsigned char)(m->value>>8);
        if(b[0]!=1)m->pending=0;
        return IEC_OK;
    }
    if(m->pending!=3 || n!=128)return bad(m);
    size_t remain=m->stream_length-m->position;if(n>remain)n=remain;
    if(m->short_read && n)n=1;
    memcpy(b,m->stream+m->position,n);m->position+=n;*length=n;
    m->eoi=m->short_read?0:m->position==m->stream_length;
    m->pending=0;++m->reads;return IEC_OK;
}
static int cancel(void *ctx){MockXum *m=ctx;return ++m->cancel_calls==m->cancel_at;}
static void pause_ms(void *ctx,unsigned ms){MockXum *m=ctx;m->pauses+=ms;}
IecTransport mock_xum_transport(MockXum *m){
    return (IecTransport){m,mock_xum_control,mock_xum_bulk,cancel,pause_ms};
}
