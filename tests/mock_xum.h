#ifndef TEST_MOCK_XUM_H
#define TEST_MOCK_XUM_H
#include "iec.h"
/* Test-only USB peer; no host file access. It interprets packets independently
   and serves a directory, status channel and numbered SEQ source. */
typedef struct {
    unsigned version,capabilities,state,initialized,shutdowns,clear_halts;
    unsigned calls,cancel_calls,cancel_at,fail_at,short_at,pauses,bad;
    unsigned pending,mode,want,value,channel,talking,eoi,closed,opens,reads;
    unsigned status_mode,short_read,wrong_count;
    const unsigned char *stream,*file,*directory,*prg;
    size_t stream_length,position,file_length,directory_length,prg_length;
    char filename[24];
    const char *dos_status;
} MockXum;
void mock_xum_init(MockXum *);
IecTransport mock_xum_transport(MockXum *);
int mock_xum_control(void *,unsigned,unsigned,unsigned,unsigned,unsigned char *,size_t,unsigned);
int mock_xum_bulk(void *,unsigned,unsigned char *,size_t *,unsigned);
#endif
