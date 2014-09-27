/*
 * SimpleThermometer
 *
 * Simple thermometer for capturing room temperature info.  This version uses
 * a TMP36 analog temperature sensor.  Largely because of variability in the
 * readings from the TMP36 the program reads the sensor every second and
 * averages values over a period of 5 minutes (which is configurable).
 *
 * Readings are displayed on an LCD panel, and are also output via the
 * Serial port for possible upstream processing.
 * 
 * To reduce variability in sensor readings the TMP36 is operated at
 * 3.3V rather than 5V, so readings are scaled to the range 0->3.3V
 *
 * Author: David Bryant (david@orangemoose.com)
 * Version 1.0, 12-August-2012
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
 * Additional LCD connections
 *    LCD1 = GND
 *    LCD2 = VCC
 *    LCD3 = Contrast voltage (wiper of contrast pot)
 *    LCD5 = GND
 *    LCD15 = VCC
 *
 * For possible use by upstream applications and remote monitoring this application
 * produces a single line of output on the Serial port for each reported data value,
 * formatted as follows:
 *    {Sample #},{Temperature F},{Sensor Avg}
 *
 * where:
 *    {Sample #} = sequential sample number displayed in HEX
 *    {Temperature F} = average Farenheit temperature across the calculation interval
 *                      as configured below, displayed as a floating point number
 *    {Sensor Avg} = average sensor reading across the same calculation interval,
 *                   displayed as an floating point number from 0.0 - 1023.0
 */
 
#include <LiquidCrystal.h>

#define AREF_VOLTS 3.3   /* Reference voltage for TMP36 */

/*
 * If debugging, send extra output info via the Serial monitor though NOTE that
 * this will confuse any upstream processing of reported data, e.g. via Processing
 * applications on a remote host connected via Serial/USB.  With debugging off
 * we should only output one line per reported data value as described above.
 */
boolean debugging = false;

int sensorPin = 3;     /* Pin the TMP36 sensor output is connected to */
int redLEDPin = 6;     /* Pin that controls red display on the LCD panel */
int brightness = 100;  /* LCD display brightness, 0 = max, 255 = off. 100 works well */
int LEDPin = 13;       /* Pin with LED to indicate update */

/*
 * Need to accumulate several pieces of data to calculate ongoing temperature
 */
int captureDelay = 5;          /* Calculation interval in minutes for displaying temp */
unsigned long captureDelayMs;  /* Calculation interval as milliseconds (calculated) */
unsigned long prevMillis = 0;  /* Timestamp for measuring elapsed time */

float totalReading = 0;  /* Total sensor reading over capture interval */
int numReadings = 0;     /* Number of sensor readings over capture interval */
float avgReading;        /* Average sensor reading over capture interval*/
int numCaptures = 0;     /* Number of capture intervals observed */
float minTempF = 1000;   /* Observed minimum temperature */
float maxTempF = 0;      /* Observed maximum temperature */

/* Initialize the LCD library with the numbers of the interface pins */
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);    /* if using 4-bit parallel interface */

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
  
  // To set the AREF to something other than 5V use the line below
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
  int reading, displayTemp;
  float voltage, temperatureC, temperatureF;

  
  /* Read temperature sensor */
  reading = analogRead(sensorPin);
  totalReading += reading;
  numReadings ++;
  avgReading = totalReading / numReadings;
 
  if(debugging) {
    /* Convert reading to voltage */
    voltage = (reading * AREF_VOLTS) / 1024.0;
     
    /* Convert to temperature */
    temperatureC = (voltage - 0.5) * 100;  // 10mV per degree with 500mV offset
    temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
    displayTemp = (int) (temperatureC + 0.5);
    
    Serial.print(displayTemp); Serial.print(" deg C [");
    Serial.print(reading); Serial.print(" val, ");
    Serial.print(avgReading); Serial.print(" avg, ");
    Serial.print(temperatureC); Serial.print(" C, ");
    Serial.print(temperatureF); Serial.print(" F, (");
    Serial.print(voltage); Serial.println(" volts)]");
  }
  
  unsigned long currentMillis = millis();
  /* Check and see if it is time to compute the running average temperature */
  if( (currentMillis - prevMillis) >= captureDelayMs) {
    
    // Convert average reading to temperature F
    voltage = (avgReading * AREF_VOLTS) / 1024.0;
    temperatureC = (voltage - 0.5) * 100; // 10mV per degree with 500mV offset
    temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
    displayTemp = (int) (temperatureC + 0.5);
    numCaptures++;
    
    /* If the first interval then initialize max & min temp */
    if(numCaptures == 1) {
        maxTempF = temperatureF;
        minTempF = temperatureF;
    }
    else {
      // Update maximum and minimum temperatures if necessary
      if(temperatureF > maxTempF) { maxTempF = temperatureF;  /* New maximum? */ }
      if(temperatureF < minTempF) { minTempF = temperatureF;  /* New minimum? */ }
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
    
    /* Flash LED */
    digitalWrite(LEDPin,HIGH); delay(50); digitalWrite(LEDPin,LOW);
    
    // Reset counters and accumulators
    prevMillis = currentMillis;
    totalReading = 0;
    numReadings = 0;
  }
  
  /* Delay one second (base sample interval) */
  delay(1000);
}
