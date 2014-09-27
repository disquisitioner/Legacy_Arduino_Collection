/*
 * Hello, My Name is OLED -- a simple OLED name badge
 *
 * Copyright (c) 2014 David Bryant <djbryant@gmail.com>
 *
 * Author: David Bryant (david@orangemoose.com)
 * Version: 1.0
 * Date: 1 March 2014
 *
 * Creates an animated, graphical name badge (of the "Hello My Name Is" variety) using an
 * Arduino and a 128x64 OLED with SSD1306 controller via the Adafruit GFX library.
 *
 * Presumes the OLED is wired for SPI communications
 *
 * Awesome font generation capabilities via The Dot Factory, a great tool by Eran Duchan
 * at http://www.pavius.net/2009/07/the-dot-factory-an-lcd-font-and-image-generator/
 */
 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Specify Arduino pins used to control the OLED
#define OLED_MOSI  13 // OLED "Data" pin (SPI MOSI = output from master device, so from Arduino thus = Data)
#define OLED_CLK   12
#define OLED_DC    11
#define OLED_RESET 10
#define OLED_CS     9
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

/*
 * Font selection is a personal choice.  I wanted a monospaced font to avoid having to
 * deal with kerning but also wanted a font with some style to it so chose Deja Vu Sans Mono.
 * Some experimenting with sizes was needed to get letters large enough to be easily seen
 * but not so large that typical names wouldn't fit on the 128x64 OLED
 */
#define CHAR_HEIGHT 28
#define CHAR_SPACE   3
// 
//  Font data for DejaVu Sans Mono 20pt
// 

