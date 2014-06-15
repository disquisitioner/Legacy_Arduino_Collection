/*
 * WeatherStation - Simple Arduino-based weather station
 *
 * Copyright (c) 2012 David Bryant <djbryant@gmail.com>
 *
 * Reads and reports temperature and barometric pressure from several attached
 * sensors.  This version is supports the Dallas Semiconductor DS18B20 digital
 * temperature sensor and the BMP085 combination temperature & pressure sensor.
 *
 * Author: David Bryant <djbryant@gmail.com>
 * Version 1.0, 25 August, 2012
 * License: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/ 
 *
 * The BMP085 code is based on an extended example by Jim Lindblom, SparkFun
 * Electronic, obtained from http://www.sparkfun.com/tutorials/253
 *
 * 
 * Arduino Pinouts (for 4-wire parallel interface to LCD)
 *   A4 = SDA for BMP085 (I2C Wire protocol)
 *   A5 = SCL for BMP085 (I2C Wire protocol)
 *    3 = DS18B20 temperature sensor
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
 * For possible use by upstream applications and remote monitoring this application
 * produces a single line of output on the Serial port for each reported data value,
 * formatted as follows:
 *    {Sample #},{DS18B20 Temp F},{DS18B20 Avg Temp F}, {BMP085 Pressure}, {BMP085 Temp F}
 *
 * where:
 *    {Sample #} = sequential sample number, in HEX
 *    {DS18B20 Temp F}     = average Farenheit temperature from DS18B20 sensor across
 *                           the collection interval, as a floating point number
 *    {DS18B20 Avg Temp F} = average temperatures from DS18B20 across collection interval
 *    {BMP085 Pressure}    = barometric pressure from BMP085, in inches of mercury
 *    {BMP085 Temp F}      = Farenheit temperature from BMP085, as a floating point number
 */
 
  
/*
 * Include the LCD library code. The standard Arduino 1.0 LCD library works fine but does not support
 * use of a shift register and so requires 6 control lines from the Arduino to the LCD.  That number
 * can be reduced to 3 control lines through use of a latching shift register but requires a different
 * LiquidCrystal library.  (This frees up 3 pins for other purposes.) The code here uses an
 * updated library from F.Malpartida et. al. at https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
 * which also has the benefit of improved performance relative to the standard Arduino 1.0 library.
 *
 * If using a shift register include LiquidCrystal_SR.h.  If using 4-wire parallel include LiquidCrystal.h
 */
#include <Wire.h>
#include <LiquidCrystal.h>

// Dallas Semiconductor DS18B20 requires Dallas Temperature Control Library, which uses the OneWire network
#include <OneWire.h>
#include <DallasTemperature.h>

/*
 * If debugging, send extra output info via the Serial monitor though NOTE that
 * this will confuse any upstream processing of reported data, e.g. via Processing
 * applications on a remote host connected via Serial/USB.  With debugging off
 * we should only output one line per reported data value as described above.
 */
boolean debugging = false;

int sensorPin = 3;     /* Pin the DS18B20 sensor output is connected to */
int redLEDPin = 6;     /* Pin that controls red display on the LCD panel */
int brightness = 100;  /* LCD display brightness, 0 = max, 255 = off. 100 works well */
int LEDPin = 13;       /* Pin with LED to indicate update */

// Configuration info for the BMP085 sensor
#define BMP085_ADDRESS 0x77  // I2C address of BMP085
const unsigned char OSS = 0;  // Oversampling Setting

// BMP085 Calibration values, which are stored in each sensor's EEPROM
int ac1;
int ac2; 
int ac3; 
unsigned int ac4;
unsigned int ac5;
unsigned int ac6;
int b1; 
int b2;
int mb;
int mc;
int md;

// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 
// END of BMP085 configuration info

/*
 * Need to accumulate several pieces of data to calculate ongoing temperature
 *
 * In a moving vehicle good values are a sample delay of 2 seconds and a capture
 * delay (reporting inverval) of 2 minutes.  At a stationary location such as a home
 * weather station good values are a sample delay of 5 seconds and a capture delay
 * of 5 minutes.
 *
 */
int sampleDelay = 5;           /* Interval at which temperature sensor is read (in seconds) */
int captureDelay = 5;          /* Interval at which samples are averaged & reported (in minutes) */
unsigned long captureDelayMs;  /* Calculation interval as milliseconds (calculated) */
unsigned long prevMillis = 0;  /* Timestamp for measuring elapsed time */


float DStotalReadingF = 0;  /* Total DS18B20 temperature F reading over capture interval */
float BMPtotalReadingF = 0; /* Total BMP085 temperature F reading over capture interval */
float BMPtotalReadingP = 0; /* Total BMP085 temperature F reading over capture interval */
int numReadings = 0;        /* Number of overall sensor readings over capture interval */
float DSavgReadingF;        /* Average DS18B20 temperature F reading over capture interval*/
float BMPavgReadingF;       /* Average BMP085 temperature F reading over capture interval */
float BMPavgReadingP;       /* Average BMP085 pressure reading over capture interval */
int numCaptures = 0;        /* Number of capture intervals observed */

