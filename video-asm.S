; vim: ts=2 syntax=avr:
; Terminalscope for AVR
; Matt Sarnoff (www.msarnoff.org)
; Released under the "do whatever you want with it, but let me know if you've
; used it for something awesome and give me credit" license.
;
; video-asm.S - assembler routine for video output

#include <avr/io.h>
#include "defs.h"

#define VIDEO_PORT  _SFR_IO_ADDR(PORT(VIDEO))
#define SYNC_PORT   _SFR_IO_ADDR(PORT(SYNC))

; registers
tmp         = 0
zero        = 1
linenum     = 17  ; line number; 0 to 256
patternrow  = 18  ; pattern row; 0 to 7 (linenum mod 8)
loopcount   = 19  ; column; 0 to 32
pattablelo  = 20  ; shifted pattern table offset (low)
pattablehi  = 21  ; shifted pattern table offset (high)
patnum      = 22  ; pattern ID of the current tile
slice       = 23  ; 8-bit slice pattern
nextslice   = 22  ; temporary register for next slice

; X is a pointer to the current cell in the tilemap
; Z is a pointer to the pattern in the pattern table

; keyboard handler registers
clk      = 20
edge     = 21
bitcount = 22
scancode = 23

.text
PATTERNS:
#if defined(FONT_8x8) || defined(FONT_8X8)
#include "fonts/8x8font.inc"
#elif defined(FONT_6x8) || defined(FONT_6X8)
#include "fonts/6x8font.inc"
#elif defined(FONT_6x8_FULL) || defined(FONT_6X8_FULL)
#include "fonts/6x8x256font.inc"
#else
#error No font specified
#endif

;-- video_output_frame
; X is set to the first cell in the tilemap
; patternrow is set to 0
; beam is blanked
.global video_output_frame
video_output_frame:
  clr zero
  out VIDEO_PORT,zero     ; blank beam
  sbi SYNC_PORT,VSYNC_PIN ; freeze vertical sweep
  clr linenum             ; first line
  clr patternrow          ; row 0
  ldi XL,lo8(TILEMAP)     ; start tile pointer at first tile
  ldi XH,hi8(TILEMAP)     ;
  
  cbi SYNC_PORT,VSYNC_PIN ; start vertical sweep

;---- output_line
; X is the tilemap pointer; it starts at the first cell for this line
; and advances 32 times.
; If the pattern row number is not 7, the tilemap pointer gets bumped back
; 32 bytes at the end of the line.
output_line:
  cbi SYNC_PORT,HSYNC_PIN

  ; line setup
  ldi pattablelo,lo8(PATTERNS)  ; load the pattern table base address
  ldi pattablehi,hi8(PATTERNS)  ;
  add pattablelo,patternrow     ; add the pattern row offset
  adc pattablehi,zero           ;
  ldi r24,TILE_HEIGHT

  ; load the first slice
  ld patnum,X+            ; 2, get the pattern ID for this cell
  movw ZL,pattablelo      ; 1, load the pattern table base
  mul patnum,r24          ; 2, multiply the pattern ID by 8 to compute offset
  add ZL,r0               ; 1, add the offset to the pattern table base
  adc ZH,r1               ; 1
  lpm slice,Z             ; 3, load the slice from the pattern table 
  ldi loopcount,TILES_WIDE; 1, load loop counter

;------ output_slice
; We have 5 clocks per pixel.
; We only have one video output pin, so we can use the OUT instruction, which
; takes only one cycle.
; The next pixel is then obtained by right-shifting the slice.
; Thus, it only takes 2 cycles to output one pixel and move to the next.
; The instructions for loading the next 8-pixel slice can be interleaved
; with the output instructions.
output_slice:
  out VIDEO_PORT,slice  ; 1, pixel 0
  lsr slice             ; 2
  ld patnum,X+          ; 4
  movw ZL,pattablelo    ; 5
  
  out VIDEO_PORT,slice  ; 1, pixel 1
  lsr slice             ; 2
  ldi r24,TILE_HEIGHT   ; 3
  mul patnum,r24        ; 5

  out VIDEO_PORT,slice  ; 1, pixel 2
  lsr slice             ; 2
  add ZL,r0             ; 3
  adc ZH,r1             ; 4
  clr r1                ; 5

  out VIDEO_PORT,slice  ; 1, pixel 3
  lsr slice             ; 2
  lpm nextslice,Z       ; 5

  out VIDEO_PORT,slice  ; 1, pixel 4
  lsr slice             ; 2
  nop                   ; 3
  nop                   ; 4
  nop                   ; 5

#if TILE_WIDTH >= 7
  out VIDEO_PORT,slice  ; 1, pixel 5
  lsr slice             ; 2
  nop                   ; 3
  nop                   ; 4
  nop                   ; 5
#if TILE_WIDTH >= 8
  out VIDEO_PORT,slice  ; 1, pixel 6
  lsr slice             ; 2
  nop                   ; 3
  nop                   ; 4
  nop                   ; 5
#endif
#endif

  out VIDEO_PORT,slice  ; 1, pixel 7
  mov slice,nextslice   ; 2
;------ end output_slice

  dec loopcount         ; 3
  brne output_slice     ; 6

  out VIDEO_PORT,zero   ; blank beam
  sbi SYNC_PORT,HSYNC_PIN
  sbiw XL,1             ; we read one too many tiles; go back
  inc patternrow
  sbrs patternrow,TILE_HBIT ; unless we've drawn 8 rows, bring the tileptr back
  sbiw XL,TILES_WIDE
  andi patternrow,(TILE_HEIGHT-1) ; take patternrow mod 8
;---- end output_line

  ; waste some time so the beam can return
  ldi r24,20
.delayloop:
  dec r24
  brne .delayloop
 
  inc linenum             ; advance to next line
  cpi linenum,NUM_LINES
  brne output_line
  breq output_frame_done
  rjmp output_line

output_frame_done:
  out VIDEO_PORT,zero     ; blank beam
  sbi SYNC_PORT,HSYNC_PIN ; return beam to start
  sbi SYNC_PORT,VSYNC_PIN

  ret
;-- end video_output_frame

