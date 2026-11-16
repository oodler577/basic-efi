#ifndef BASIC_EFI_H
#define BASIC_EFI_H
#include <stdint.h>
#include <stddef.h>
#include "charset.h"
#define COLS 80
#define ROWS 24
#define LINE_MAXIMUM 240
#define FILE_CAPACITY 131072
enum { KEY_UP=256, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_HOME, KEY_END,
       KEY_DELETE, KEY_INSERT, KEY_ESCAPE, KEY_RESET, KEY_EXIT };
typedef struct {
    unsigned char cells[ROWS][COLS];
    uint8_t color[ROWS][COLS];
    unsigned char continued[ROWS];
    int x,y,dirty;
    unsigned scrolls;
    unsigned charset;
    unsigned char ink,reverse,verbatim;
} Screen;
extern Screen screen;
extern int exiting;
void screen_clear(void);
void screen_charset(unsigned set);
void screen_put(int c);
void screen_text(const char *s);
int screen_edit(char *line, size_t capacity);
void screen_set_cell(unsigned offset, unsigned char value);
void screen_set_color(unsigned offset, unsigned char value);
void platform_draw(void);
/* 1: blocking editor input; 0: poll break/reset, retain one key; -1: GET. */
int platform_key(int wait);
int platform_read(const char *name, unsigned char *buffer, size_t capacity, size_t *length);
int platform_write(const char *name, const unsigned char *buffer, size_t length);
void platform_directory(void);
void platform_trace(int c);
void platform_idle(void);
void platform_info(void);
int basic_run(void);
int valid_filename(const char *name);
/* Storage return codes: 0 success, 1 missing, 2 exists, 3 I/O, 4 too large. */
#endif
