#ifndef BASIC_IEC_H
#define BASIC_IEC_H
#include <stddef.h>
#include <stdint.h>

/* Independently implemented xum1541 wire protocol; see docs/IEC-PROTOCOL.md. */
enum {
    IEC_OK, IEC_NO_ADAPTER, IEC_MULTIPLE, IEC_UNSUPPORTED, IEC_USB,
    IEC_TIMEOUT, IEC_CANCELLED, IEC_PROTOCOL, IEC_DRIVE, IEC_TOO_LARGE,
    IEC_BAD_NAME, IEC_OFFLINE, IEC_BAD_DIRECTORY
};
enum { IEC_IN=0x83, IEC_OUT=0x04, IEC_CHUNK=128, IEC_LIMIT=131072 };
typedef struct {
    void *context;
    int (*control)(void *,unsigned type,unsigned request,unsigned value,unsigned index,
                   unsigned char *data,size_t length,unsigned timeout_ms);
    int (*bulk)(void *,unsigned endpoint,unsigned char *data,size_t *length,unsigned timeout_ms);
    int (*cancelled)(void *);
    void (*pause)(void *,unsigned milliseconds);
} IecTransport;
typedef struct {
    IecTransport io;
    unsigned version;
    int connected;
    char drive_status[96];
} Iec;
typedef void (*IecDirectoryLine)(void *,unsigned blocks,const unsigned char *text,size_t length);
int iec_connect(Iec *,const IecTransport *);
void iec_disconnect(Iec *);
int iec_status(Iec *,unsigned device,unsigned *code);
int iec_read_file(Iec *,unsigned device,const char *,unsigned char *,size_t,size_t *);
int iec_read_program(Iec *,unsigned device,const char *,unsigned char *,size_t,size_t *);
int iec_read_directory(Iec *,unsigned device,unsigned char *,size_t,size_t *);
int iec_parse_directory(const unsigned char *,size_t,IecDirectoryLine,void *);
int iec_valid_name(const char *);
const char *iec_error(int);

/* The monitor retains selection after errors. Only IEC OFF restores FAT. */
int storage_iec_selected(void);
unsigned storage_iec_device(void);
int storage_iec_command(const char *);
int storage_iec_read(const char *,int prg,unsigned char *,size_t,size_t *);
void storage_iec_directory(unsigned char *,size_t);
void storage_iec_close(void);
void storage_iec_info(void);
int platform_iec_transport(IecTransport *);
#endif