float DSminTempF = 1000;   /* Observed minimum temperature */
float DSmaxTempF = -1000;  /* Observed maximum temperature */

// Results from reading the temperature & pressure from the BMP085
short BMP085temp;
float BMP085tempF;
long BMP085press;
float BMP085pressHG;

/* 
 * Initialize the OneWire library, creating an instance to communicate with
 * any OneWire devices (not just the temperature sensor). Then initialize the
 * temperature sensor on that OneWire bus
 */
OneWire oneWire(sensorPin);
DallasTemperature sensors(&oneWire);
 
/* Initialize the LCD library with the numbers of the interface pins */
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);    /* if using 4-bit parallel interface */
// LiquidCrystal_SR lcd(7, 8, 9);          /* if using 3 wire shift register interface */

/* Define custom degree & arrow symbols */
/* The tool at http://www.quinapalus.com/hd44780udg.html is very useful for this... */
#define DEG_CHAR  1
#define ARR_CHAR  2

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
  // Output via the Serial monitor
  Serial.begin(9600); 
  
  // Start the DS18B20
  sensors.begin();
  
  // Initialize the Wire protocol (I2C)
  Wire.begin();
  
  // Initialize sensors
  initializeSensors();
  
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
  lcd.print("--.-"); lcd.write(DEG_CHAR); lcd.print("F");
  lcd.setCursor(0,1);
  lcd.print("Sampling...");

  // Calculate capture delay parameters
  captureDelayMs = captureDelay * 60 * 1000L;   /* convert minutes to milliseconds */
  prevMillis = millis();
  
  // Output key operating parameters to help interpret overall data set
  Serial.print("Sample interval: "); Serial.print(sampleDelay); Serial.print(" sec, ");
  Serial.print("Capture interval: "); Serial.print(captureDelay); Serial.println(" min.");
}
 
void loop()
{
  float DStemperatureC, DStemperatureF;
  float DScurrentTempC, DScurrentTempF;
  
  numReadings ++;  // Since we started at 0...
  
  // Read temperature from DS18b20
  DScurrentTempF = ds18b20GetTemperatureF();
  DStotalReadingF += DScurrentTempF;
  DSavgReadingF = DStotalReadingF / numReadings;

  // Read values from BMP085
  BMP085temp = bmp085GetTemperature(bmp085ReadUT());
  BMP085tempF = (BMP085temp * 0.18) + 32.0;
  BMP085press = bmp085GetPressure(bmp085ReadUP());
  BMP085pressHG = BMP085press/3386.389;
  BMPtotalReadingF += BMP085tempF;
  BMPtotalReadingP += BMP085pressHG;

  lcd.setCursor(8,0);       // Move to area reserved for current sensor value
  lcd.print("(");
  lcdprint1f(DScurrentTempF);     // lcd.print() but with 1 decimal digit
  lcd.write(DEG_CHAR);
  lcd.print(") ");
  
  if(debugging) {
    Serial.print("DS18B20 #1: ");
    Serial.print(DScurrentTempC); Serial.print(" C, ");
    Serial.print(DScurrentTempF); Serial.print(" F, (");
    Serial.print(DSavgReadingF); Serial.println(" avg)");
    
    Serial.print("BMP085 Temperature: ");
    Serial.print(BMP085tempF); Serial.print("F, [read ");
    Serial.print(BMP085temp, DEC);
    Serial.println(" *0.1 deg C]");
    Serial.print("BMP085 Pressure: ");
    Serial.print(BMP085pressHG); Serial.print("\" HG, [read ");
    Serial.print(BMP085press, DEC);
    Serial.println(" Pascals]");
    Serial.println();
  }
  
  unsigned long currentMillis = millis();
  /* Check and see if it is time to compute the running average temperature */
  if( (currentMillis - prevMillis) >= captureDelayMs) {
    
    numCaptures++;  // First, increment capture count (since it started at 0)
    
    /* If the first interval then initialize max & min temp for DS18B20 */
    if(numCaptures == 1) {
        DSmaxTempF = DSavgReadingF;
        DSminTempF = DSavgReadingF;
    }
    else {
      // Update max and min if necessary
      if(DSavgReadingF > DSmaxTempF) { DSmaxTempF = DSavgReadingF; } /* New maximum? */
      if(DSavgReadingF < DSminTempF) { DSminTempF = DSavgReadingF; } /* New Minimum? */
    }
    
    // Calculate average readings for BMP085 temperature & pressure
    BMPavgReadingF = BMPtotalReadingF / numReadings;
    BMPavgReadingP = BMPtotalReadingP / numReadings;
    
    // Report results by printing them out the Serial port...
    Serial.print(numCaptures); Serial.print(","); Serial.print(DSavgReadingF); Serial.print(",");
    Serial.print(BMPavgReadingP); Serial.print(","); Serial.println(BMPavgReadingF);
    
    // Update display
    lcd.clear();
    lcd.setCursor(0,0);       // Start at top left
    lcdprint1f(DSavgReadingF);  // lcd.print but only 1 decimal digit
    lcd.write(DEG_CHAR);      // Custom degree symbol
    lcd.print("F");
    
    lcd.setCursor(8,0);       // Move to area reserved for current sensor value
    lcd.print("(");
    lcdprint1f(DSavgReadingF);     // lcd.print but only 1 decimal digit
    lcd.write(DEG_CHAR);
    lcd.print(")");
   
    lcd.setCursor(0,1);       // Move to bottom left (2nd row)
    lcdprint1f(DSminTempF);     // lcd.print but only 1 decimal digit
    lcd.write(ARR_CHAR);      // Custom arrow character
    lcdprint1f(DSmaxTempF);     // lcd.print but only 1 decimal digit
    lcd.write(DEG_CHAR);      // Custom degree symbol
    
    // Diplay number of captures in hex, roll over if greater than 0x999
    if(numCaptures > 0x999) numCaptures = 0;
    if(numCaptures < 0x10)       { lcd.setCursor(15,1); lcd.print(numCaptures,HEX); }
    else if(numCaptures < 0x100) { lcd.setCursor(14,1); lcd.print(numCaptures,HEX); }
    else                         { lcd.setCursor(13,1); lcd.print(numCaptures,HEX); }
    
    /* Flash LED */
    digitalWrite(LEDPin,HIGH); delay(50); digitalWrite(LEDPin,LOW);
    
    // Reset counters and accumulators
    prevMillis = currentMillis;
    DStotalReadingF = 0;
    BMPtotalReadingF = 0;
    BMPtotalReadingP = 0;
    numReadings = 0;
  }
  
  /* Delay base sample interval (in milliseconds) */
  delay(1000*sampleDelay);
}
// END of main loop()


