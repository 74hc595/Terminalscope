/* Terminalscope for AVR
 * Matt Sarnoff (www.msarnoff.org)
 * Released under the "do whatever you want with it, but let me know if you've
 * used it for something awesome and give me credit" license.
 *
 * video.c - output and cursor handling routines
 */

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "video.h"
#include "defs.h"

/* an extra row is allocated to mitigate the effects of stupidly writing
 * beyond the end of the screen (try to make this not happen) */
char TILEMAP[TILES_HIGH+1][TILES_WIDE];
static int8_t cx;
static int8_t cy;
static uint8_t showcursor;

/* Vertical margins */
static int8_t mtop;
static int8_t mbottom;

/* reverse video */
static uint8_t revvideo;

static void CURSOR_INVERT() __attribute__((noinline));
static void CURSOR_INVERT()
{
  TILEMAP[cy][cx] ^= showcursor;
}

void video_welcome()
{
  video_clrscr();

  /* Print border */
  uint8_t i;
  for (i = 1; i < TILES_WIDE-1; i++)
  {
    video_putcxy(i, 0, '\x12');
    video_putcxy(i, 2, '\x12');
  }
  video_putcxy(0, 1, '\x19');
  video_putcxy(TILES_WIDE-1, 1, '\x19');
  video_putcxy(0, 0, '\x0D');
  video_putcxy(TILES_WIDE-1, 0, '\x0C');
  video_putcxy(0, 2, '\x0E');
  video_putcxy(TILES_WIDE-1, 2, '\x0B');
  video_putsxy_P(2,1, PSTR("Terminalscope by Matt Sarnoff"));
  video_putsxy_P(TILES_WIDE-18, 1, PSTR(VERSION_STRING));

  cx = 0;
  cy = 3;

  showcursor = 0;
}

void video_setup()
{
  revvideo = 0;
  
  /*video_welcome();*/
  
  DDR(VIDEO) |= VIDEO_MASK;
  DDR(SYNC)  |= SYNC_MASK;

  TCCR1A = 0;
  TCCR1B = _BV(WGM12); /* CTC mode */
  OCR1A = 326;         /* 20000000 / (2*1024*(1+162)) = 59.91 Hz */

}

void video_start()
{
  /* start frame timer */
  TCCR1B |= _BV(CS12) | _BV(CS10); /* 1/1024 prescaler */
}

void video_wait()
{
  /* wait for compare match */
  loop_until_bit_is_set(TIFR1, OCF1A);
  set_bit(TIFR1, OCF1A);
}

/* video_output_frame is in video-asm.S */


/****** Output routines ******/

void video_reset_margins()
{
  video_set_margins(0, TILES_HIGH-1);
}

void video_set_margins(int8_t top, int8_t bottom)
{
  /* sanitize input */
  if (top < 0) top = 0;
  if (bottom >= TILES_HIGH) bottom = TILES_HIGH-1;
  if (top >= bottom) { top = 0; bottom = TILES_HIGH-1; }

  mtop = top;
  mbottom = bottom;
  video_gotoxy(mtop, 0);
}

int8_t video_top_margin()
{
  return mtop;
}

int8_t video_bottom_margin()
{
  return mbottom;
}

void video_set_reverse(uint8_t val)
{
  revvideo = (val) ? 0x80 : 0;
}

static void _video_scrollup()
{
  memmove(&TILEMAP[mtop], &TILEMAP[mtop+1], (mbottom-mtop)*TILES_WIDE);
  memset(&TILEMAP[mbottom], revvideo, TILES_WIDE);
}

static void _video_scrolldown()
{
  memmove(&TILEMAP[mtop+1], &TILEMAP[mtop], (mbottom-mtop)*TILES_WIDE);
  memset(&TILEMAP[mtop], revvideo, TILES_WIDE);
}

void video_scrollup()
{
  CURSOR_INVERT();
  _video_scrollup();
  CURSOR_INVERT();
}

void video_scrolldown()
{
  CURSOR_INVERT();
  _video_scrolldown();
  CURSOR_INVERT();
}

void video_movesol()
{
  CURSOR_INVERT();
  cx = 0;
  CURSOR_INVERT();
}

