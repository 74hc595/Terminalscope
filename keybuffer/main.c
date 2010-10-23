/* PS/2 keyboard buffer and decoder
 * Matt Sarnoff (www.msarnoff.org)
 * Released under the "do whatever you want with it, but let me know if you've
 * used it for something awesome and give me credit" license.
 *
 * Reads scancodes from a PS/2 keyboard, decodes them to ASCII values
 * (or values greater than 0x80 for special keys) and stores them in a buffer.
 * A host microcontroller can then read the keypresses synchronously via SPI.
 *
 * Pins:
 *   PB3 (pin 2) to PS/2 data line
 *   PB4 (pin 3) to PS/2 clock line
 *   PB2 (pin 7) to SPI SCK line
 *   PB1 (pin 6) to SPI MISO line
 *
 * To read a keypress, send any byte on the SPI bus.
 * The first keycode in the queue will be dequeued and sent to the master.
 * If there are no keycodes in the buffer, 0 will be sent to the master.
 * 
 * Caps lock and the keyboard LEDs are not supported.
 * (Sorry, internet forum posters.)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>

#include "keycodes.h"

#define ESC K_ESC
#define CLK K_CAPSLK
#define NLK K_NUMLK
#define SLK K_SCRLK
#define F1  K_F1
#define F2  K_F2
#define F3  K_F3
#define F4  K_F4
#define F5  K_F5
#define F6  K_F6
#define F7  K_F7
#define F8  K_F8
#define F9  K_F9
#define F10 K_F10
#define F11 K_F11
#define F12 K_F12
#define INS K_INS
#define DEL K_DEL
#define HOM K_HOME
#define END K_END
#define PGU K_PGUP
#define PGD K_PGDN
#define ARL K_LEFT
#define ARR K_RIGHT
#define ARU K_UP
#define ARD K_DOWN
#define PRS K_PRTSC
#define BRK K_BREAK

static char codetable[] PROGMEM = {
//   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
0,   F9,  0,   F5,  F3,  F1,  F2,  F12, 0,   F10, F8,  F6,  F4,  '\t','`', 0,
0,   0,   0,   0,   0,   'q', '1', 0,   0,   0,   'z', 's', 'a', 'w', '2', 0,
0,   'c', 'x', 'd', 'e', '4', '3', 0,   0,   ' ', 'v', 'f', 't', 'r', '5', 0,
0,   'n', 'b', 'h', 'g', 'y', '6', 0,   0,   0,   'm', 'j', 'u', '7', '8', 0,
0,   ',', 'k', 'i', 'o', '0', '9', 0,   0,   '.', '/', 'l', ';', 'p', '-', 0,
0,   0,   '\'',0,   '[', '=', 0,   0,   CLK, 0,   '\n',']', 0,   '\\',0,   0,
0,   0,   0,   0,   0,   0,   '\b',0,   0,   '1', 0,   '4', '7', 0,   0,   0,
'0', '.', '2', '5', '6', '8', ESC,  NLK, F11, '+', '3', '-', '*', '9', SLK, 0,
0,   0,   0,   F7
};

static char codetable_shifted[] PROGMEM = {
//   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
0,   F9,  0,   F5,  F3,  F1,  F2,  F12, 0,   F10, F8,  F6,  F4,  '\t','~', 0,
0,   0,   0,   0,   0,   'Q', '!', 0,   0,   0,   'Z', 'S', 'A', 'W', '@', 0,
0,   'C', 'X', 'D', 'E', '$', '#', 0,   0,   ' ', 'V', 'F', 'T', 'R', '%', 0,
0,   'N', 'B', 'H', 'G', 'Y', '^', 0,   0,   0,   'M', 'J', 'U', '&', '*', 0,
0,   '<', 'K', 'I', 'O', ')', '(', 0,   0,   '>', '?', 'L', ':', 'P', '_', 0,
0,   0,   '"', 0,   '{', '+', 0,   0,   CLK, 0,   '\n','}', 0,   '|', 0,   0,
0,   0,   0,   0,   0,   0,   '\b',0,   0,   '1', 0,   '4', '7', 0,   0,   0,
'0', '.', '2', '5', '6', '8', ESC, NLK, F11, '+', '3', '-', '*', '9', SLK, 0,
0,   0,   0,   F7
};

// codes that follow E0 or E1
static char codetable_extended[] PROGMEM = {
//   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   PRS, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '\n',0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   END, 0,   ARL, HOM, 0,   0,   0,
INS, DEL, ARD, '5', ARR, ARU, 0,   BRK, 0,   0,   PGD, 0,   PRS, PGU, 0,   0,
0,   0,   0,   0
};

/* keyboard state */
int8_t keyup;
int8_t extended;
int8_t bitcount;
uint8_t scancode;
int8_t mods;

