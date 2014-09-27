#include <Wire.h>
#include <RGB_Matrix_Arduino.h>

/*
 * Scrolling Sign - generate a side-scrolling text message on an 8x8 LED matrix,
 * specifically the RGB LED Matrix from Logos Electromechanical
 * (http://www.logos-electro.com/store/rgb-led-matrix-r2)
 *
 * Author: David Bryant (david@orangemoose.com)
 * Date: 12-Jun-2014
 * Version: 1.0
 *
 * This is variant 1, which displays the 5x7 letters bottom-adjusted vertically in
 * the 8x8 matrix, which matches how the HD44780 LCD controller font is laid out, but
 * which results in elevated descending letters (g,j,p,q,y) and also never uses the top
 * row of the 8x8 matrix.
 */

#include "font_5x8v2.h"

#define BKG 0x000000
#define DOT 0x0000ff
#define DELAY 75 // scrolling delay in milliseconds

char message[] = "Final score: Giants 5, Mets 4  ";

uint8_t buf[192];

RGB_Matrix matrix = RGB_Matrix((uint8_t)0x7f, buf);

void setup() {
  
  uint8_t x, y;
  uint8_t r,g,b;
  int i, j, offset, o, w;
  
  matrix.begin();
  
  // Set buffer to background color
  for(i=0;i<8;i++) {
    for(j=0;j<8;j++) {
      matrix.setPixel(i,j,BKG);
    }
  }
  matrix.transmitBuf();
}

void loop() {
  
  int i, j, k, offset, o, w;
  uint8_t x, y, mask;
  
  for(i=0;i<strlen(message);i++) {
    // Process next letter
    offset =  message[i] - FIRST_CHAR;
    w = pgm_read_word(&font_info[offset][0]);
    o = pgm_read_word(&font_info[offset][1]);
    
    // For each column in current letter
    for(j=0;j<w;j++) {
      mask = pgm_read_byte(&font_bitmaps[o+j]);
      
      // Left shift current buffer to make room
      leftShiftBuf();
      
      // Copy in a new column
      for(k=0;k<8;k++) {
        if(mask & (1<<k)) { matrix.setPixel(k,7,DOT);}
        else              { matrix.setPixel(k,7,BKG);}
      }
      matrix.transmitBuf(); // update matrix
      delay(DELAY);         // pause
    }
    // Add in a blank column between letters
    leftShiftBuf();
    for(k=0;k<8;k++) {
      matrix.setPixel(k,7,BKG);
    }
    matrix.transmitBuf();
    delay(DELAY);
  }
}

void leftShiftBuf()
{
  uint8_t x, y;
  
  for(x=0;x<8;x++) {
    for(y=0;y<7;y++) {
      buf[(x*24)+(y*3)]     = buf[(x*24)+((y+1)*3)];
      buf[(x*24)+(y*3) + 1] = buf[(x*24)+((y+1)*3) + 1];
      buf[(x*24)+(y*3) + 2] = buf[(x*24)+((y+1)*3) + 2];
    }
  }
}
