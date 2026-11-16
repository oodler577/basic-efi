#include "iec.h"
#include <string.h>

/* No bus bit-banging, fastloaders, firmware updates or disk-write API. */
static int fault(Iec *s,int error){s->connected=0;return error;}
static int check(Iec *s){
    if(!s->connected)return IEC_OFFLINE;
    if(s->io.cancelled && s->io.cancelled(s->io.context))return fault(s,IEC_CANCELLED);
    return IEC_OK;
}
static int transfer(Iec *s,unsigned ep,unsigned char *data,size_t want,size_t *actual){
    int e=check(s);if(e)return e;
    size_t n=want;e=s->io.bulk(s->io.context,ep,data,&n,3000);
    if(e)return fault(s,e);
    if(n>want || (ep==IEC_OUT && n!=want))return fault(s,IEC_PROTOCOL);
    *actual=n;return IEC_OK;
}
static int send(Iec *s,unsigned char *data,size_t n){size_t actual;return transfer(s,IEC_OUT,data,n,&actual);}
static int status(Iec *s,unsigned *value){
    /* Finite even for an adapter that returns BUSY forever. No command retries. */
    for(unsigned i=0;i<8;++i){
        unsigned char reply[3];size_t n;int e=transfer(s,IEC_IN,reply,3,&n);
        if(e)return e;
        if(n!=3)return fault(s,IEC_PROTOCOL);
        if(reply[0]==2){*value=(unsigned)reply[1]+256u*reply[2];return IEC_OK;}
        if(reply[0]==3)return fault(s,IEC_DRIVE);
        if(reply[0]!=1)return fault(s,IEC_PROTOCOL);
        if(s->io.pause)s->io.pause(s->io.context,10);
    }
    return fault(s,IEC_TIMEOUT);
}
static int command(Iec *s,unsigned cmd,unsigned *value){
    unsigned char packet[4]={(unsigned char)cmd,0,0,0};
    int e=send(s,packet,sizeof(packet));return e?e:status(s,value);
}
static int write_bus(Iec *s,unsigned mode,const unsigned char *bytes,size_t n){
    unsigned char packet[4]={9,(unsigned char)mode,(unsigned char)n,(unsigned char)(n>>8)};
    unsigned value;int e=send(s,packet,sizeof(packet));if(e)return e;
    /* Only short addressing/filename messages are exposed by this module. */
    unsigned char copy[24];if(n>sizeof(copy))return fault(s,IEC_PROTOCOL);
    memcpy(copy,bytes,n);e=send(s,copy,n);if(e)return e;
    e=status(s,&value);if(e)return e;
    return value==n?IEC_OK:fault(s,IEC_DRIVE);
}
static int address(Iec *s,unsigned dev,unsigned second,int talk){
    unsigned char bytes[2]={(unsigned char)((talk?0x40:0x20)|dev),(unsigned char)second};
    return write_bus(s,talk?0x13:0x12,bytes,2);
}
static int release(Iec *s,int talk){
    unsigned char byte=talk?0x5f:0x3f;return write_bus(s,0x12,&byte,1);
}
static int close_file(Iec *s,unsigned dev,unsigned channel){
    int e=address(s,dev,0xe0|channel,0);return e?e:release(s,0);
}
int iec_connect(Iec *s,const IecTransport *io){
    memset(s,0,sizeof(*s));
    if(!io || !io->control || !io->bulk)return IEC_UNSUPPORTED;
    s->io=*io;
    if(io->cancelled && io->cancelled(io->context))return IEC_CANCELLED;
    unsigned char info[8]={0};
    int e=io->control(io->context,0xa0,1,0,0,info,sizeof(info),3000);if(e)return e;
    if(info[0]<7 || info[0]>8 || !(info[1]&1) || (info[2]&0x30)){
        /* INIT succeeded but this is an unsupported mode. No bus transactions. */
        io->control(io->context,0x20,3,0,0,NULL,0,1000);return IEC_UNSUPPORTED;
    }
    if(info[2]&1){
        e=io->control(io->context,2,1,0,IEC_IN,NULL,0,1000);if(e)return e;
        e=io->control(io->context,2,1,0,IEC_OUT,NULL,0,1000);if(e)return e;
    }
    s->version=info[0];s->connected=1;return IEC_OK;
}
void iec_disconnect(Iec *s){
    /* On a failed transfer preserve the adapter's unfinished-session flag:
       next INIT performs its firmware recovery. SHUTDOWN would erase it. */
    if(s->connected)s->io.control(s->io.context,0x20,3,0,0,NULL,0,1000);
    s->connected=0;
}
static int read_stream(Iec *s,unsigned char *out,size_t cap,size_t *length){
    size_t used=0;*length=0;
    if(cap>IEC_LIMIT)cap=IEC_LIMIT;
    for(;;){
        unsigned char bytes[IEC_CHUNK],packet[4]={8,0x10,IEC_CHUNK,0};
        size_t n;unsigned eoi;int e=send(s,packet,4);if(e)return e;
        e=transfer(s,IEC_IN,bytes,sizeof(bytes),&n);if(e)return e;
        /* READ returns bytes directly, without the WRITE/ioctl status packet. */
        e=command(s,23,&eoi);if(e)return e;
        if(eoi>1)return fault(s,IEC_PROTOCOL);
        if(n>cap-used)return IEC_TOO_LARGE;
        memcpy(out+used,bytes,n);used+=n;
        if(eoi){*length=used;return IEC_OK;}
        if(n<IEC_CHUNK)return fault(s,IEC_PROTOCOL);
    }
}
int iec_status(Iec *s,unsigned dev,unsigned *code){
    s->drive_status[0]=0;
    if(dev<8 || dev>30)return IEC_PROTOCOL;
    unsigned value;int e=command(s,24,&value);if(e)return e;
    e=address(s,dev,0x6f,1);if(e)return e;
    unsigned char bytes[96];size_t n;e=read_stream(s,bytes,sizeof(bytes),&n);
    int cleanup=s->connected?release(s,1):IEC_OK;
    if(e)return e;
    if(cleanup)return cleanup;
    if(n<4 || bytes[n-1]!=13 || bytes[0]<'0' || bytes[0]>'9' ||
       bytes[1]<'0' || bytes[1]>'9' || bytes[2]!=',')return IEC_PROTOCOL;
    for(size_t i=0;i<n-1;++i){if(bytes[i]<32 || bytes[i]>126)return IEC_PROTOCOL;}
    memcpy(s->drive_status,bytes,n-1);s->drive_status[n-1]=0;
    *code=10u*(bytes[0]-'0')+(bytes[1]-'0');return IEC_OK;
}
static int read_named(Iec *s,unsigned dev,const char *name,unsigned channel,
                      unsigned char *out,size_t cap,size_t *length){
    *length=0;s->drive_status[0]=0;
    if(dev<8 || dev>30 || !out)return IEC_PROTOCOL;
    int e=address(s,dev,0xf0|channel,0);if(e)return e;
    e=write_bus(s,0x10,(const unsigned char*)name,strlen(name));if(e)return e;
    e=release(s,0);if(e)return e;
    /* Read DOS status before TALK: a missing file must not hang waiting for data. */
    unsigned code;e=iec_status(s,dev,&code);
    if(!e && code>=20)e=IEC_DRIVE;
    if(!e){
        unsigned value;e=command(s,24,&value);
        if(!e)e=address(s,dev,0x60|channel,1);
        if(!e){
            e=read_stream(s,out,cap,length);
            int cleanup=s->connected?release(s,1):IEC_OK;
            if(!e)e=cleanup;
        }
    }
    int cleanup=s->connected?close_file(s,dev,channel):IEC_OK;
    if(!e)e=cleanup;
    if(!e){e=iec_status(s,dev,&code);if(!e && code>=20)e=IEC_DRIVE;}
    if(e)*length=0;
    return e;
}
int iec_valid_name(const char *name){
    size_t n=strlen(name);if(!n || n>16)return 0;
    for(size_t i=0;i<n;++i){unsigned char c=(unsigned char)name[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||
             c==' '||c=='.'||c=='_'||c=='-'))return 0;
    }
    return name[0]!=' ' && name[n-1]!=' ';
}
static int read_typed(Iec *s,unsigned dev,const char *name,int prg,unsigned char *out,size_t cap,size_t *length){
    *length=0;if(!iec_valid_name(name))return IEC_BAD_NAME;
    /* Force SEQ or PRG, always read mode. Never pass user DOS syntax. */
    char request[23]="0:";size_t n=strlen(name);
    for(size_t i=0;i<n;++i)request[i+2]=name[i]>='a'&&name[i]<='z'?(char)(name[i]-32):name[i];
    memcpy(request+n+2,prg?",P,R":",S,R",5);
    return read_named(s,dev,request,2,out,cap,length);
}
int iec_read_file(Iec *s,unsigned dev,const char *name,unsigned char *out,size_t cap,size_t *length){
    return read_typed(s,dev,name,0,out,cap,length);
}
int iec_read_program(Iec *s,unsigned dev,const char *name,unsigned char *out,size_t cap,size_t *length){
    return read_typed(s,dev,name,1,out,cap,length);
}
int iec_read_directory(Iec *s,unsigned dev,unsigned char *out,size_t cap,size_t *length){
    return read_named(s,dev,"$0",0,out,cap,length);
}
static unsigned le16(const unsigned char *p){return (unsigned)p[0]+256u*p[1];}
int iec_parse_directory(const unsigned char *data,size_t n,IecDirectoryLine line,void *context){
    /* Sequential BASIC directory records. Never follow disk-supplied pointers. */
    if(n<4 || n>IEC_LIMIT)return IEC_BAD_DIRECTORY;
    size_t p=2;
    while(p+2<=n){
        unsigned link=le16(data+p);p+=2;
        if(!link)return p==n?IEC_OK:IEC_BAD_DIRECTORY;
        if(p+2>n)return IEC_BAD_DIRECTORY;
        unsigned blocks=le16(data+p);p+=2;size_t start=p;
        while(p<n && data[p] && p-start<=80)++p;
        if(p==n || p-start>80)return IEC_BAD_DIRECTORY;
        if(line)line(context,blocks,data+start,p-start);
        ++p;
    }
    return IEC_BAD_DIRECTORY;
}
const char *iec_error(int e){
    switch(e){
    case IEC_OK:return "OK";
    case IEC_NO_ADAPTER:return "NO ZOOMFLOPPY/XUM1541 FOUND";
    case IEC_MULTIPLE:return "MULTIPLE ADAPTERS; CONNECT ONLY ONE";
    case IEC_UNSUPPORTED:return "UNSUPPORTED USB INTERFACE, FIRMWARE OR BUS MODE";
    case IEC_USB:return "USB TRANSFER FAILED OR ADAPTER DISCONNECTED";
    case IEC_TIMEOUT:return "TRANSFER TIMEOUT";
    case IEC_CANCELLED:return "CANCELLED";
    case IEC_PROTOCOL:return "INVALID OR SHORT ADAPTER RESPONSE";
    case IEC_DRIVE:return "DRIVE ERROR OR DEVICE NOT PRESENT";
    case IEC_TOO_LARGE:return "FILE TOO LARGE";
    case IEC_BAD_NAME:return "USE 1-16 LETTERS, DIGITS, SPACES, DOT, DASH OR UNDERSCORE";
    case IEC_BAD_DIRECTORY:return "INVALID DIRECTORY DATA";
    default:return "OFFLINE; USE IEC ON TO RECONNECT";
    }
}
