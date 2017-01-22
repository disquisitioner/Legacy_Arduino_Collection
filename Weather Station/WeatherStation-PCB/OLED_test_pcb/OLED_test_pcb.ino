/*********************************************************************
This sketch is the first bring-up test for my Weather Station PCB,
designed to check the wiring for the OLED display.  No other features of
the Weather Station PCB are used.

Basic stub for using an OLED display such as from Adafruit
(http://www.adafruit.com/category/63_98 ) via their libraries. Check
out the example sketches packaged with those libraries to see how to 
use all the text, drawing, and scrolling functions.

Notes:
 *) You need to modify Adafruit_SSD1306.h to reflect the size of
    the OLED you are using (either 32 or 64 pixels). 
 *) The display can co-exist with other SPI devices, in which case the
    OLED_DC, OLED_CLK, and OLED_MOSI (Data) pins may be shared but
    the CS (Chip Select) connection must be unique for each device.
    
Author: David Bryant (djbryant@gmail.com)
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()   {                
  // Serial.begin(9600);  // Uncomment if using Serial output
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  
  display.clearDisplay();   // clears the screen and buffer

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.println(3.141592);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  display.display();
}


void loop() {
  
}