// Read the temperature F from the DS18B20 sensor
float ds18b20GetTemperatureF()
{  
  // Issue a global temperature request to all devices on the I2C bus
  sensors.requestTemperatures();
  
  // Now read temperature from DS18B20 sensor #1 (device #0)
  return(sensors.getTempFByIndex(0));
}

// Single routine called to initialize all our sensors
void initializeSensors()
{
    bmp085Calibration();
}

// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  ac1 = bmp085ReadInt(0xAA);
  ac2 = bmp085ReadInt(0xAC);
  ac3 = bmp085ReadInt(0xAE);
  ac4 = bmp085ReadInt(0xB0);
  ac5 = bmp085ReadInt(0xB2);
  ac6 = bmp085ReadInt(0xB4);
  b1 = bmp085ReadInt(0xB6);
  b2 = bmp085ReadInt(0xB8);
  mb = bmp085ReadInt(0xBA);
  mc = bmp085ReadInt(0xBC);
  md = bmp085ReadInt(0xBE);
}

// Calculate temperature given uncompensated temperature ('ut').
// Value returned will be in units of 0.1 deg C
short bmp085GetTemperature(unsigned int ut)
{
  long x1, x2;
  
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return ((b5 + 8)>>4);  
}

// Calculate pressure given uncompensated pressure ('up')
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up)
{
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;
  
  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;
  
  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
  
  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;
    
  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;
  
  return p;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char data;
  
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  
  Wire.requestFrom(BMP085_ADDRESS, 1);
  while(!Wire.available())
    ;
    
  return Wire.read();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  unsigned char msb, lsb;
  
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  
  Wire.requestFrom(BMP085_ADDRESS, 2);
  while(Wire.available()<2)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  
  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value ('ut')
unsigned int bmp085ReadUT()
{
  unsigned int ut;
  
  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();
  
  // Wait at least 4.5ms
  delay(5);
  
  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value ('up')
unsigned long bmp085ReadUP()
{
  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;
  
  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();
  
  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));
  
  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF6);
  Wire.endTransmission();
  Wire.requestFrom(BMP085_ADDRESS, 3);
  
  // Wait for data to become available
  while(Wire.available() < 3)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  xlsb = Wire.read();
  
  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);
  
  return up;
}

/*
 * lcd.print() displays floating point values with two decimal digits, which is
 * overkill for our small 16x2 LCD display.  This function results in the displayed
 * value just having one decimal digit.
 */
void lcdprint1f(float num)
{
  int base, whole, frac;
  
  base = (int) ((num*10) + 0.5);
  whole = (int) (base / 10);
  frac = base % 10;
  lcd.print(whole); lcd.print("."); lcd.print(frac);
}