// Character bitmaps for DejaVu Sans Mono 20pt
static unsigned char PROGMEM dejaVuSansMono_20ptBitmaps[] = 
{
	// @0 '!' (4 pixels wide)
	B00000000, //     
	B00000000, //     
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B00000000, //     
	B00000000, //     
	B00000000, //     
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B11110000, // @@@@
	B00000000, //     
	B00000000, //     
	B00000000, //     
	B00000000, //     
	B00000000, //     
	B00000000, //     

	// @28 'B' (14 pixels wide)
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B11111111, B11000000, // @@@@@@@@@@    
	B11111111, B11110000, // @@@@@@@@@@@@  
	B11111111, B11110000, // @@@@@@@@@@@@  
	B11110000, B11111000, // @@@@    @@@@@ 
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B11110000, // @@@@    @@@@  
	B11111111, B11110000, // @@@@@@@@@@@@  
	B11111111, B10000000, // @@@@@@@@@     
	B11111111, B11110000, // @@@@@@@@@@@@  
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B01111100, // @@@@     @@@@@
	B11111111, B11111000, // @@@@@@@@@@@@@ 
	B11111111, B11110000, // @@@@@@@@@@@@  
	B11111111, B11100000, // @@@@@@@@@@@   
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               

	// @84 'D' (14 pixels wide)
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B11111111, B00000000, // @@@@@@@@      
	B11111111, B11100000, // @@@@@@@@@@@   
	B11111111, B11110000, // @@@@@@@@@@@@  
	B11110000, B11111000, // @@@@    @@@@@ 
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B01111000, // @@@@     @@@@ 
	B11110000, B11111000, // @@@@    @@@@@ 
	B11111111, B11110000, // @@@@@@@@@@@@  
	B11111111, B11100000, // @@@@@@@@@@@   
	B11111111, B00000000, // @@@@@@@@      
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               

	// @140 'H' (13 pixels wide)
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11111111, B11111000, // @@@@@@@@@@@@@
	B11111111, B11111000, // @@@@@@@@@@@@@
	B11111111, B11111000, // @@@@@@@@@@@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              

	// @196 'a' (14 pixels wide)
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00011111, B11100000, //    @@@@@@@@   
	B01111111, B11111000, //  @@@@@@@@@@@@ 
	B01111111, B11111000, //  @@@@@@@@@@@@ 
	B01100000, B01111100, //  @@      @@@@@
	B00000000, B00111100, //           @@@@
	B00011111, B11111100, //    @@@@@@@@@@@
	B01111111, B11111100, //  @@@@@@@@@@@@@
	B01111111, B11111100, //  @@@@@@@@@@@@@
	B11111000, B00111100, // @@@@@     @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B01111100, // @@@@     @@@@@
	B11111000, B11111100, // @@@@@   @@@@@@
	B01111111, B11111100, //  @@@@@@@@@@@@@
	B01111111, B10111100, //  @@@@@@@@ @@@@
	B00011111, B00111100, //    @@@@@  @@@@
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               

	// @252 'd' (13 pixels wide)
	B00000000, B00000000, //              
	B00000000, B01111000, //          @@@@
	B00000000, B01111000, //          @@@@
	B00000000, B01111000, //          @@@@
	B00000000, B01111000, //          @@@@
	B00000000, B01111000, //          @@@@
	B00000000, B01111000, //          @@@@
	B00011110, B01111000, //    @@@@  @@@@
	B00111111, B01111000, //   @@@@@@ @@@@
	B01111111, B11111000, //  @@@@@@@@@@@@
	B01111000, B11111000, //  @@@@   @@@@@
	B11111000, B11111000, // @@@@@   @@@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11110000, B01111000, // @@@@     @@@@
	B11111000, B11111000, // @@@@@   @@@@@
	B01111000, B11111000, //  @@@@   @@@@@
	B01111111, B11111000, //  @@@@@@@@@@@@
	B00111111, B01111000, //   @@@@@@ @@@@
	B00011110, B01111000, //    @@@@  @@@@
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              

	// @308 'e' (14 pixels wide)
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00001111, B11000000, //     @@@@@@    
	B00111111, B11110000, //   @@@@@@@@@@  
	B01111111, B11111000, //  @@@@@@@@@@@@ 
	B01111000, B01111000, //  @@@@    @@@@ 
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11111111, B11111100, // @@@@@@@@@@@@@@
	B11111111, B11111100, // @@@@@@@@@@@@@@
	B11111111, B11111100, // @@@@@@@@@@@@@@
	B11110000, B00000000, // @@@@          
	B11110000, B00000000, // @@@@          
	B01111000, B00011000, //  @@@@      @@ 
	B01111111, B11111000, //  @@@@@@@@@@@@ 
	B00111111, B11111000, //   @@@@@@@@@@@ 
	B00001111, B11100000, //     @@@@@@@   
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               

	// @364 'i' (14 pixels wide)
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B01111111, B10000000, //  @@@@@@@@     
	B01111111, B10000000, //  @@@@@@@@     
	B01111111, B10000000, //  @@@@@@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B00000111, B10000000, //      @@@@     
	B11111111, B11111100, // @@@@@@@@@@@@@@
	B11111111, B11111100, // @@@@@@@@@@@@@@
	B11111111, B11111100, // @@@@@@@@@@@@@@
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               

	// @420 'l' (13 pixels wide)
	B00000000, B00000000, //              
	B11111111, B00000000, // @@@@@@@@     
	B11111111, B00000000, // @@@@@@@@     
	B11111111, B00000000, // @@@@@@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B10000000, //     @@@@@    
	B00000111, B11111000, //      @@@@@@@@
	B00000111, B11111000, //      @@@@@@@@
	B00000011, B11111000, //       @@@@@@@
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              

	// @476 'n' (12 pixels wide)
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B11110011, B11000000, // @@@@  @@@@  
	B11111111, B11100000, // @@@@@@@@@@@ 
	B11111111, B11110000, // @@@@@@@@@@@@
	B11111001, B11110000, // @@@@@  @@@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B11110000, B11110000, // @@@@    @@@@
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             
	B00000000, B00000000, //             

	// @532 'o' (14 pixels wide)
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00001111, B11000000, //     @@@@@@    
	B00111111, B11110000, //   @@@@@@@@@@  
	B01111111, B11111000, //  @@@@@@@@@@@@ 
	B01111000, B01111000, //  @@@@    @@@@ 
	B11111000, B01111100, // @@@@@    @@@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B11111000, B01111100, // @@@@@    @@@@@
	B01111000, B01111000, //  @@@@    @@@@ 
	B00111111, B11111000, //   @@@@@@@@@@@ 
	B00111111, B11110000, //   @@@@@@@@@@  
	B00001111, B11000000, //     @@@@@@    
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               

	// @588 'r' (11 pixels wide)
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B11110011, B11000000, // @@@@  @@@@ 
	B11110111, B11100000, // @@@@ @@@@@@
	B11111111, B11100000, // @@@@@@@@@@@
	B11111100, B00100000, // @@@@@@    @
	B11111000, B00000000, // @@@@@      
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B11110000, B00000000, // @@@@       
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            
	B00000000, B00000000, //            

	// @644 't' (13 pixels wide)
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B11111111, B11111000, // @@@@@@@@@@@@@
	B11111111, B11111000, // @@@@@@@@@@@@@
	B11111111, B11111000, // @@@@@@@@@@@@@
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B00000000, //     @@@@     
	B00001111, B11111000, //     @@@@@@@@@
	B00000111, B11111000, //      @@@@@@@@
	B00000011, B11111000, //       @@@@@@@
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              
	B00000000, B00000000, //              

	// @700 'v' (14 pixels wide)
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B11110000, B00111100, // @@@@      @@@@
	B11110000, B00111100, // @@@@      @@@@
	B01111000, B01111000, //  @@@@    @@@@ 
	B01111000, B01111000, //  @@@@    @@@@ 
	B01111000, B01111000, //  @@@@    @@@@ 
	B00111000, B01110000, //   @@@    @@@  
	B00111100, B11110000, //   @@@@  @@@@  
	B00111100, B11110000, //   @@@@  @@@@  
	B00011100, B11100000, //    @@@  @@@   
	B00011100, B11100000, //    @@@  @@@   
	B00011111, B11100000, //    @@@@@@@@   
	B00001111, B11000000, //     @@@@@@    
	B00001111, B11000000, //     @@@@@@    
	B00001111, B11000000, //     @@@@@@    
	B00001111, B11000000, //     @@@@@@    
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               
	B00000000, B00000000, //               

	// @756 'y' (15 pixels wide)
	B00000000, B00000000, //                
	B00000000, B00000000, //                
	B00000000, B00000000, //                
	B00000000, B00000000, //                
	B00000000, B00000000, //                
	B00000000, B00000000, //                
	B00000000, B00000000, //                
	B11110000, B00011110, // @@@@       @@@@
	B01111000, B00111100, //  @@@@     @@@@ 
	B01111000, B00111100, //  @@@@     @@@@ 
	B01111000, B00111100, //  @@@@     @@@@ 
	B00111100, B01111000, //   @@@@   @@@@  
	B00111100, B01111000, //   @@@@   @@@@  
	B00011100, B01111000, //    @@@   @@@@  
	B00011110, B11110000, //    @@@@ @@@@   
	B00011110, B11110000, //    @@@@ @@@@   
	B00001110, B11110000, //     @@@ @@@@   
	B00001111, B11100000, //     @@@@@@@    
	B00000111, B11100000, //      @@@@@@    
	B00000111, B11000000, //      @@@@@     
	B00000111, B11000000, //      @@@@@     
	B00000011, B11000000, //       @@@@     
	B00000011, B10000000, //       @@@      
	B00000111, B10000000, //      @@@@      
	B00001111, B10000000, //     @@@@@      
	B01111111, B00000000, //  @@@@@@@       
	B01111110, B00000000, //  @@@@@@        
	B01111100, B00000000, //  @@@@@         
};

