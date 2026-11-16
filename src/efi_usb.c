#include "efi_usb.h"
#include "basic.h"
#include <stddef.h>
#include <string.h>
_Static_assert(sizeof(BasicUsbRequest)==8,"USB setup packet ABI");
_Static_assert(offsetof(BasicUsbIo,device)==6*sizeof(BasicUsbUnused),"USB IO device method ABI");
_Static_assert(offsetof(BasicUsbIo,endpoint)==9*sizeof(BasicUsbUnused),"USB IO endpoint method ABI");
static EFI_BOOT_SERVICES *services;
static EFI_GUID usb_guid=BASIC_USB_GUID;
static EFI_HANDLE adapter;
static BasicUsbIo *selected;
void efi_usb_init(EFI_BOOT_SERVICES *bs){services=bs;}
static unsigned u16(const unsigned char *b){return (unsigned)b[0]+256u*b[1];}
static int matches(BasicUsbIo *io){
    if(!io || !io->control || !io->bulk || !io->device || !io->configuration ||
       !io->interface || !io->endpoint)return 0;
    unsigned char d[18]={0},c[9]={0},f[9]={0},ep[7];
    if(EFI_ERROR(io->device(io,d)) || d[0]!=18 || d[1]!=1 ||
       u16(d+8)!=0x16d0 || u16(d+10)!=0x0504)return 0;
    if(EFI_ERROR(io->configuration(io,c)) || c[0]!=9 || c[1]!=2 || c[5]!=1)return -1;
    if(EFI_ERROR(io->interface(io,f)) || f[0]!=9 || f[1]!=4 || f[2] || f[3] ||
       f[4]!=2 || f[5]!=0xff || f[6] || f[7])return -1;
    unsigned endpoints=0;
    for(unsigned i=0;i<2;++i){
        memset(ep,0,sizeof(ep));
        if(EFI_ERROR(io->endpoint(io,(UINT8)i,ep)) || ep[0]!=7 || ep[1]!=5 ||
           (ep[3]&3)!=2 || u16(ep+4)!=64)return -1;
        unsigned bit=ep[2]==IEC_IN?1:ep[2]==IEC_OUT?2:0;
        if(!bit || (endpoints&bit))return -1;
        endpoints|=bit;
    }
    return endpoints==3?1:-1;
}
static BasicUsbIo *current(void){
    /* Re-resolve on every call: hot-unplug can destroy a protocol instance. */
    BasicUsbIo *io=NULL;
    if(!adapter || EFI_ERROR(services->HandleProtocol(adapter,&usb_guid,(VOID**)&io)) ||
       io!=selected)return NULL;
    return io;
}
static int result(EFI_STATUS s,UINT32 status){
    if(s==EFI_TIMEOUT || (status&0x40))return IEC_TIMEOUT;
    return EFI_ERROR(s)||status?IEC_USB:IEC_OK;
}
static int usb_control(void *ctx,unsigned type,unsigned request,unsigned value,unsigned index,
                       unsigned char *data,size_t length,unsigned timeout){
    (void)ctx;BasicUsbIo *io=current();if(!io)return IEC_USB;
    BasicUsbRequest packet={(UINT8)type,(UINT8)request,(UINT16)value,(UINT16)index,(UINT16)length};
    UINT32 status=0;
    EFI_STATUS s=io->control(io,&packet,length?(type&128?0:1):2,timeout,data,(UINTN)length,&status);
    return result(s,status);
}
static int usb_bulk(void *ctx,unsigned ep,unsigned char *data,size_t *length,unsigned timeout){
    (void)ctx;BasicUsbIo *io=current();if(!io)return IEC_USB;
    UINTN n=(UINTN)*length;UINT32 status=0;
    EFI_STATUS s=io->bulk(io,(UINT8)ep,data,&n,(UINTN)timeout,&status);
    *length=(size_t)n;return result(s,status);
}
static int cancelled(void *ctx){
    (void)ctx;platform_draw();int k=platform_key(0);
    return k==KEY_ESCAPE || k==KEY_RESET || k==KEY_EXIT;
}
static void pause_ms(void *ctx,unsigned ms){(void)ctx;services->Stall((UINTN)ms*1000);}
int platform_iec_transport(IecTransport *transport){
    EFI_HANDLE *handles=NULL;UINTN count=0;adapter=NULL;selected=NULL;
    EFI_STATUS s=services->LocateHandleBuffer(ByProtocol,&usb_guid,NULL,&count,&handles);
    if(EFI_ERROR(s))return s==EFI_NOT_FOUND?IEC_NO_ADAPTER:IEC_USB;
    unsigned matches_count=0;int unsupported=0;
    for(UINTN i=0;i<count;++i){
        BasicUsbIo *io=NULL;
        if(EFI_ERROR(services->HandleProtocol(handles[i],&usb_guid,(VOID**)&io)))continue;
        int found=matches(io);if(found<0){unsupported=1;continue;}
        if(found){++matches_count;adapter=handles[i];selected=io;}
    }
    services->FreePool(handles);
    if(matches_count!=1 || unsupported){adapter=NULL;selected=NULL;
        return matches_count>1?IEC_MULTIPLE:unsupported?IEC_UNSUPPORTED:IEC_NO_ADAPTER;
    }
    *transport=(IecTransport){NULL,usb_control,usb_bulk,cancelled,pause_ms};return IEC_OK;
}
