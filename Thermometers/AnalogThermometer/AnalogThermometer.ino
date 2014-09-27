/*
 * Simple thermometer for capturing room temperature info.  Designed
 * to use the TMP36 analog temperature sensor operating at 3.3V.  Displays
 * information on a 16x2 LCD.
 *
 * Author: David Bryant (david@orangemoose.com)
 * Version 2.0, 12-September-2012
 *
 * Arduino Pinouts (for 4-wire parallel interface to LCD)
 *    
 *    6 = PWM control for red backlight LED brightness -> LCD16
 *    7 = LCD RS -> LCD4
 *    8 = LCD EN -> LCD6
 *    9 = LCD DB4 (data) -> LCD11
 *   10 = LCD DB5 (data) -> LCD12
 *   11 = LCD DB6 (data) -> LCD13
 *   12 = LCD DB7 (data) -> LCD14
 *   13 = Blink on-board LED to indicate an update
 *
 * Additional LCD connections (valid for either with or without a shift register)
 *    LCD1 = GND
 *    LCD2 = VCC
 *    LCD3 = Contrast voltage (wiper of contrast pot)
 *    LCD5 = GND
 *    LCD15 = VCC
 *
 * We're operating the TMP36 at 3.3V to get more precise and less noisy results
 * so need to power it from the 3.3V supply pin on the Arduino.  Additionaly we
 * must tell the Arduino to use 3.3V as the reference voltage for all analog
 * inputs, which requires connecting the 3.3V supply pin to the AREF pin.
 * Adafruit has a great TMP36 tutorial at:
 * http://learn.adafruit.com/tmp36-temperature-sensor/using-a-temp-sensor
 *
 * For possible use by upstream applications and remote monitoring this sketch
 * produces a single line of output on the Serial port for each reported data value,
 * formatted as follows:
 *    {Sample #},{Temperature F},{Sensor Avg}
 *
 * where:
 *    {Sample #} = sequential sample number
 *    {Temperature F} = average Farenheit temperature across the calculation interval
 *                      as configured below, displayed as a floating point number
 *    {Sensor Avg} = average sensor reading across the same calculation interval,
 *                   displayed as an floating point number from 0.0 - 1023.0
 */
 
/*
 * Include the LCD library code. The standard Arduino 1.0 LCD library works fine but does not support
 * use of a shift register and so requires 6 control lines from the Arduino to the LCD.  That number
 * can be reduced to 3 control lines through use of a latching shift register but requires a different
 * LiquidCrystal library.  (This frees up 3 pins for other purposes.) The code here uses an
 * updated library from F.Malpartida et. al. at https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
 * which also has the benefit of improved performance relative to the standard Arduino 1.0 library. If you
 * want to use that enhanced library you'll need to download and install it, moving the standard 1.0 library
 * out of the way so it isn't picked up by the IDE.  The good news is that library is backward compatible
 * with the standard LCD library API so applications written for that library work unmodified with the
 * enhanced version.
 *
 * If using a shift register include LiquidCrystal_SR.h.  If using 4-wire parallel (or if you want to use
 * this script with the standard LCD library) include LiquidCrystal.h.
 */
#include <Wire.h>
#include <LiquidCrystal.h>

#define AREF_VOLTS 3.3   /* Reference voltage for TMP36 */

/*
 * If debugging, send extra output info via the Serial monitor though NOTE that
 * this will confuse any upstream processing of reported data, e.g. via Processing
 * applications on a remote host connected via Serial/USB.  With debugging off
 * we should only output one line per reported data value as described above. If
 * not debugging them comment out this #define
 */
#define DEBUG

const int sensorPin = 3;     /* Pin the TMP36 sensor output is connected to */
const int redLEDPin = 6;     /* Pin that controls red display on the LCD panel */
const int brightness = 100;  /* LCD display brightness, 0 = max, 255 = off. 100 works well */
const int LEDPin = 13;       /* Pin with LED to indicate update */

/*
 * Need to accumulate several pieces of data to calculate ongoing temperature
 */
const int sampleDelay = 5;     /* Interval in seconds at which temperature sensor is read */
const int captureDelay = 5;    /* Calculation interval in minutes for displaying temp */
unsigned long captureDelayMs;  /* Calculation interval as milliseconds (calculated) */
unsigned long prevMillis = 0;  /* Timestamp for measuring elapsed time */


float totalReading = 0;  /* Total sensor reading over capture interval */
int numReadings = 0;     /* Number of sensor readings over capture interval */
float avgReading;        /* Average sensor reading over capture interval*/
int numCaptures = 0;     /* Number of capture intervals observed */

float minTempF = 1000;   /* Observed minimum temperature */
float maxTempF = -1000;  /* Observed maximum temperature */

/* Initialize the LCD library with the numbers of the interface pins */
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);    /* if using 4-bit parallel interface */
// LiquidCrystal_SR lcd(7, 8, 9);          /* if using 3 wire shift register interface */

#define DEG_CHAR  1
#define ARR_CHAR  2

