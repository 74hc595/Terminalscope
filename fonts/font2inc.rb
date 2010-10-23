#!/usr/bin/env ruby

# Converts a font in a PGM image to an .byte assembler data
# Each character cell must be 8x8 pixels
# Input images may be 1024x8 pixels (128 characters)
# or 2048x8 pixels (256 characters)
# White (255,255,255) pixels are considered "on"
# All others are considered "off"
# Result is written to stdout, and should be sent to an .inc file.
# Example:
#    ./font2inc.rb 6x8font.pgm > 6x8font.inc
#
# Matt Sarnoff (www.msarnoff.org)
# November 10, 2009

FLIPHORIZ = false
FLIPVERT = false

# load pgm file
pgmfile = ARGV[0]
pixels = []
File.open(pgmfile).each_line do |l|
  if $. > 4 then
    pixels << l.to_i
  end
end

width = pixels.length / 8

if width != 1024 && width != 2048 then
  puts "Input file must be 1024x8 or 2048x8 pixels, grayscale"
  exit
end

# create bitmaps
bitmaps = Array.new(width/8, [0,0,0,0,0,0,0,0])

p = 0
row = 0
col = 0
chr = 0
pixels.each do |val|
  row = p.div width
  col = p.modulo 8
  chr = (p.modulo width).div 8
  px = (val == 255) ? 1 : 0
  
  if FLIPHORIZ then
    col = 7 - col
  end
    
  if FLIPVERT then
    row = 7 - row
  end
  
  ch = bitmaps[chr].clone
  ch[row] += (px << col)
  bitmaps[chr] = ch
  
  p += 1
end

bitmaps.each do |b|
  puts ".byte " + b.join(',')
end
