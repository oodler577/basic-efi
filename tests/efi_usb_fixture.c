/* Installed only in BOOTIEC.EFI. Exercises the real UEFI transport against a
   synthetic USB_IO protocol while QEMU boots the actual interpreter. */
#include "efi_usb.h"
#include "basic.h"
#include "mock_xum.h"
#include <string.h>
static EFI_BOOT_SERVICES *bs;
static EFI_GUID guid=BASIC_USB_GUID;
static EFI_HANDLE handle,second_handle;
static MockXum peer;
static unsigned invalid_descriptor;
static void trace(const char *s){while(*s)platform_trace(*s++);}
static EFI_STATUS EFIAPI control(BasicUsbIo *io,BasicUsbRequest *r,UINT32 direction,UINT32 timeout,
                                  VOID *data,UINTN length,UINT32 *status){
    (void)io;*status=0;
    if(r->length!=length || direction!=(length?(r->type&128?0u:1u):2u))return EFI_INVALID_PARAMETER;
    return mock_xum_control(&peer,r->type,r->request,r->value,r->index,data,length,timeout)?EFI_DEVICE_ERROR:EFI_SUCCESS;
}
static EFI_STATUS EFIAPI bulk(BasicUsbIo *io,UINT8 ep,VOID *data,UINTN *length,UINTN timeout,UINT32 *status){
    (void)io;*status=0;
    if(ep==IEC_IN && peer.pending==3 && peer.stream==peer.file && !strcmp(peer.filename,"0:TIMEOUT,S,R")){
        peer.state=1;return EFI_TIMEOUT;
    }
    if(ep==IEC_IN && peer.pending==3 && peer.stream==peer.file && !strcmp(peer.filename,"0:BAD,S,R")){
        static const unsigned char bad[]={1,8,0,0,10,0,0x99,'1',0,0,0};
        peer.stream=bad;peer.stream_length=sizeof(bad);
    }
    size_t n=*length;int e=mock_xum_bulk(&peer,ep,data,&n,(unsigned)timeout);*length=n;
    return e?EFI_DEVICE_ERROR:EFI_SUCCESS;
}
static EFI_STATUS EFIAPI device(BasicUsbIo *io,VOID *out){
    (void)io;unsigned char bytes[18]={18,1,0,2,0,0,0,64,0xd0,0x16,4,5,0,1,0,0,0,1};
    if(invalid_descriptor==1)bytes[8]=0;
    memcpy(out,bytes,sizeof(bytes));return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI configuration(BasicUsbIo *io,VOID *out){
    (void)io;const unsigned char bytes[9]={9,2,32,0,1,1,0,0x80,50};
    memcpy(out,bytes,sizeof(bytes));return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI interface(BasicUsbIo *io,VOID *out){
    (void)io;unsigned char bytes[9]={9,4,0,0,2,0xff,0,0,0};
    if(invalid_descriptor==2)bytes[3]=1;
    memcpy(out,bytes,sizeof(bytes));return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI endpoint(BasicUsbIo *io,UINT8 index,VOID *out){
    (void)io;if(index>1)return EFI_NOT_FOUND;
    unsigned char bytes[7]={7,5,index?IEC_OUT:IEC_IN,2,64,0,0};
    if(invalid_descriptor==3)bytes[3]=3;
    if(invalid_descriptor==4)bytes[4]=32;
    if(invalid_descriptor==5)bytes[2]=IEC_IN;
    memcpy(out,bytes,sizeof(bytes));return EFI_SUCCESS;
}
static BasicUsbIo usb={.control=control,.bulk=bulk,.device=device,.configuration=configuration,
                       .interface=interface,.endpoint=endpoint};
void iec_fixture_uninstall(void){
    bs->UninstallProtocolInterface(handle,&guid,&usb);
    trace(peer.bad?"IEC FIXTURE: INVALID TRANSACTION\n":"IEC FIXTURE: CLEAN EXIT\n");
}
int iec_fixture_install(EFI_BOOT_SERVICES *services){
    bs=services;mock_xum_init(&peer);IecTransport transport;
    if(EFI_ERROR(bs->InstallProtocolInterface(&handle,&guid,EFI_NATIVE_INTERFACE,&usb)))return 0;
    for(invalid_descriptor=1;invalid_descriptor<=5;++invalid_descriptor){
        int expected=invalid_descriptor==1?IEC_NO_ADAPTER:IEC_UNSUPPORTED;
        if(platform_iec_transport(&transport)!=expected)return 0;
    }
    invalid_descriptor=0;
    if(EFI_ERROR(bs->InstallProtocolInterface(&second_handle,&guid,EFI_NATIVE_INTERFACE,&usb)))return 0;
    if(platform_iec_transport(&transport)!=IEC_MULTIPLE)return 0;
    if(EFI_ERROR(bs->UninstallProtocolInterface(second_handle,&guid,&usb)))return 0;
    if(platform_iec_transport(&transport)!=IEC_OK)return 0;
    if(EFI_ERROR(bs->UninstallProtocolInterface(handle,&guid,&usb)))return 0;
    unsigned char data[8];
    if(transport.control(transport.context,0xa0,1,0,0,data,8,1000)!=IEC_USB)return 0;
    handle=NULL;
    if(EFI_ERROR(bs->InstallProtocolInterface(&handle,&guid,EFI_NATIVE_INTERFACE,&usb)))return 0;
    trace("IEC FIXTURE: DESCRIPTORS, MULTIPLE ADAPTERS, HOT-UNPLUG PASS\n");return 1;
}
