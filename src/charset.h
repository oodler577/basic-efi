#ifndef BASIC_CHARSET_H
#define BASIC_CHARSET_H
#include <stddef.h>
#include <stdint.h>
enum { CHARSET_ISO, CHARSET_PET, CHARSET_PET_LOWER, CHARSET_COUNT };
unsigned char charset_screen(unsigned set, unsigned char byte);
unsigned char charset_byte(unsigned set, unsigned char cell);
unsigned char charset_row(unsigned set, unsigned char cell, unsigned row);
uint32_t charset_unicode(unsigned set, unsigned char cell);
int charset_input(unsigned set, uint32_t unicode);
const char *charset_name(unsigned set);
/* Firmware GOP blit layout, deliberately independent of framebuffer format. */
typedef struct { uint8_t blue,green,red,reserved; } TextPixel;
#define TEXT_MAX_SCALE 4
#define TEXT_ROW_PIXELS (80*8*8*TEXT_MAX_SCALE*TEXT_MAX_SCALE)
extern const TextPixel text_palette[16];
unsigned text_scale(unsigned width, unsigned height);
int text_render_row(TextPixel *pixels, size_t capacity, const unsigned char *cells,
                    const unsigned char *colors, unsigned count, unsigned set,
                    unsigned scale, int cursor);
#endif