/* circular buffer for keys */
#define MAX_BUF 32
volatile uint8_t bufsize;
uint8_t charbuf[MAX_BUF];
uint8_t bufhead;
uint8_t buftail;

void sprinthex(char *str, uint8_t n)
{
  static char hexchars[] = "0123456789ABCDEF";
  str[0] = hexchars[(n >> 4) & 0xF];
  str[1] = hexchars[n & 0xF];
  str[2] = ' ';
}

void kb_init()
{
  /* PB3 is data line, PB4 is clock line */
  
  GIMSK |= _BV(PCIE); // enable pin change interrupt for clock line
  PCMSK |= _BV(PCINT4);

  keyup = 0;
  extended = 0;
  mods = 0;
  bitcount = 11;
  scancode = 0;
  bufsize = bufhead = buftail = 0;
}

void decode(uint8_t code)
{
  if (code == 0xF0)
    keyup = 1;
  else if (code == 0xE0 || code == 0xE1)
    extended = 1;
  else
  {
    if (keyup) // handling a key release; don't do anything
    {
      if (code == 0x12) // left shift
        mods &= ~_BV(0);
      else if (code == 0x59) // right shift
        mods &= ~_BV(1);
      else if (code == 0x14) // left/right ctrl
        mods &= (extended) ? ~_BV(3) : ~_BV(2);
    }
    else // handling a key press; store character
    {
      if (code == 0x12) // left shift
        mods |= _BV(0);
      else if (code == 0x59) // right shift
        mods |= _BV(1);
      else if (code == 0x14) // left/right ctrl
        mods |= (extended) ? _BV(3) : _BV(2);
      else if (code <= 0x83)
      {
        uint8_t chr;
        if (extended)
          chr = pgm_read_byte(codetable_extended+code);
        else if (mods & 0b1100) // ctrl
          chr = pgm_read_byte(codetable+code) & 31;
        else if (mods & 0b0011) // shift
          chr = pgm_read_byte(codetable_shifted+code);
        else
          chr = pgm_read_byte(codetable+code);

        if (!chr) chr = '?';

        // add to buffer
        if (bufsize < MAX_BUF)
        {
          charbuf[buftail] = chr;
          if (++buftail >= MAX_BUF) buftail = 0;
          bufsize++;
        }
      }
    }
    extended = 0;
    keyup = 0;
  }
}

ISR(PCINT0_vect)
{
  // ignore if this is a rising edge
  if (PINB & _BV(4))
    return;

  --bitcount;
  if (bitcount >= 2 && bitcount <= 9)
  {
    scancode >>= 1;
    if (PINB & _BV(3))
      scancode |= 0x80;
  }
  else if (bitcount == 0)
  {
    decode(scancode);
    scancode = 0;
    bitcount = 11;
  }
}

uint8_t buffer_get_key()
{
  if (bufsize == 0)
    return 0;

  uint8_t newchar = charbuf[bufhead];
  if (++bufhead >= MAX_BUF) bufhead = 0;
  bufsize--;

  return newchar;
}

void spi_init()
{
  DDRB |= _BV(1);
  USICR = _BV(USIWM0) | _BV(USICS1);
}

int main()
{
  spi_init();
  kb_init();

  sei();
  uint8_t dataval = 0;
  while (1)
  {
    // wait for start condition
    while (!(USISR & _BV(USISIF)));

    // clear counter overflow flag
    USISR = _BV(USIOIF);

    // send data
    USIDR = dataval;

    // wait for the transfer to finish
    while (!(USISR & _BV(USIOIF)));

    // get byte from keyboard buffer
    dataval = buffer_get_key();
  }
  return 0;
}