// Character descriptors for DejaVu Sans Mono 20pt
// { [Char width in bits], [Offset into dejaVuSansMono_20ptCharBitmaps in bytes] }
const int16_t PROGMEM dejaVuSansMono_20ptDescriptors[][2] = 
{
	{4, 0}, 		// ! 
	{0, 0}, 		// " 
	{0, 0}, 		// # 
	{0, 0}, 		// $ 
	{0, 0}, 		// % 
	{0, 0}, 		// & 
	{0, 0}, 		// ' 
	{0, 0}, 		// ( 
	{0, 0}, 		// ) 
	{0, 0}, 		// * 
	{0, 0}, 		// + 
	{0, 0}, 		// , 
	{0, 0}, 		// - 
	{0, 0}, 		// . 
	{0, 0}, 		// / 
	{0, 0}, 		// 0 
	{0, 0}, 		// 1 
	{0, 0}, 		// 2 
	{0, 0}, 		// 3 
	{0, 0}, 		// 4 
	{0, 0}, 		// 5 
	{0, 0}, 		// 6 
	{0, 0}, 		// 7 
	{0, 0}, 		// 8 
	{0, 0}, 		// 9 
	{0, 0}, 		// : 
	{0, 0}, 		// ; 
	{0, 0}, 		// < 
	{0, 0}, 		// = 
	{0, 0}, 		// > 
	{0, 0}, 		// ? 
	{0, 0}, 		// @ 
	{0, 0}, 		// A 
	{14, 28}, 		// B 
	{0, 0}, 		// C 
	{14, 84}, 		// D 
	{0, 0}, 		// E 
	{0, 0}, 		// F 
	{0, 0}, 		// G 
	{13, 140}, 		// H 
	{0, 0}, 		// I 
	{0, 0}, 		// J 
	{0, 0}, 		// K 
	{0, 0}, 		// L 
	{0, 0}, 		// M 
	{0, 0}, 		// N 
	{0, 0}, 		// O 
	{0, 0}, 		// P 
	{0, 0}, 		// Q 
	{0, 0}, 		// R 
	{0, 0}, 		// S 
	{0, 0}, 		// T 
	{0, 0}, 		// U 
	{0, 0}, 		// V 
	{0, 0}, 		// W 
	{0, 0}, 		// X 
	{0, 0}, 		// Y 
	{0, 0}, 		// Z 
	{0, 0}, 		// [ 
	{0, 0}, 		// backslash
	{0, 0}, 		// ] 
	{0, 0}, 		// ^ 
	{0, 0}, 		// _ 
	{0, 0}, 		// ` 
	{14, 196}, 		// a 
	{0, 0}, 		// b 
	{0, 0}, 		// c 
	{13, 252}, 		// d 
	{14, 308}, 		// e 
	{0, 0}, 		// f 
	{0, 0}, 		// g 
	{0, 0}, 		// h 
	{14, 364}, 		// i 
	{0, 0}, 		// j 
	{0, 0}, 		// k 
	{13, 420}, 		// l 
	{0, 0}, 		// m 
	{12, 476}, 		// n 
	{14, 532}, 		// o 
	{0, 0}, 		// p 
	{0, 0}, 		// q 
	{11, 588}, 		// r 
	{0, 0}, 		// s 
	{13, 644}, 		// t 
	{0, 0}, 		// u 
	{14, 700}, 		// v 
	{0, 0}, 		// w 
	{0, 0}, 		// x 
	{15, 756}, 		// y 
};