void video_setx(int8_t x)
{
  CURSOR_INVERT();
  cx = x;
  if (cx < 0) cx = 0;
  if (cx >= TILES_WIDE) cx = TILES_WIDE-1;
  CURSOR_INVERT();
}

/* Absolute positioning does not respect top/bottom margins */
void video_gotoxy(int8_t x, int8_t y)
{
  CURSOR_INVERT();
  cx = x;
  if (cx < 0) cx = 0;
  if (cx >= TILES_WIDE) cx = TILES_WIDE-1;
  cy = y;
  if (cy < 0) cy = 0;
  if (cy >= TILES_HIGH) cy = TILES_HIGH-1;
  CURSOR_INVERT();
}

void video_movex(int8_t dx)
{
  CURSOR_INVERT();
  cx += dx;
  if (cx < 0) cx = 0;
  if (cx >= TILES_WIDE) cx = TILES_WIDE-1;
  CURSOR_INVERT();
}

void video_movey(int8_t dy)
{
  CURSOR_INVERT();
  cy += dy;
  if (cy < mtop) cy = mtop;
  if (cy > mbottom) cy = mbottom;
  CURSOR_INVERT();
}

static void _video_lfwd()
{
  cx = 0;
  if (++cy > mbottom)
  {
    cy = mbottom;
    _video_scrollup();
  }
}

static inline void _video_cfwd()
{
  if (++cx > TILES_WIDE)
    _video_lfwd();
}

void video_cfwd()
{
  CURSOR_INVERT();
  _video_cfwd();
  CURSOR_INVERT();
}

void video_lfwd()
{
  CURSOR_INVERT();
  cx = 0;
  if (++cy > mbottom)
  {
    cy = mbottom;
    _video_scrollup();
  }
  CURSOR_INVERT();
}

void video_lf()
{
  CURSOR_INVERT();
  if (++cy > mbottom)
  {
    cy = mbottom;
    _video_scrollup();
  }
  CURSOR_INVERT();
}

static void _video_lback()
{
  cx = TILES_WIDE-1;
  if (--cy < 0)
  { cx = 0; cy = mtop; }
}

void video_lback()
{
  CURSOR_INVERT();
  cx = TILES_WIDE-1;
  if (--cy < 0)
  { cx = 0; cy = mtop; }
  CURSOR_INVERT();
}

void video_cback()
{
  CURSOR_INVERT();
  if (--cx < 0)
    _video_lback();
  CURSOR_INVERT();
}

int8_t video_getx()
{
  return cx;
}

int8_t video_gety()
{
  return cy;
}

char video_charat(int8_t x, int8_t y)
{
  return TILEMAP[cy][cx];
}

void video_clrscr()
{
  CURSOR_INVERT();
  video_reset_margins(); 
  memset(TILEMAP, revvideo, TILES_WIDE*TILES_HIGH);
  cx = cy = 0;
  CURSOR_INVERT();
}

void video_clrline()
{
  CURSOR_INVERT();
  memset(&TILEMAP[cy], revvideo, TILES_WIDE);
  cx = 0;
  CURSOR_INVERT();
}

void video_clreol()
{
  memset(&TILEMAP[cy][cx], revvideo, TILES_WIDE-cx);
}

void video_erase(uint8_t erasemode)
{
  CURSOR_INVERT();
  switch(erasemode)
  {
    case 0: /* erase from cursor to end of screen */
      memset(&TILEMAP[cy][cx], revvideo,
          (TILES_WIDE*TILES_HIGH)-(cy*TILES_WIDE+cx));
      break;
    case 1: /* erase from beginning of screen to cursor */
      memset(TILEMAP, revvideo, cy*TILES_WIDE+cx+1);
      break;
    case 2: /* erase entire screen */
      memset(TILEMAP, revvideo, TILES_WIDE*TILES_HIGH);
      break;
  }
  CURSOR_INVERT();
}

void video_eraseline(uint8_t erasemode)
{
  CURSOR_INVERT();
  switch(erasemode)
  {
    case 0: /* erase from cursor to end of line */
      memset(&TILEMAP[cy][cx], revvideo, TILES_WIDE-cx);
      break;
    case 1: /* erase from beginning of line to cursor */
      memset(&TILEMAP[cy], revvideo, cx+1);
      break;
    case 2: /* erase entire line */
      memset(&TILEMAP[cy], revvideo, TILES_WIDE);
      break;
  }
  CURSOR_INVERT();
}

