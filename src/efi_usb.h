#ifndef BASIC_EFI_USB_H
#define BASIC_EFI_USB_H
#include <efi.h>
#include "iec.h"
/* Minimal ABI declarations from UEFI 2.11 section 17.2. Unused method slots
   are retained; this is not a USB host-controller driver. */
typedef struct BasicUsbIo BasicUsbIo;
typedef struct {UINT8 type,request;UINT16 value,index,length;} BasicUsbRequest;
typedef VOID (EFIAPI *BasicUsbUnused)(VOID);
struct BasicUsbIo {
    EFI_STATUS (EFIAPI *control)(BasicUsbIo *,BasicUsbRequest *,UINT32,UINT32,VOID *,UINTN,UINT32 *);
    EFI_STATUS (EFIAPI *bulk)(BasicUsbIo *,UINT8,VOID *,UINTN *,UINTN,UINT32 *);
    BasicUsbUnused interrupts_and_isochronous[4];
    EFI_STATUS (EFIAPI *device)(BasicUsbIo *,VOID *);
    EFI_STATUS (EFIAPI *configuration)(BasicUsbIo *,VOID *);
    EFI_STATUS (EFIAPI *interface)(BasicUsbIo *,VOID *);
    EFI_STATUS (EFIAPI *endpoint)(BasicUsbIo *,UINT8,VOID *);
    BasicUsbUnused strings_languages_reset[3];
};
#define BASIC_USB_GUID {0x2b2f68d6,0x0cd2,0x44cf,{0x8e,0x8b,0xbb,0xa2,0x0b,0x1b,0x5b,0x75}}
void efi_usb_init(EFI_BOOT_SERVICES *);
#endif
