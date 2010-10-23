/* Terminalscope for AVR
 * Matt Sarnoff (www.msarnoff.org)
 * Released under the "do whatever you want with it, but let me know if you've
 * used it for something awesome and give me credit" license.
 *
 * termconfig.c - terminal configuration and setup screen
 */

#include "termconfig.h"
#include "video.h"
#include "keycodes.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <avr/eeprom.h>

#define PARAM_NAME_LEN  16
#define PARAM_MAX_VALS  5
#define PARAM_VAL_LEN   5

#define EEPROM_MAGIC        0x42
#define EEPROM_MAGIC_ADDR   0x00
#define EEPROM_PROF1_ADDR   0x01
#define EEPROM_PROF2_ADDR   (EEPROM_PROF1_ADDR+TC_NUM_PARAMS)

typedef struct
{
  const char name[PARAM_NAME_LEN+1];
  const char valnames[PARAM_MAX_VALS][PARAM_VAL_LEN+1];
  uint8_t vals[PARAM_MAX_VALS];
  uint8_t numvals;
  uint8_t defaultval;
} termparam_t;

const termparam_t p_baudrate PROGMEM = {
  "Baud rate",
  { "2400", "4800", "9600", "19200", "38400" },
  { 0, 1, 2, 3, 4 },
  5,
  4
};

const termparam_t p_databits PROGMEM = {
  "Data bits",
  { "7", "8" },
  { _BV(UCSZ01), _BV(UCSZ01)|_BV(UCSZ00) },
  2,
  1
};

const termparam_t p_parity PROGMEM = {
  "Parity",
  { "N", "E", "O" },
  { 0, _BV(UPM01), _BV(UPM01)|_BV(UPM00) },
  3,
  0
};

const termparam_t p_stopbits PROGMEM = {
  "Stop bits",
  { "1", "2" },
  { 0, _BV(USBS0) },
  2,
  0
};

const termparam_t p_enterchar PROGMEM = {
  "Enter sends",
  { "CR", "LF", "CRLF" },
  { 0b10, 0b01, 0b11 },
  3,
  0
};

const termparam_t p_localecho PROGMEM = {
  "Local echo",
  { "Off", "On" },
  { 0, 1 },
  2,
  0
};

const termparam_t p_escseqs PROGMEM = {
  "Escape sequences",
  { "Off", "On", },
  { 0, 1 },
  2,
  1
};

const termparam_t p_revvideo PROGMEM = {
  "Reverse video",
  { "Off", "On", },
  { 0, 1 },
  2,
  0
};

static const termparam_t *params[] = {
  &p_baudrate,
  &p_databits,
  &p_parity,
  &p_stopbits,
  &p_enterchar,
  &p_localecho,
  &p_escseqs,
  &p_revvideo
};

static uint8_t profile1[TC_NUM_PARAMS];
static uint8_t profile2[TC_NUM_PARAMS];
static uint8_t profile1temp[TC_NUM_PARAMS];
static uint8_t profile2temp[TC_NUM_PARAMS];
static uint8_t *config;
static uint8_t profilenumber;

PGM_P cfg_param_name(uint8_t param)
{
  return (PGM_P) &(params[param]->name);
}

uint8_t cfg_param_value(uint8_t param)
{
  uint8_t val = config[param];
  return pgm_read_byte(&(params[param]->vals[val]));
}

PGM_P cfg_param_value_str(uint8_t param)
{
  uint8_t val = config[param];
  return (PGM_P) &(params[param]->valnames[val]);
}

void cfg_set_profile(uint8_t pn)
{
  profilenumber = pn;
  config = (profilenumber) ? profile2 : profile1;
}

uint8_t cfg_profile()
{
  return profilenumber;
}

void cfg_set_defaults()
{
  uint8_t i;
  for (i = 0; i < TC_NUM_PARAMS; i++)
    profile1[i] = profile2[i] = pgm_read_byte(&(params[i]->defaultval));
}

void cfg_load()
{
  eeprom_busy_wait();

  /* check for magic number */
  uint8_t magic = eeprom_read_byte((const uint8_t *)EEPROM_MAGIC_ADDR);
  if (magic != EEPROM_MAGIC)
  {
    /* no data previously stored; set defaults and save */
    cfg_set_defaults();
    cfg_save();
  }
  else
  {
    /* data previously stored; read it */
    eeprom_busy_wait();
    eeprom_read_block(profile1,(const void *)EEPROM_PROF1_ADDR,TC_NUM_PARAMS);
    eeprom_busy_wait();
    eeprom_read_block(profile2,(const void *)EEPROM_PROF2_ADDR,TC_NUM_PARAMS);

    /* sanity check */
    uint8_t i;
    for (i = 0; i < TC_NUM_PARAMS; i++)
    {
      /* if the value is corrupt, restore the default */
      uint8_t maxval = pgm_read_byte(&(params[i]->numvals));
      if (profile1[i] >= maxval)
        profile1[i] = pgm_read_byte(&(params[i]->defaultval));
      if (profile2[i] >= maxval)
        profile2[i] = pgm_read_byte(&(params[i]->defaultval));
    }
  }
}