/* Define custom degree & arrow symbols */
/* The tool at http://www.quinapalus.com/hd44780udg.html is very useful for this... */
byte degree[8] = {
  0b01100,    /* .##.. */
  0b10010,    /* #..#. */
  0b10010,    /* #..#. */
  0b01100,    /* .##.. */
  0b00000,    /* ..... */
  0b00000,    /* ..... */
  0b00000,    /* ..... */
  0b00000     /* ..... */
};

byte arrow[8] = {
  0b00000,    /* ..... */
  0b00100,    /* ..#.. */
  0b00010,    /* ...#. */
  0b11111,    /* ##### */
  0b00010,    /* ...#. */
  0b00100,    /* ..#.. */
  0b00000,    /* ..... */
  0b00000,    /* ..... */
};

void setup()
{
  // Debugging via the Serial monitor
  Serial.begin(9600); 
  
  // Set the AREF to 3.3V (instead of the usual 5V)
  analogReference(EXTERNAL);
  
  // Enable output pin for LED
  pinMode(LEDPin,OUTPUT);
  
  // Create custom characters using LCD library
  lcd.createChar(DEG_CHAR,degree);
  lcd.createChar(ARR_CHAR,arrow);
  
  // Initialize LCD and set display brightness
  lcd.begin(16,2);
  analogWrite(redLEDPin,brightness);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Gathering data");
  lcd.setCursor(0,1);
  lcd.print("...stand by...");
  
  // Calculate capture delay parameters
  captureDelayMs = captureDelay * 60 * 1000L;   /* convert minutes to milliseconds */
  prevMillis = millis();
  
  // Output key operating parameters to help interpret overall data set
  Serial.print("Capture interval: "); Serial.print(captureDelay); Serial.print(" min. ");
  Serial.print("V-Ref: "); Serial.println(AREF_VOLTS); 
}
 
void loop()
{
  int reading;
  float voltage, temperatureC, temperatureF;
  
  /* Read temperature sensor */
  reading = analogRead(sensorPin);
  
  /* Accumluate readings so we can calculate average */
  totalReading += reading;
  numReadings ++;

  /* If debugging dump current data to Serial Monitor */
#if defined DEBUG

  /* Convert latest reading to voltage */
  voltage = (reading * AREF_VOLTS) / 1024.0;
  
  /* Calculate average reading so we can display it */
  avgReading = totalReading / numReadings;
   
  /* Convert voltage to temperature based on TMP36 calibration curve */
  temperatureC = (voltage - 0.5) * 100;  // 10mV per degree with 500mV offset
  temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  
  int displayTemp = (int) (temperatureC + 0.5);
  Serial.print(displayTemp); Serial.print(" deg C [");
  Serial.print(reading); Serial.print(" val, ");
  Serial.print(avgReading); Serial.print(" avg, ");
  Serial.print(temperatureC); Serial.print(" C, ");
  Serial.print(temperatureF); Serial.print(" F, (");
  Serial.print(voltage); Serial.println(" volts)]");
#endif
  
  unsigned long currentMillis = millis();
  /* Check and see if it is time to compute the running average temperature */
  if( (currentMillis - prevMillis) >= captureDelayMs) {
    
    // Calculate average reading and convert to temperature F
    avgReading = totalReading / numReadings;
    voltage = (avgReading * AREF_VOLTS) / 1024.0;
    temperatureC = (voltage - 0.5) * 100; // 10mV per degree with 500mV offset
    temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
    numCaptures++;
    
    /* If the first interval then initialize max & min temp */
    if(numCaptures == 1) {
        maxTempF = temperatureF;
        minTempF = temperatureF;
    }
    else {
      // Update max, min observed (average) temperatures
      if(temperatureF > maxTempF) { maxTempF = temperatureF; } /* New maximum? */
      if(temperatureF < minTempF) { minTempF = temperatureF; } /* New Minimum? */
    }
    
    Serial.print(numCaptures); Serial.print(","); Serial.print(temperatureF);
    Serial.print(","); Serial.println(avgReading);
    
    // Update display
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(temperatureF);
    lcd.write(DEG_CHAR);  // Custom degree symbol
    lcd.print("F");
    lcd.setCursor(0,1);
    lcd.print(minTempF);
    lcd.write(ARR_CHAR);  // Custom arrow character
    lcd.print(maxTempF);
    lcd.write(DEG_CHAR);  // Custom degree symbol
    
    // Handle capture number, roll over if greater than 0x999
    if(numCaptures > 0x999) numCaptures = 0;
    if(numCaptures < 0x10)       { lcd.print("   "); lcd.print(numCaptures,HEX); }
    else if(numCaptures < 0x100) { lcd.print("  "); lcd.print(numCaptures,HEX); }
    else                         { lcd.print(" "); lcd.print(numCaptures,HEX); }
    
    /* Flash LED */
    digitalWrite(LEDPin,HIGH); delay(50); digitalWrite(LEDPin,LOW);
    
    // Reset counters and accumulators
    prevMillis = currentMillis;
    totalReading = 0;
    numReadings = 0;
  }
  
  /* Delay base sample interval (in milliseconds) */
  delay(1000*sampleDelay);
}