/* Font information for DejaVu Sans Mono 20pt
const FONT_INFO dejaVuSansMono_20ptFontInfo =
{
	4, //  Character height
	'!', //  Start character
	'y', //  End character
	NULL, //  Character block lookup
	dejaVuSansMono_20ptDescriptors, //  Character descriptor array
	dejaVuSansMono_20ptBitmaps, //  Character bitmap array
};
*/
#define FIRST_CHAR  '!'


void setup() {
  
  // Serial.begin(9600);
  
  // generate high voltage internally from the 3.3V supply line
  display.begin(SSD1306_SWITCHCAPVCC);
  
  display.clearDisplay();  // Clear the screen & buffer
  display.setTextColor(WHITE);
}

void loop() {
  
  // Suitable page delays are handled in each page function
  firstPage();
  secondPage();
  thirdPage();
}

// *********************************************************************************
// First page says "Hello!" ****
void firstPage()
{
  int16_t i, j;
  

  display.clearDisplay();
  // Draw a series of radial lines as a background pattern
  for(i=0;i<display.width();i+=4) {
    display.drawLine(display.width()-i-1,0,i,display.height()-1,WHITE);
    display.display();
  }
  for(i=0;i<display.height();i+=4) {
    display.drawLine(0,i,display.width()-1,display.height()-i,WHITE);
    display.display();
  }
  delay(500);  // Let the user briefly enjoy the nice pattern
  
  display.fillRoundRect(10,15,110,32,5,BLACK);  // Sized to fit "Hello!"
  showText3("Hello!",21,20);  // Using our custom font
  display.display();
  
  delay(2500);  // Allow time for reading what we've written
}