/* Does not respect top/bottom margins */
void video_putcxy(int8_t x, int8_t y, char c)
{
  if (x < 0 || x >= TILES_WIDE) return;
  if (y < 0 || y >= TILES_HIGH) return;
  TILEMAP[y][x] = c ^ revvideo;
}

/* Does not respect top/bottom margins */
void video_putsxy(int8_t x, int8_t y, char *str)
{
  if (x < 0 || x >= TILES_WIDE) return;
  if (y < 0 || y >= TILES_HIGH) return;
  int len = strlen(str);
  if (len > TILES_WIDE-x) len = TILES_WIDE-x;
  memcpy((char *)(&TILEMAP[y][x]), str, len);
  if (revvideo) video_invert_range(x, y, len);
}

/* Does not respect top/bottom margins */
void video_putsxy_P(int8_t x, int8_t y, PGM_P str)
{
  if (x < 0 || x >= TILES_WIDE) return;
  if (y < 0 || y >= TILES_HIGH) return;
  int len = strlen_P(str);
  if (len > TILES_WIDE-x) len = TILES_WIDE-x;
  memcpy_P((char *)(&TILEMAP[y][x]), str, len);
  if (revvideo) video_invert_range(x, y, len);
}

/* Does not respect top/bottom margins */
void video_putline(int8_t y, char *str)
{
  if (y < 0 || y >= TILES_HIGH) return;
  /* strncpy fills unused bytes in the destination with nulls */
  strncpy((char *)(&TILEMAP[y]), str, TILES_WIDE);
  if (revvideo) video_invert_range(0, y, TILES_WIDE);
}

/* Does not respect top/bottom margins */
void video_putline_P(int8_t y, PGM_P str)
{
  if (y < 0 || y >= TILES_HIGH) return;
  /* strncpy fills unused bytes in the destination with nulls */
  strncpy_P((char *)(&TILEMAP[y]), str, TILES_WIDE);
  if (revvideo) video_invert_range(0, y, TILES_WIDE);
}

void video_setc(char c)
{
  CURSOR_INVERT();
  TILEMAP[cy][cx] = c ^ revvideo;
  CURSOR_INVERT();
}

static inline void _video_putc(char c)
{
  /* If the last character printed exceeded the right boundary,
   * we have to go to a new line. */
  if (cx >= TILES_WIDE) _video_lfwd();

  if (c == '\r') cx = 0;
  else if (c == '\n') _video_lfwd();
  else
  {
    TILEMAP[cy][cx] = c ^ revvideo;
    _video_cfwd();
  }
}

void video_putc(char c)
{
  CURSOR_INVERT();
  _video_putc(c);
  CURSOR_INVERT();
}

void video_putc_raw(char c)
{
  CURSOR_INVERT();
  
  /* If the last character printed exceeded the right boundary,
   * we have to go to a new line. */
  if (cx >= TILES_WIDE) _video_lfwd();
  
  TILEMAP[cy][cx] = c ^ revvideo;
  _video_cfwd();
  CURSOR_INVERT();
}

void video_puts(char *str)
{
  /* Characters are interpreted and printed one at a time. */
  char c;
  CURSOR_INVERT();
  while ((c = *str++))
    _video_putc(c);
  CURSOR_INVERT();
}

void video_puts_P(PGM_P str)
{
  char c;
  CURSOR_INVERT();
  while ((c = pgm_read_byte(str++)))
    _video_putc(c);
  CURSOR_INVERT();
}

void video_show_cursor()
{
  if (!showcursor)
  {
    showcursor = 0x80;
    CURSOR_INVERT();
  }
}

void video_hide_cursor()
{
  if (showcursor)
  {
    CURSOR_INVERT();
    showcursor = 0;
  }
}

uint8_t video_cursor_visible()
{
  return showcursor != 0;
}

void video_invert_range(int8_t x, int8_t y, uint8_t rangelen)
{
  char *start = &TILEMAP[y][x];
  uint8_t i;
  for (i = 0; i < rangelen; i++)
  {
    *start ^= 0x80;
    start++;
  }
}

