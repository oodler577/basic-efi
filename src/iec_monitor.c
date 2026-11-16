#include "basic.h"
#include "iec.h"
#include <string.h>
static Iec session;
static int selected;
static unsigned device=8;
int storage_iec_selected(void){return selected;}
unsigned storage_iec_device(void){return device;}
static void number(unsigned n){
    char text[6],reverse[5];unsigned i=0,j=0;
    do{reverse[i++]=(char)('0'+n%10);n/=10;}while(n && i<5);
    while(i)text[j++]=reverse[--i];
    text[j]=0;screen_text(text);
}
static void error(int e){
    screen_text("?IEC ");screen_text(iec_error(e));screen_text("\r");
    if(e==IEC_DRIVE && session.drive_status[0]>='2' && session.drive_status[0]<='9'){
        screen_text(session.drive_status);screen_text("\r");
    }
    if(selected && !session.connected)screen_text("IEC REMAINS SELECTED; IEC ON RECONNECTS, IEC OFF SELECTS FAT.\r");
}
void storage_iec_info(void){
    if(!selected){screen_text("STORAGE: FAT BASIC FOLDER; IEC OFF\r");return;}
    screen_text("STORAGE: IEC DEVICE ");number(device);
    screen_text(session.connected?"; READ ONLY; XUM FIRMWARE ":"; OFFLINE; READ ONLY\r");
    if(session.connected){number(session.version);screen_text("; HARDWARE UNVERIFIED\r");}
}
void storage_iec_close(void){iec_disconnect(&session);selected=0;}
int storage_iec_command(const char *arg){
    while(*arg==' ')++arg;
    if(!*arg){storage_iec_info();return 1;}
    if(!strcmp(arg,"OFF")){storage_iec_close();storage_iec_info();return 1;}
    if(!strcmp(arg,"ON")){
        iec_disconnect(&session);IecTransport io;
        int e=platform_iec_transport(&io);
        if(!e)e=iec_connect(&session,&io);
        if(e)error(e);else{selected=1;storage_iec_info();}
        return 1;
    }
    if(!strncmp(arg,"DEVICE ",7)){
        const char *p=arg+7;while(*p==' ')++p;unsigned n=0,digits=0;
        while(*p>='0' && *p<='9' && digits<2){n=10*n+(unsigned)(*p++-'0');++digits;}
        while(*p==' ')++p;
        if(*p || !digits || n<8 || n>30)screen_text("?USE IEC DEVICE 8 THROUGH 30\r");
        else{device=n;screen_text("IEC DEVICE ");number(device);screen_text("\r");}
        return 1;
    }
    if(!strcmp(arg,"STATUS")){
        if(!selected){screen_text("?USE IEC ON FIRST\r");return 1;}
        unsigned code;int e=iec_status(&session,device,&code);
        if(e)error(e);else{screen_text(session.drive_status);screen_text("\r");}
        return 1;
    }
    screen_text("?USE IEC [ON|OFF|STATUS|DEVICE 8..30]; DIR AND LOAD USE SELECTED STORAGE\r");return 1;
}
int storage_iec_read(const char *name,int prg,unsigned char *buffer,size_t cap,size_t *length){
    int e=prg?iec_read_program(&session,device,name,buffer,cap,length):iec_read_file(&session,device,name,buffer,cap,length);
    if(e)error(e);
    return e;
}
static void directory_line(void *ctx,unsigned blocks,const unsigned char *text,size_t n){
    (void)ctx;number(blocks);screen_text(" ");
    /* Disk bytes must not inject CLS, charset changes, cursor or color controls.
       Render a portable directory in either active character set. */
    for(size_t i=0;i<n;++i){unsigned c=text[i];
        if(c==18 || c==146)continue;
        if(c==160)c=' ';
        if(c>=193 && c<=218)c-=128;
        if(c<32 || c>95)c='?';
        char one[2]={(char)c,0};screen_text(one);
    }
    screen_text("\r");
}
void storage_iec_directory(unsigned char *buffer,size_t cap){
    size_t n;int e=iec_read_directory(&session,device,buffer,cap,&n);
    /* Validate the whole directory before displaying any records. */
    if(!e)e=iec_parse_directory(buffer,n,NULL,NULL);
    if(e){error(e);return;}
    screen_text("IEC DIRECTORY, DEVICE ");number(device);screen_text("\r");
    (void)iec_parse_directory(buffer,n,directory_line,NULL);
}
