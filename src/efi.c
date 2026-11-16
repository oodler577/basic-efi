#include <efi.h>
#include <efilib.h>
#include "basic.h"
#include <string.h>
static EFI_SYSTEM_TABLE *st;
static EFI_BOOT_SERVICES *bs;
static EFI_FILE_HANDLE basic_dir;
static Screen previous;
static int have_previous,pending;
static const UINTN colors[16]={0,15,12,11,13,10,9,14,6,6,12,8,7,10,9,7};

void platform_trace(int c){
#ifdef BASIC_DEBUG_PORT
    /* Only the explicitly labelled QEMU test build writes the debug I/O port. */
    unsigned char v=(unsigned char)c;
    __asm__ volatile("outb %0,$0xe9"::"a"(v));
#else
    (void)c;
#endif
}
void platform_draw(void){
    if(!screen.dirty)return;
    st->ConOut->EnableCursor(st->ConOut,FALSE);
    for(int y=0;y<ROWS;++y){
        if(have_previous && !memcmp(screen.cells[y],previous.cells[y],COLS) && !memcmp(screen.color[y],previous.color[y],COLS))continue;
        int x=0;
        while(x<COLS){
            int start=x;unsigned color=screen.color[y][x]&15;CHAR16 text[COLS+1];int n=0;
            do{text[n++]=(CHAR16)(unsigned char)screen.cells[y][x++];}while(x<COLS&&(screen.color[y][x]&15)==color);
            text[n]=0;st->ConOut->SetAttribute(st->ConOut,EFI_BACKGROUND_BLUE|colors[color]);
            st->ConOut->SetCursorPosition(st->ConOut,(UINTN)start,(UINTN)y);
            st->ConOut->OutputString(st->ConOut,text);
        }
    }
    if(!have_previous){
        st->ConOut->SetCursorPosition(st->ConOut,0,24);
        st->ConOut->SetAttribute(st->ConOut,EFI_BACKGROUND_BLUE|EFI_LIGHTCYAN);
        st->ConOut->OutputString(st->ConOut,L" BASIC EFI 0.1 | HELP | ESC BREAK | F9 RESET | EXIT");
    }
    previous=screen;have_previous=1;screen.dirty=0;
    st->ConOut->SetAttribute(st->ConOut,EFI_BACKGROUND_BLUE|EFI_WHITE);
    st->ConOut->SetCursorPosition(st->ConOut,(UINTN)screen.x,(UINTN)screen.y);
    st->ConOut->EnableCursor(st->ConOut,TRUE);
}
static int decode(EFI_INPUT_KEY k){
    switch(k.ScanCode){case SCAN_UP:return KEY_UP;case SCAN_DOWN:return KEY_DOWN;
    case SCAN_LEFT:return KEY_LEFT;case SCAN_RIGHT:return KEY_RIGHT;case SCAN_HOME:return KEY_HOME;
    case SCAN_END:return KEY_END;case SCAN_DELETE:return KEY_DELETE;case SCAN_ESC:return KEY_ESCAPE;
    case SCAN_F9:return KEY_RESET;}
    if(k.UnicodeChar==3)return KEY_ESCAPE;
    return k.UnicodeChar<128?(int)k.UnicodeChar:0;
}
int platform_key(int wait){
    if(wait!=0&&pending){int k=pending;pending=0;return k;}
    EFI_INPUT_KEY key;
    for(;;){
        EFI_STATUS s=st->ConIn->ReadKeyStroke(st->ConIn,&key);
        if(!EFI_ERROR(s)){
            int k=decode(key);
            if(k==KEY_ESCAPE||k==KEY_RESET){pending=0;return k;}
            if(wait!=0)return k;
            if(!pending)pending=k;
            return 0;
        }
        if(wait!=1)return 0;
        UINTN index;bs->WaitForEvent(1,&st->ConIn->WaitForKey,&index);
    }
}
void platform_idle(void){}
static void filename16(const char *name,CHAR16 *out){while(*name)*out++=(CHAR16)(unsigned char)*name++;*out=0;}
int platform_read(const char *name,unsigned char *buffer,size_t cap,size_t *length){
    if(!basic_dir||!valid_filename(name))return 3;
    CHAR16 path[65];filename16(name,path);EFI_FILE_HANDLE f;
    EFI_STATUS s=basic_dir->Open(basic_dir,&f,path,EFI_FILE_MODE_READ,0);
    if(EFI_ERROR(s))return s==EFI_NOT_FOUND?1:3;
    union{UINT64 align;unsigned char bytes[512];}info;UINTN size=sizeof(info);
    s=f->GetInfo(f,&GenericFileInfo,&size,&info);
    if(EFI_ERROR(s)){f->Close(f);return 3;}
    EFI_FILE_INFO *fi=(EFI_FILE_INFO*)&info;
    if(fi->Attribute&EFI_FILE_DIRECTORY){f->Close(f);return 3;}
    if(fi->FileSize>cap){f->Close(f);return 4;}
    UINTN want=(UINTN)fi->FileSize,n=want;s=f->Read(f,&n,buffer);f->Close(f);
    if(EFI_ERROR(s)||n!=want)return 3;
    *length=n;return 0;
}
int platform_write(const char *name,const unsigned char *buffer,size_t length){
    if(!basic_dir||!valid_filename(name))return 3;
    CHAR16 path[65];filename16(name,path);EFI_FILE_HANDLE f;
    EFI_STATUS s=basic_dir->Open(basic_dir,&f,path,EFI_FILE_MODE_READ,0);
    if(!EFI_ERROR(s)){f->Close(f);return 2;}
    if(s!=EFI_NOT_FOUND)return 3;
    s=basic_dir->Open(basic_dir,&f,path,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0);
    if(EFI_ERROR(s))return 3;
    UINTN n=length;s=f->Write(f,&n,(VOID*)buffer);
    if(EFI_ERROR(s)||n!=length){f->Delete(f);return 3;}
    s=f->Flush(f);EFI_STATUS close_status=f->Close(f);
    return EFI_ERROR(s)||EFI_ERROR(close_status)?3:0;
}
void platform_directory(void){
    if(!basic_dir){screen_text("?BASIC FOLDER UNAVAILABLE\r");return;}
    screen_text("FILES IN BASIC:\r");basic_dir->SetPosition(basic_dir,0);
    union{UINT64 align;unsigned char bytes[1024];}data;
    for(;;){UINTN n=sizeof(data);EFI_STATUS s=basic_dir->Read(basic_dir,&n,&data);
        if(EFI_ERROR(s)){screen_text("?DIRECTORY ERROR\r");break;}if(!n)break;
        EFI_FILE_INFO *fi=(EFI_FILE_INFO*)&data;if(fi->Attribute&EFI_FILE_DIRECTORY)continue;
        char name[65];unsigned i=0;
        while(i<64&&fi->FileName[i]&&fi->FileName[i]<128){name[i]=(char)fi->FileName[i];++i;}
        if(fi->FileName[i])continue;
        name[i]=0;if(!valid_filename(name))continue;
        screen_text("  ");screen_text(name);screen_text("\r");
    }
}
EFI_STATUS efi_main(EFI_HANDLE image,EFI_SYSTEM_TABLE *system){
    InitializeLib(image,system);st=system;bs=system->BootServices;
    bs->SetWatchdogTimer(0,0,0,NULL);
    INT32 original_mode=st->ConOut->Mode->Mode,selected=-1;
    for(INT32 i=0;i<st->ConOut->Mode->MaxMode;++i){UINTN cols,rows;
        if(!EFI_ERROR(st->ConOut->QueryMode(st->ConOut,(UINTN)i,&cols,&rows))&&cols>=80&&rows>=25){
            if(selected<0)selected=i;
            if(cols==80&&rows==25){selected=i;break;}
        }
    }
    if(selected<0){Print(L"BASIC EFI requires an 80x25 text console.\r\n");return EFI_UNSUPPORTED;}
    st->ConOut->SetMode(st->ConOut,(UINTN)selected);
    st->ConOut->SetAttribute(st->ConOut,EFI_BACKGROUND_BLUE|EFI_WHITE);st->ConOut->ClearScreen(st->ConOut);
    EFI_LOADED_IMAGE *loaded=NULL;EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;EFI_FILE_HANDLE root=NULL;
    if(!EFI_ERROR(bs->HandleProtocol(image,&LoadedImageProtocol,(VOID**)&loaded))&&
       !EFI_ERROR(bs->HandleProtocol(loaded->DeviceHandle,&FileSystemProtocol,(VOID**)&fs))&&
       !EFI_ERROR(fs->OpenVolume(fs,&root))){
        /* Only the image's own volume, existing BASIC directory, no device enumeration. */
        root->Open(root,&basic_dir,L"BASIC",EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,EFI_FILE_DIRECTORY);
        root->Close(root);
    }
    basic_run();if(basic_dir)basic_dir->Close(basic_dir);
    st->ConOut->SetMode(st->ConOut,(UINTN)original_mode);
    st->ConOut->SetAttribute(st->ConOut,EFI_LIGHTGRAY);st->ConOut->ClearScreen(st->ConOut);
    return EFI_SUCCESS;
}
