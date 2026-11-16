#define _POSIX_C_SOURCE 200809L
#include "basic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
static int interactive,rootfd=-1,pending;
static const int ansi_colors[16]={30,97,91,96,95,92,94,93,33,33,91,90,37,92,94,37};
static struct termios original;
static volatile sig_atomic_t interrupted;
static void on_signal(int n){(void)n;interrupted=1;}
static void restore(void){if(interactive){tcsetattr(0,TCSAFLUSH,&original);fputs("\033[0m\033[?25h\033[?1049l",stdout);}}
void platform_trace(int c){if(!interactive){if(c==13)c=10;fputc(c,stdout);fflush(stdout);}}
static void put_unicode(uint32_t c){
    if(c<128)fputc((int)c,stdout);
    else if(c<2048){fputc(192|(c>>6),stdout);fputc(128|(c&63),stdout);}
    else if(c<65536){fputc(224|(c>>12),stdout);fputc(128|((c>>6)&63),stdout);fputc(128|(c&63),stdout);}
    else {fputc(240|(c>>18),stdout);fputc(128|((c>>12)&63),stdout);fputc(128|((c>>6)&63),stdout);fputc(128|(c&63),stdout);}
}
void platform_info(void){screen_text("DISPLAY: TERMINAL; ISO UNICODE / APPROXIMATE PETSCII GLYPHS\r");}
void platform_draw(void){
    if(!interactive || !screen.dirty)return;
    fputs("\033[?25l\033[44m\033[97m\033[H",stdout);
    for(int y=0;y<ROWS;++y){
        printf("\033[%d;1H",y+1);
        int color=-1;
        for(int x=0;x<COLS;++x){
            int next=ansi_colors[screen.color[y][x]&15];
            if(next!=color){printf("\033[%dm",next);color=next;}
            int reverse=(screen.color[y][x]&128) || (screen.charset!=CHARSET_ISO && (screen.cells[y][x]&128));
            printf("\033[%dm",reverse?7:27);
            put_unicode(charset_unicode(screen.charset,screen.cells[y][x]));
        }
    }
    printf("\033[27m\033[25;1H\033[36m  BASIC EFI 0.1  |  F9 RESET  |  ESC BREAK  |  HELP  |  EXIT\033[K");
    printf("\033[%d;%dH\033[97m\033[?25h",screen.y+1,screen.x+1);fflush(stdout);screen.dirty=0;
}
static int ready(int us){fd_set f;FD_ZERO(&f);FD_SET(0,&f);struct timeval t={0,us};return select(1,&f,0,0,&t)>0;}
static int readkey(void){
    unsigned char c;if(read(0,&c,1)!=1)return KEY_EXIT;
    if(c==3)return KEY_ESCAPE;
    if(c==4)return KEY_EXIT;
    if(c!=27){
        /* Batch input is bytes in the selected encoding; terminals supply UTF-8. */
        if(!interactive)return c;
        uint32_t u=c;
        if(c>=128){
            unsigned n=c>=0xc2&&c<=0xdf?1:c>=0xe0&&c<=0xef?2:c>=0xf0&&c<=0xf4?3:0;
            if(!n)return 0;
            u=c&((1u<<(6-n))-1);
            for(unsigned i=0;i<n;++i){
                if(!ready(30000)||read(0,&c,1)!=1 || (c&192)!=128)return 0;
                u=(u<<6)|(c&63);
            }
            if((n==1&&u<128)||(n==2&&u<2048)||(n==3&&u<65536)||u>0x10ffff||(u>=0xd800&&u<=0xdfff))return 0;
        }
        return charset_input(screen.charset,u);
    }
    if(!ready(30000))return KEY_ESCAPE;
    if(read(0,&c,1)!=1)return KEY_ESCAPE;
    if(c!='['&&c!='O')return KEY_ESCAPE;
    if(!ready(30000)||read(0,&c,1)!=1)return KEY_ESCAPE;
    switch(c){case 'A':return KEY_UP;case 'B':return KEY_DOWN;case 'C':return KEY_RIGHT;
    case 'D':return KEY_LEFT;case 'H':return KEY_HOME;case 'F':return KEY_END;}
    char seq[8]={0};int n=0;seq[n++]=(char)c;
    while(n<7 && ready(30000)){if(read(0,&c,1)!=1)break;seq[n++]=(char)c;if(c=='~')break;}
    if(!strcmp(seq,"3~"))return KEY_DELETE;
    if(!strcmp(seq,"20~"))return KEY_RESET;
    return 0;
}
int platform_key(int wait){
    if(interrupted){interrupted=0;return KEY_ESCAPE;}
    if(wait!=0 && pending){int v=pending;pending=0;return v;}
    if(!wait){
        if(!interactive||!ready(0))return 0;
        int v=readkey();
        if(v==KEY_ESCAPE||v==KEY_RESET||v==KEY_EXIT){pending=0;return v;}
        if(!pending)pending=v;
        return 0;
    }
    if(wait==-1 && !ready(0))return 0;
    return readkey();
}
void platform_idle(void){}
int platform_read(const char *name,unsigned char *buffer,size_t cap,size_t *length){
    if(!valid_filename(name))return 3;
    int fd=openat(rootfd,name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0)return errno==ENOENT?1:3;
    struct stat st;if(fstat(fd,&st)||!S_ISREG(st.st_mode)){close(fd);return 3;}
    if(st.st_size<0 || (uint64_t)st.st_size>cap){close(fd);return 4;}
    size_t used=0;while(used<(size_t)st.st_size){ssize_t n=read(fd,buffer+used,(size_t)st.st_size-used);if(n<=0){close(fd);return 3;}used+=(size_t)n;}
    close(fd);*length=used;return 0;
}
int platform_write(const char *name,const unsigned char *buffer,size_t length){
    if(!valid_filename(name))return 3;
    int fd=openat(rootfd,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
    if(fd<0)return errno==EEXIST?2:3;
    size_t used=0;int fail=0;
    while(used<length){ssize_t n=write(fd,buffer+used,length-used);if(n<=0){fail=1;break;}used+=(size_t)n;}
    if(fsync(fd))fail=1;
    if(close(fd))fail=1;
    if(fail){unlinkat(rootfd,name,0);return 3;}return 0;
}
void platform_directory(void){
    int fd=openat(rootfd,".",O_RDONLY|O_DIRECTORY);if(fd<0){screen_text("?DIRECTORY ERROR\r");return;}
    DIR *d=fdopendir(fd);if(!d){close(fd);return;}
    screen_text("FILES IN BASIC:\r");struct dirent *ent;
    while((ent=readdir(d))){struct stat st;if(!valid_filename(ent->d_name))continue;
        if(fstatat(rootfd,ent->d_name,&st,AT_SYMLINK_NOFOLLOW)||!S_ISREG(st.st_mode))continue;
        screen_text("  ");screen_text(ent->d_name);screen_text("\r");
    }closedir(d);
}
int main(int argc,char **argv){
    const char *dir=argc>1?argv[1]:"programs";
    rootfd=open(dir,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(rootfd<0){perror(dir);return 1;}
    interactive=isatty(0)&&isatty(1);
    if(interactive){tcgetattr(0,&original);struct termios t=original;t.c_lflag&=(tcflag_t)~(ICANON|ECHO|ISIG);t.c_iflag&=(tcflag_t)~(IXON|ICRNL);t.c_cc[VMIN]=1;t.c_cc[VTIME]=0;tcsetattr(0,TCSAFLUSH,&t);fputs("\033[?1049h",stdout);atexit(restore);}
    signal(SIGINT,on_signal);int result=basic_run();close(rootfd);return result;
}