// *********************************************************************************
// Second page says "My Name is" on a dotted background
void secondPage()
{
  int16_t i, j;


  display.clearDisplay();  // Clear the screen & buffer
  // Draw the dots
  for(j=0;j<display.height();j+=2) {
    for(i=(j%4)/2;i<display.width();i+=2) {
      display.drawPixel(i,j,WHITE);
    }
  }
  display.fillRoundRect(16,8,96,48,8,BLACK);   // Sized to fit "My Name is"
  display.setTextSize(2);
  display.setCursor(24,15);
  display.println("My Name");
  display.setCursor(55,34);
  display.println("is");
  display.display();
 
  delay(2000);  // Allow time for reading what we've written
}

// ********************************************************************************
// Third page shows your name
char firstname[] = "David";
char lastname[] = "Bryant";

void thirdPage()
{
  
  int16_t i, j;

  // Display name, animated
  vanishName(firstname,23,2,lastname,17,36);

  // Other (simpler) variations
  // display.clearDisplay();  // Clear the screen & buffer
  // display.display();
  // Variation 1: Display name using scaled font built into OLED
  //   display.setTextSize(3);
  //   display.setCursor(20,5);  display.println("DAVID");
  //   display.setCursor(10,35); display.println("BRYANT");
  // or Variation 2: Display name with our custom font (static)
  //   showText3("David",23,2);
  //   showText3("Bryant",17,32);
  // display.display();
  // Invert display as a simple effect
  // delay(4000);
  // display.invertDisplay(true);
  // delay(4000);
  // display.invertDisplay(false);
  // display.display();
}

void showText3(char *string, int x, int y)
{
  int i, o, w, wb;
  int offset;
  int str_width;
  
  // Serial.println(string);
  
  str_width = 0;
  for(i=0;string[i]!=0;i++) {
    offset = string[i] - FIRST_CHAR;
    w = pgm_read_word(&dejaVuSansMono_20ptDescriptors[offset][0]);
    wb = (w+7)/8;
    o = pgm_read_word(&dejaVuSansMono_20ptDescriptors[offset][1]);
    if(w != 0) {
      display.drawBitmap(x,y,&dejaVuSansMono_20ptBitmaps[o],8*wb,CHAR_HEIGHT,WHITE);
    }
    else {
      // Serial.print("Font missing character '");
    }
    x += w + CHAR_SPACE;
    str_width += w + CHAR_SPACE;
  }
  // This helps during development so you can better center the string
  // Serial.print(str_width-CHAR_SPACE); Serial.println(" pixels wide");
}

