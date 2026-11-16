#ifndef BASIC_PRG_H
#define BASIC_PRG_H
#include <stddef.h>
#include <stdint.h>
#define PRG_CAPACITY 30000
#define PRG_LINE_MAX 240
enum {PRG_OK,PRG_ADDRESS,PRG_STRUCTURE,PRG_LINK,PRG_LINE_NUMBER,
      PRG_LINE_LENGTH,PRG_TOO_LARGE,PRG_UNSUPPORTED,PRG_TRAILING};
typedef struct {int code;unsigned line,token;} PrgError; /* line=65536 if unavailable */
/* Maps are built by keyword name from this build's actual ROM token table. */
void prg_maps(const char native[128][16],uint8_t from_c64[256],uint8_t to_c64[256]);
/* Input/output buffers must not overlap. On failure *length=0 and scratch
   output is unspecified; callers must not install or write it. */
int prg_import(const uint8_t *,size_t,const uint8_t map[256],uint8_t *,size_t,size_t *,PrgError *);
int prg_export(const uint8_t *,size_t,unsigned base,const uint8_t map[256],uint8_t *,size_t,size_t *,PrgError *);
const char *prg_error(int);
const char *prg_keyword(unsigned);
int prg_filename(const char *);
#endif
