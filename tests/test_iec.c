#include "mock_xum.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static MockXum mock;
static Iec session;
static unsigned char buffer[IEC_LIMIT+2],large_file[IEC_LIMIT+1];
static void connect_mock(void){
    mock_xum_init(&mock);IecTransport io=mock_xum_transport(&mock);
    assert(iec_connect(&session,&io)==IEC_OK);assert(session.connected && session.version==8);
}
static unsigned lines;
static void line(void *ctx,unsigned blocks,const unsigned char *data,size_t n){
    (void)ctx;(void)blocks;assert(n<=80 && data);++lines;
}
static void read_ok(void){
    size_t n=123;assert(iec_read_file(&session,9,"hello",buffer,sizeof(buffer),&n)==IEC_OK);
    assert(n==mock.file_length && !memcmp(buffer,mock.file,n));
    assert(!strcmp(mock.filename,"0:HELLO,S,R"));
    assert(mock.closed==1 && !mock.pending && !mock.talking && !mock.bad);
}
int main(void){
    connect_mock();read_ok();unsigned calls=mock.calls,cancel_calls=mock.cancel_calls;
    iec_disconnect(&session);assert(!session.connected && mock.shutdowns==1);
    for(unsigned version=0;version<=9;++version){
        mock_xum_init(&mock);mock.version=version;IecTransport io=mock_xum_transport(&mock);
        assert(iec_connect(&session,&io)==((version==7||version==8)?IEC_OK:IEC_UNSUPPORTED));
    }
    for(unsigned state=0x10;state<=0x30;state+=0x10){
        mock_xum_init(&mock);mock.state=state;IecTransport io=mock_xum_transport(&mock);
        assert(iec_connect(&session,&io)==IEC_UNSUPPORTED && mock.shutdowns==1);
    }
    mock_xum_init(&mock);mock.capabilities=0;IecTransport io=mock_xum_transport(&mock);
    assert(iec_connect(&session,&io)==IEC_UNSUPPORTED);
    mock_xum_init(&mock);mock.state=1;io=mock_xum_transport(&mock);
    assert(iec_connect(&session,&io)==IEC_OK && mock.clear_halts==3);read_ok();
    for(unsigned fail=1;fail<=calls;++fail){
        connect_mock();mock.fail_at=fail;size_t n=100;
        assert(iec_read_file(&session,8,"HELLO",buffer,sizeof(buffer),&n)==IEC_TIMEOUT);
        assert(!n && !session.connected && mock.calls==fail && !mock.bad);
        iec_disconnect(&session);assert(!mock.shutdowns);
        assert(iec_read_file(&session,8,"HELLO",buffer,sizeof(buffer),&n)==IEC_OFFLINE);
    }
    for(unsigned at=2;at<=cancel_calls;++at){
        connect_mock();mock.cancel_at=at;size_t n=10;
        assert(iec_read_file(&session,8,"HELLO",buffer,sizeof(buffer),&n)==IEC_CANCELLED);
        assert(!n && !session.connected && !mock.bad);
    }
    for(unsigned short_call=1;short_call<=3;++short_call){
        connect_mock();mock.short_at=short_call;size_t n;
        assert(iec_read_file(&session,8,"HELLO",buffer,sizeof(buffer),&n)==IEC_PROTOCOL);
        assert(!session.connected && !n && mock.calls==short_call);
    }
    connect_mock();mock.status_mode=1;unsigned code;
    assert(iec_status(&session,8,&code)==IEC_TIMEOUT && mock.pauses==80 && mock.calls==9);
    connect_mock();mock.status_mode=3;
    assert(iec_status(&session,8,&code)==IEC_DRIVE);
    connect_mock();mock.status_mode=99;
    assert(iec_status(&session,8,&code)==IEC_PROTOCOL);
    connect_mock();mock.wrong_count=1;size_t n;
    assert(iec_read_file(&session,8,"HELLO",buffer,sizeof(buffer),&n)==IEC_DRIVE);
    connect_mock();mock.short_read=1;
    assert(iec_status(&session,8,&code)==IEC_PROTOCOL && !session.connected);
    connect_mock();mock.dos_status="BAD STATUS\r";
    assert(iec_status(&session,8,&code)==IEC_PROTOCOL && !mock.talking);
    connect_mock();assert(iec_read_file(&session,8,"MISSING",buffer,sizeof(buffer),&n)==IEC_DRIVE);
    assert(!n && session.connected && mock.closed==1 && mock.reads==1);
    assert(!strcmp(session.drive_status,"62, FILE NOT FOUND,00,00"));
    for(size_t size=0;size<=257;++size){
        connect_mock();mock.file=large_file;mock.file_length=size;
        memset(buffer,0xa5,sizeof(buffer));
        assert(iec_read_file(&session,8,"BOUNDARY",buffer,size,&n)==IEC_OK && n==size);
        assert(buffer[size]==0xa5 && !mock.bad);
        if(size){
            connect_mock();mock.file=large_file;mock.file_length=size;
            assert(iec_read_file(&session,8,"BOUNDARY",buffer,size-1,&n)==IEC_TOO_LARGE);
            assert(!n && mock.closed==1 && !mock.talking);
        }
    }
    connect_mock();mock.file=large_file;mock.file_length=sizeof(large_file);
    assert(iec_read_file(&session,8,"LARGE",buffer,sizeof(buffer),&n)==IEC_TOO_LARGE);
    assert(!n && session.connected && mock.closed==1);
    connect_mock();assert(iec_read_directory(&session,8,buffer,sizeof(buffer),&n)==IEC_OK);
    assert(iec_parse_directory(buffer,n,line,NULL)==IEC_OK && lines==3);
    for(size_t size=0;size<n;++size)assert(iec_parse_directory(buffer,size,NULL,NULL)==IEC_BAD_DIRECTORY);
    buffer[n]=0;assert(iec_parse_directory(buffer,n+1,NULL,NULL)==IEC_BAD_DIRECTORY);
    unsigned char long_line[90]={1,8,1,1,0,0};memset(long_line+6,'X',81);
    assert(iec_parse_directory(long_line,sizeof(long_line),NULL,NULL)==IEC_BAD_DIRECTORY);
    assert(iec_valid_name("A 16 CHAR NAME!!")==0);
    assert(iec_valid_name("ABC 0123_-.") && iec_valid_name("1234567890123456"));
    const char *invalid[]={""," "," NAME","NAME ","12345678901234567","@NAME","N:DISK","$","*","?","A,B","A\rB","A/B"};
    for(size_t i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i){
        connect_mock();assert(!iec_valid_name(invalid[i]));
        assert(iec_read_file(&session,8,invalid[i],buffer,sizeof(buffer),&n)==IEC_BAD_NAME && !mock.calls && !n);
    }
    puts("PASS: IEC wire protocol, DOS status, SEQ reads, directory bounds, EOF boundaries, cancellation and failure at every USB transfer");
    return 0;
}