/*
 * Assorted routines to animate the name after it has been displayed.  Options available are:
 *   flyName()   --  Causes the first and last name to zoom radially off the screen.  Cool, but
 *                   really only works well on 16MHz devices due to the work involved in erasing
 *                   and re-drawing the letters.
 *   vanishName() -- Causes letters in the name to randomly disappear. Works much better on all
 *                   devices, both 8MHz and 16MHz
 * Both routines take the general approach that each letter needs to be rendered and accounted
 * for separately, using arrays that keep track of the revelant data for each.
 */
 
// Some constants needed to handle the per-letter data arrays
#define MAXFIRST 12    // Largest allowed first name
#define MAXLAST  12    // Largest allowed last name
#define XPOS 0
#define YPOS 1
#define BWIDTH 2
#define OFFSET 3
// Attributes used to fly letters off screen
#define DX 4
#define DY 5

void flyName(char *firstname, int fx, int fy, char *lastnamne, int lx, int ly)
{
  // In this algorithm we need (x,y), byte width, offset, and (deltax,deltay) for each letter
  int fltrs[MAXFIRST][6];
  int lltrs[MAXLAST][6];
  int i, j, w, deltax, flen, llen;
  float theta;
  
  // Build working arrays - one for first name...
  flen = strlen(firstname);
  theta = 3.14159 / (flen - 1);
  for(i=0,deltax=0;i<flen;i++) {
    fltrs[i][XPOS] = fx+deltax;
    fltrs[i][YPOS] = fy;
    w = pgm_read_word(&dejaVuSansMono_20ptDescriptors[firstname[i] - FIRST_CHAR][0]);
    fltrs[i][BWIDTH] = (w+7)/8;
    fltrs[i][OFFSET]  = pgm_read_word(&dejaVuSansMono_20ptDescriptors[firstname[i] - FIRST_CHAR][1]);
    fltrs[i][DX] = -8 * cos(i*theta);
    fltrs[i][DY] = -8 * sin(i*theta);
    
    deltax += w + CHAR_SPACE;
  }
  
   // ...and one for last name
   llen = strlen(lastname);
   theta = 3.14159 / (flen - 1);
   for(i=0,deltax=0;i<llen;i++) {
    lltrs[i][XPOS] = lx+deltax;
    lltrs[i][YPOS] = ly;
    w = pgm_read_word(&dejaVuSansMono_20ptDescriptors[lastname[i] - FIRST_CHAR][0]);
    lltrs[i][BWIDTH] = (w+7)/8;
    lltrs[i][OFFSET] = pgm_read_word(&dejaVuSansMono_20ptDescriptors[lastname[i] - FIRST_CHAR][1]);
    lltrs[i][DX] = -8 * cos(i*theta);
    lltrs[i][DY] =  8 * sin(i*theta);
    
    deltax += w + CHAR_SPACE;
  }

  // Display the name and pause for it to be read
  display.clearDisplay();
  for(i=0;i<flen;i++) {
    display.drawBitmap(
      fltrs[i][XPOS],fltrs[i][YPOS],
      &dejaVuSansMono_20ptBitmaps[fltrs[i][OFFSET]],
      8*fltrs[i][BWIDTH],
      CHAR_HEIGHT,WHITE);
  }
  for(i=0;i<llen;i++) {
    display.drawBitmap(
      lltrs[i][XPOS],lltrs[i][YPOS],
      &dejaVuSansMono_20ptBitmaps[lltrs[i][OFFSET]],
      8*lltrs[i][BWIDTH],
      CHAR_HEIGHT,WHITE);
  }
  display.display();
  delay(3500);  // See my name...
  
  // Loop and erase/move/draw so they fly off screen
  for(j=0;j<6;j++) {
    // Erase names, then move each letter to its new location
    display.clearDisplay();
    for(i=0;i<flen;i++) {
        fltrs[i][XPOS] += fltrs[i][DX];
        fltrs[i][YPOS] += fltrs[i][DY];
    }
   // Move last name 
   for(i=0;i<llen;i++) {
        lltrs[i][XPOS] += lltrs[i][DX];
        lltrs[i][YPOS] += lltrs[i][DY];
    }
    
    // Draw the letters
    for(i=0;i<flen;i++) {
      display.drawBitmap(
        fltrs[i][XPOS],fltrs[i][YPOS],
        &dejaVuSansMono_20ptBitmaps[fltrs[i][OFFSET]],
        8*fltrs[i][BWIDTH],
        CHAR_HEIGHT,WHITE);
    }
    for(i=0;i<llen;i++) {
      display.drawBitmap(
        lltrs[i][XPOS],lltrs[i][YPOS],
        &dejaVuSansMono_20ptBitmaps[lltrs[i][OFFSET]],
        8*lltrs[i][BWIDTH],
        CHAR_HEIGHT,WHITE);
    }
    display.display();  // update display
  }
  display.clearDisplay();  // just to be sure
  delay(1000);
}
//
void vanishName(char *firstname, int fx, int fy, char *lastnamne, int lx, int ly)
{
  // In this algorithm we  need (x,y), byte width and offset for each letter
  int nltrs[MAXFIRST+MAXLAST][4];
  int shuffle[MAXFIRST+MAXLAST];
  int i, j, w, deltax, flen, llen, nlen;
  
  // Build working array - first name...
  flen = strlen(firstname);
  for(i=0,deltax=0;i<flen;i++) {
    nltrs[i][XPOS] = fx+deltax;
    nltrs[i][YPOS] = fy;
    w = pgm_read_word(&dejaVuSansMono_20ptDescriptors[firstname[i] - FIRST_CHAR][0]);
    nltrs[i][BWIDTH] = (w+7)/8;
    nltrs[i][OFFSET]  = pgm_read_word(&dejaVuSansMono_20ptDescriptors[firstname[i] - FIRST_CHAR][1]);
    shuffle[i] = i;
    deltax += w + CHAR_SPACE;
  }
  
   // ...then last name
   llen = strlen(lastname);
   for(i=0,deltax=0;i<llen;i++) {
    nltrs[i+flen][XPOS] = lx+deltax;
    nltrs[i+flen][YPOS] = ly;
    w = pgm_read_word(&dejaVuSansMono_20ptDescriptors[lastname[i] - FIRST_CHAR][0]);
    nltrs[i+flen][BWIDTH] = (w+7)/8;
    nltrs[i+flen][OFFSET] = pgm_read_word(&dejaVuSansMono_20ptDescriptors[lastname[i] - FIRST_CHAR][1]);
    shuffle[i+flen] = i+flen;
    
    deltax += w + CHAR_SPACE;
  }
  nlen = flen + llen;  // Overall name length

  // Display the name and pause for it to be read
  display.clearDisplay();
  for(i=0;i<nlen;i++) {
    display.drawBitmap(
      nltrs[i][XPOS],nltrs[i][YPOS],
      &dejaVuSansMono_20ptBitmaps[nltrs[i][OFFSET]],
      8*nltrs[i][BWIDTH],
      CHAR_HEIGHT,WHITE);
  }
  display.display();
  delay(3500);  // See my name...
  
  // Shuffle using Fisher-Yates algorithm

  randomSeed(analogRead(0));
  for(i=nlen-1;i>0;i--){
    j = random(i+1);
    w = shuffle[i];
    shuffle[i] = shuffle[j];
    shuffle[j] = w;
  }
  
  // Loop and erase letters at random
  for(i=0;i<nlen;i++) {
      j = shuffle[i];
      display.drawBitmap(
        nltrs[j][XPOS],nltrs[j][YPOS],
        &dejaVuSansMono_20ptBitmaps[nltrs[j][OFFSET]],
        8*nltrs[j][BWIDTH],
        CHAR_HEIGHT,BLACK);
      display.display();  // update display so letter disappears
      delay(75); // pause after each letter vanishes
  }
  delay(500);  // Delay (screen will be blank)
}
  