void cfg_save()
{
  eeprom_busy_wait();
  eeprom_write_byte((uint8_t *)EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  eeprom_busy_wait();
  eeprom_write_block(profile1, (void *)EEPROM_PROF1_ADDR, TC_NUM_PARAMS);
  eeprom_busy_wait();
  eeprom_write_block(profile2, (void *)EEPROM_PROF2_ADDR, TC_NUM_PARAMS);
}

void cfg_print_line(uint8_t linenum)
{
  video_putcxy(0, linenum, '1'+profilenumber);
  video_putcxy(1, linenum, ']');

  video_putsxy_P(3,  linenum, cfg_param_value_str(TC_BAUDRATE));
  video_putsxy_P(9,  linenum, cfg_param_value_str(TC_DATABITS));
  video_putsxy_P(10, linenum, cfg_param_value_str(TC_PARITY));
  video_putsxy_P(11, linenum, cfg_param_value_str(TC_STOPBITS));
  video_putsxy_P(13, linenum, cfg_param_value_str(TC_ENTERCHAR));

  if (cfg_param_value(TC_ESCSEQS))
    video_putsxy(18, linenum, "ES");
  
  if (cfg_param_value(TC_LOCALECHO))
    video_putsxy(21, linenum, "LE");

  video_putsxy_P(TILES_WIDE-22, linenum, PSTR("(press NumLock to set)"));
}

/***** Setup screen *****/
static int8_t currparam;
static uint8_t currprof;

static void setup_print_line(int8_t param)
{
  uint8_t linenum = 4 + 2*param;
  video_gotoxy(0, linenum);
  video_clrline();

  if (param == TC_NUM_PARAMS)
    video_putsxy(3, linenum, "Save");
  else if (param == -1)
  {
    video_putsxy_P(3, linenum, PSTR("Profile"));
    video_putcxy(3+PARAM_NAME_LEN+3, linenum, '1'+currprof);
    if (currprof == profilenumber)
      video_putsxy_P(3+PARAM_NAME_LEN+5, linenum, PSTR("(active)"));
  }
  else
  {
    video_putsxy_P(3, linenum, cfg_param_name(param));
    video_putsxy_P(3+PARAM_NAME_LEN+3, linenum, cfg_param_value_str(param));
  }

  /* Highlight with inverse video if this parameter is selected */
  if (currparam == param)
    video_invert_range(2, linenum, TILES_WIDE-4);

  /* Redraw the border */
  video_putcxy(0, linenum, '\x19');
  video_putcxy(TILES_WIDE-1, linenum, '\x19');
}

void setup_redraw()
{
  video_clrscr();

  int8_t i;
  for (i = -1; i < TC_NUM_PARAMS+1; i++)
    setup_print_line(i);
  
  /* Print border */
  for (i = 1; i < TILES_WIDE-1; i++)
  {
    video_putcxy(i, 0, '\x12');
    video_putcxy(i, TILES_HIGH-1, '\x12');
  }
  for (i = 1; i < TILES_HIGH-1; i++)
  {
    video_putcxy(0, i, '\x19');
    video_putcxy(TILES_WIDE-1, i, '\x19');
  }
  video_putcxy(0, 0, '\x0D');
  video_putcxy(TILES_WIDE-1, 0, '\x0C');
  video_putcxy(0, TILES_HIGH-1, '\x0E');
  video_putcxy(TILES_WIDE-1, TILES_HIGH-1, '\x0B');

  video_putsxy_P(5, TILES_HIGH-2,
      PSTR("\x03\x04: select     Enter: change     Esc: quit"));
}

void setup_start()
{
  video_hide_cursor();

  currparam = -1;

  /* copy the current settings into the temps */
  memcpy(profile1temp, profile1, TC_NUM_PARAMS);
  memcpy(profile2temp, profile2, TC_NUM_PARAMS);
  config = (profilenumber) ? profile2temp : profile1temp;
  currprof = profilenumber;

  setup_redraw();
}

uint8_t setup_handle_key(uint8_t key)
{
  uint8_t ret = 0;

  switch (key)
  {
    case K_UP:
    {
      int8_t oldparam = currparam;
      currparam--;
      if (currparam < -1)
        currparam = TC_NUM_PARAMS;
      setup_print_line(oldparam);
      setup_print_line(currparam);
      break;
    }
    case K_DOWN:
    {
      int8_t oldparam = currparam;
      currparam++;
      if (currparam > TC_NUM_PARAMS)
        currparam = -1;
      setup_print_line(oldparam);
      setup_print_line(currparam);
      break;
    }
    case '\n':
    {
      if (currparam == TC_NUM_PARAMS) /* save and quit */
      {
        /* copy the temp settings back */
        memcpy(profile1, profile1temp, TC_NUM_PARAMS);
        memcpy(profile2, profile2temp, TC_NUM_PARAMS);
        config = (profilenumber) ? profile2 : profile1;
        cfg_save();
        ret = SETUP_SAVE;
      }
      else if (currparam == -1) /* change profile */
      {
        currprof = !currprof;
        config = (currprof) ? profile2temp : profile1temp;
        setup_redraw();
      }
      else
      {
        uint8_t maxval = pgm_read_byte(&(params[currparam]->numvals));
        config[currparam]++;
        if (config[currparam] >= maxval)
          config[currparam] = 0;
        setup_print_line(currparam);
      }
      break;
    }
    case '\x1B': /* ESC */
    case K_NUMLK:
    {
      config = (profilenumber) ? profile2 : profile1;
      ret = SETUP_CANCEL;
      break;
    }
  }

  return ret;
}

void setup_leave()
{
  video_welcome();
  cfg_print_line(video_gety());
  video_lfwd();
  video_show_cursor();
}
