/* Terminalscope for AVR
 * Matt Sarnoff (www.msarnoff.org)
 * Released under the "do whatever you want with it, but let me know if you've
 * used it for something awesome and give me credit" license.
 *
 * termconfig.h - terminal configuration and setup screen
 */

#ifndef _TERMCONFIG_H_
#define _TERMCONFIG_H_

#include <avr/pgmspace.h>

enum
{
  TC_BAUDRATE,
  TC_DATABITS,
  TC_PARITY,
  TC_STOPBITS,
  TC_ENTERCHAR,
  TC_LOCALECHO,
  TC_ESCSEQS,
  TC_REVVIDEO,
  TC_NUM_PARAMS
};

#define SETUP_CANCEL  1
#define SETUP_SAVE    2

/* Get the name of the specified parameter */
PGM_P cfg_param_name(uint8_t param);

/* Get the value of the specified parameter in a configuration */
uint8_t cfg_param_value(uint8_t param);

/* Get the string representation of the value of the specified parameter */
PGM_P cfg_param_value_str(uint8_t param);

/* Set the current profile to 0 or 1 */
void cfg_set_profile(uint8_t pn);

/* Returns the current profile number */
uint8_t cfg_profile();

/* Set parameter values to their defaults */
void cfg_set_defaults();

/* Load parameter values from EEPROM */
void cfg_load();

/* Save parameter values to EEPROM */
void cfg_save();

/* Print the configuration summary at the specified line */
void cfg_print_line(uint8_t linenum);

/* Start the setup screen */
void setup_start();

/* Redraw the setup screen */
void setup_redraw(); 

/* Handle keystrokes on the setup screen */
uint8_t setup_handle_key(uint8_t key);

/* Leave the setup screen */
void setup_leave();
#endif
