/* Terminalscope for AVR
 * Matt Sarnoff (www.msarnoff.org)
 * Released under the "do whatever you want with it, but let me know if you've
 * used it for something awesome and give me credit" license.
 *
 * video.c - output and cursor handling routines
 */

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "defs.h"

#include <avr/pgmspace.h>

/* Set up video ports and timer parameters. */
/* Warning: uses TIMER0 */
void video_setup();

/* Start frame timer. The timer fires at roughly 60 Hz. */
void video_start();

/* Waits for the frame timer to trigger.
 * Use this once per iteration of your main loop, then call
 * video_output_frame() immediately afterward. */
void video_wait();

/* Outputs one frame of video.
 * Must be called roughly 60 times per second. */
void video_output_frame();

void keyhandler();
void keyhandler_for_interrupt();

/****** Output routines ******/

/* Clears the screen and prints the welcome message. */
void video_welcome();

/* Set the top and bottom margins. The cursor is moved to the first column
 * of the top margin. */
void video_set_margins(int8_t top, int8_t bottom);

/* Resets the top margin to the top line of the screen and the bottom margin
 * to the bottom line of the screen. */
void video_reset_margins();

/* Returns the line number of the top margin. */
int8_t video_top_margin();

/* Returns the line number of the bottom margin. */
int8_t video_bottom_margin();

/* Sets whether or not the screen should be displayed in reverse video. */
void video_set_reverse(uint8_t val);

/* Clears the screen, returns the cursor to (0,0), and resets the margins
 * to the full size of the screen. */
void video_clrscr();

/* Clears the current line and returns the cursor to he start of the line. */
void video_clrline();

/* Clears the rest of the line from the cursor position to the end of the line
 * without moving the cursor. */
void video_clreol();

/* erasemode = 0: erase from the cursor (inclusive) to the end of the screen.
 * erasemode = 1: erase from the start of the screen to the cursor (inclusive).
 * erasemode = 2: erase the entire screen.
 * The cursor does not move.
 * This call corresponds to the ANSI "Erase in Display" escape sequence. */
void video_erase(uint8_t erasemode);

/* erasemode = 0: erase from the cursor (inclusive) to the end of the line.
 * erasemode = 1: erase from the start of the line to the cursor (inclusive).
 * erasemode = 2: erase the entire line.
 * The cursor does not move.
 * This call corresponds to the ANSI "Erase in Line" escape sequence. */
void video_eraseline(uint8_t erasemode);

/* Overwrites the character at the cursor position without moving it. */
void video_setc(char c);

/* Prints a character at the cursor position and advances the cursor.
 * Carriage returns and newlines are interpreted. */
void video_putc(char c);

/* Prints a character at the cursor position and advances the cursor.
 * Carriage returns and newlines are not interpreted. */
void video_putc_raw(char c);

/* Prints a string at the cursor position and advances the cursor.
 * The screen will be scrolled if necessary. */
void video_puts(char *str);

/* Prints a string from program memory at the cursor position and advances
 * the cursor. The screen will be scrolled if necessary. */
void video_puts_P(PGM_P str);

/* Prints a character at the specified position. The cursor is not advanced. */
void video_putcxy(int8_t x, int8_t y, char c);

/* Prints a string at the specified position. Escape characters are not
 * interpreted. The cursor is not advanced and the screen is not scrolled. */
void video_putsxy(int8_t x, int8_t y, char *str);

/* Prints a string from program memory at the specified position.
 * Escape characters are not interpreted. The cursor is not advanced and the
 * screen is not scrolled. */
void video_putsxy_P(int8_t x, int8_t y, PGM_P str);

/* Prints a string on the specified line. The previous contents of the line
 * are erased. Escape characters are not interpreted. The cursor is not
 * advanced and the screen is not scrolled. */
void video_putline(int8_t y, char *str);

/* Prints a string from program memory on the specified line.
 * The previous contents of the line are erased. Escape characters are not
 * interpreted. The cursor is not advanced and the screen is not scrolled. */
void video_putline_P(int8_t y, PGM_P str);

/* Moves the cursor to the specified coordinates. 
 * This function can be used to move the cursor outside of the margins. */
void video_gotoxy(int8_t x, int8_t y);

/* Moves the cursor left/right by the specified number of columns. */
void video_movex(int8_t dx);

/* Moves the cursor up/down the specified number of lines. 
 * The cursor does not move beyond the top/bottom margins. */
void video_movey(int8_t dy);

/* Moves the cursor to the start of the current line. */
void video_movesol();

/* Sets the horizontal position of the cursor. */
void video_setx(int8_t x);

/* Advances the cursor one character to the right, advancing to a new line if
 * necessary. */
void video_cfwd();

/* Advances the cursor one line down and moves it to the start of the new line.
 * The screen is scrolled if the bottom margin is exceeded. */
void video_lfwd();

/* Advances the cursor one line down but does not return the cursor to the start
 * of the new line. The screen is scrolled if the bottom margin is exceeded. */
void video_lf();

/* Moves the cursor one character back, moving to the end of the previous line
 * if necessary. */
void video_cback();

/* Moves the cursor to the end of the previous line, or to the first column
 * of the top margin if the top margin is exceeded. */
void video_lback();

/* Scrolls the region between the top and bottom margins up one line.
 * A blank line is added at the bottom. The cursor is not moved. */
void video_scrollup();

/* Scrolls the region between the top and bottom margins down one line.
 * A blank lines is added at the top. The cursor is not moved. */
void video_scrolldown();

/* Returns the x coordinate of the cursor. */
int8_t video_getx();

/* Returns the y coordinate of the cursor. */
int8_t video_gety();

/* Returns the character at the specified position. */
char video_charat(int8_t x, int8_t y);

/* Shows the cursor. Off by default. */
void video_show_cursor();

/* Hides the cursor. */
void video_hide_cursor();

/* Returns 1 if the cursor is visible, 0 if it is hidden. */
uint8_t video_cursor_visible();

/* Set inverse video for the character range specified. */
void video_invert_range(int8_t x, int8_t y, uint8_t rangelen);
#endif
