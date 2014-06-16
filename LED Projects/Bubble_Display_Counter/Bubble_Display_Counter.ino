/*
 * Based on 'Bubble_Display_Counter' from Spark Fun Electronics by Nathan Seidle and Joel Bartle,
 * which uses Dean Reading's SevenSeg library for driving seven segment displays.
 *
 * This sketch provides a simple counter example for the HP QDSP-6064 7-segment four digit
 * display ( https://www.sparkfun.com/products/12710), though it could certainly be used for
 * other similar displays.
 *
 * Author: David Bryant (djbryant@gmail.com)
 * Version 1.1, 17-August-2012
 */

/*
 Pinout for HP Bubble Display:
 1:  Cathode 1
 2:  Anode E
 3:  Anode C
 4:  Cathode 3
 5:  Anode dp
 6:  Cathode 4
 7:  Anode G
 8:  Anode D
 9:  Anode F
 10: Cathode 2
 11: Anode B
 12: Anode A
 */

#include "SevSeg.h"

//Create an instance of the object.
SevSeg myDisplay;

//Create global variables
unsigned long timer;
int deciSecond = 0;

void setup()
{

  int displayType = COMMON_CATHODE; //Your display is either common cathode or common anode

  
       //This pinout is for a bubble display
       //Declare what pins are connected to the GND pins (cathodes)
       int digit1 = 12; //  8; //Pin 1
       int digit2 = 13; //  5; //Pin 10
       int digit3 = 11; // 11; //Pin 4
       int digit4 = 10; // 13; //Pin 6
       
       //Declare what pins are connected to the segments (anodes)
       int segA = 6; //Pin 12  (seg a)
       int segB = 5; //Pin 11  (seg b)
       int segC = 8; //Pin 3   (seg c)
       int segD = 3; //Pin 8   (seg d)
       int segE = 7; //Pin 2   (seg e)
       int segF = 4; //Pin 9   (seg f)
       int segG = 2; //Pin 7   (seg g)
       int segDP= 9; //Pin 5   (dec.pt.)
   
  int numberOfDigits = 4; //Do you have a 1, 2 or 4 digit display?

  myDisplay.Begin(displayType, numberOfDigits, digit1, digit2, digit3, digit4, segA, segB, segC, segD, segE, segF, segG, segDP);
  
  myDisplay.SetBrightness(100); //Set the display to 100% brightness level

  timer = millis();
}

void loop()
{
  //Example ways of displaying a decimal number
  char tempString[10]; //Used for sprintf
  sprintf(tempString, "%4d", deciSecond); //Convert deciSecond into a string that is right adjusted
  //sprintf(tempString, "%d", deciSecond); //Convert deciSecond into a string that is left adjusted
  //sprintf(tempString, "%04d", deciSecond); //Convert deciSecond into a string with leading zeros
  //sprintf(tempString, "%4d", deciSecond * -1); //Shows a negative sign infront of right adjusted number
  //sprintf(tempString, "%4X", deciSecond); //Count in HEX, right adjusted

  //Produce an output on the display
  myDisplay.DisplayString(tempString, 0); //(numberToDisplay, decimal point location)

  //Other examples
  //myDisplay.DisplayString(tempString, 0); //Display string, no decimal point
  //myDisplay.DisplayString("-23b", 3); //Display string, decimal point in third position

  //Check if 10ms has elapsed
  if (millis() - timer >= 100)
  {
    timer = millis();
    deciSecond++;
  }

  delay(5);
}

