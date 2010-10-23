/* Terminalscope for AVR
 * Matt Sarnoff (www.msarnoff.org)
 * Released under the "do whatever you want with it, but let me know if you've
 * used it for something awesome and give me credit" license.
 *
 * main.c - main functions
 */

#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "defs.h"
#include "video.h"

/* SPI port definitions for keyboard buffer */
#define DDR_SPI PORTB
#define DD_SS   2
#define DD_MOSI 3
#define DD_MISO 4
#define DD_SCK  5

void puthex(uint8_t n)
{
  static char hexchars[] = "0123456789ABCDEF";
  char hexstr[5];
  hexstr[0] = hexchars[(n >> 4) & 0xF];
  hexstr[1] = hexchars[n & 0xF];
  hexstr[2] = hexstr[3] = ' ';
  hexstr[4] = '\0';
  video_puts(hexstr);
}

extern void app_setup();
extern void app_handle_key(uint8_t key);
extern uint8_t app_main_loop();
uint16_t frame;

void spi_init()
{
  PORTB = 0;
  DDRB |= _BV(DD_MOSI) | _BV(DD_SCK) | _BV(DD_SS);
  SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0);
  volatile char dummy;
  dummy = SPSR;
  dummy = SPDR;
}

uint8_t spi_write_read(uint8_t dataout)
{
  uint8_t datain;
  SPDR = dataout;
  while (!(SPSR & _BV(SPIF)));
  datain = SPSR; /* clear SPIF flag */
  datain = SPDR;
  return datain;
}

void poll_keyboard()
{
  /* pull keystrokes out of the buffer until it's empty
   * and send them to the app */
  uint8_t key;
  while ((key = spi_write_read(0)))
    app_handle_key(key);
}

int main()
{
  video_setup();
  spi_init();

  app_setup();

  video_start();

  frame = 0;
  for (;;)
  {
    video_wait();
    video_output_frame();
  
    app_main_loop();
    poll_keyboard();

    frame++;
  }

  return 0;
}

