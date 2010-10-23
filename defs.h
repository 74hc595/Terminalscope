/* Terminalscope for AVR
 * Matt Sarnoff (www.msarnoff.org)
 * Released under the "do whatever you want with it, but let me know if you've
 * used it for something awesome and give me credit" license.
 *
 * defs.h - global definitions 
 */

#ifndef _DEFS_H_
#define _DEFS_H_

#define VERSION_STRING "v0.6 Feb 13 2010"

#define set_bit(v,b)        v |= _BV(b)
#define clear_bit(v,b)      v &= ~_BV(b)
#define pulse_bit(v,b)      do { set_bit(v,b); clear_bit(v,b); } while(0)
#define pulse_bit_low(v,b)  do { clear_bit(v,b); set_bit(v,b); } while(0)

/* binary logarithm for compile-time constants */
#define LOG2(v)       (8 - 90/(((v)/4+14)|1) - 2/((v)/2+1))

#define PASTE(x,y)    x ## y
#define PORT(x)       PASTE(PORT,x)
#define PIN(x)        PASTE(PIN,x)
#define DDR(x)        PASTE(DDR,x)

#define VIDEO         B
#define VIDEO_OUT_PIN 0
#define VIDEO_MASK    0b00000001

#define SYNC          C
#define HSYNC_PIN     1
#define VSYNC_PIN     0
#define SYNC_MASK     0b00000011

#define TILE_WIDTH    6   /* must be 8 or less! */
#define TILE_HEIGHT   8   /* must be a power of two! */
#define TILE_HBIT     LOG2(TILE_HEIGHT)
#define TILES_WIDE    54
#define TILES_HIGH    24
#define NUM_TILES     (TILES_WIDE*TILES_HIGH)
#define PIXELS_WIDE   (TILE_WIDTH*TILES_WIDE)
#define PIXELS_HIGH   (TILE_HEIGHT*TILES_HIGH)
#define NUM_LINES     PIXELS_HIGH


#define FONT_6x8

#endif
