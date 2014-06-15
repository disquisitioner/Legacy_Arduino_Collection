/*
 * OLEDWeatherStation - Simple Arduino-based weather station
 *
 * Copyright (c) 2012 David Bryant <djbryant@gmail.com>
 *
 * Reads and reports temperature and barometric pressure from several attached
 * sensors.  This version is supports the Dallas Semiconductor DS18B20 digital
 * temperature sensor and the BMP085 combination temperature & pressure sensor.
 *
 * Modified from 'WeatherStation', replacing the 16x2 LCD display with a 128x64
 * OLED connected via SPI.  Several different display screens are generated on
 * the OLED, with a push button used to step among the screens.
 *
 * Author: David Bryant <djbryant@gmail.com>
 * Version 1.0, 1 May, 2013
 * License: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/ 
 *
 * The BMP085 code is based on an extended example by Jim Lindblom, SparkFun
 * Electronic, obtained from http://www.sparkfun.com/tutorials/253
 *
 * 
 * Arduino Pinouts (for 5-wire SPI connection to OLED)
 *   A4 = SDA for BMP085 (I2C Wire protocol)
 *   A5 = SCL for BMP085 (I2C Wire protocol)
 *    2 = Push button for cycling through the display screens. Wired to
 *        be pulled HIGH when not pressed.
 *    3 = DS18B20 temperature sensor (direct digital read)
 *    9 = MOSI -> OLED DATA
 *   10 = CLK  -> OLED CLK
 *   11 = DC  -> OLED SAO
 *   12 = CS  -> OLED CS
 *   13 = RESET -> OLED RST
 *
 * Additional OLED connections 
 *    OLED VIN -> VCC
 *    OLED Gnd -> GND
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


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Dallas Semiconductor DS18B20 requires Dallas Temperature Control Library, which uses the OneWire network
#include <OneWire.h>
#include <DallasTemperature.h>

// Include special fonts for OLED display
/* On Windows: */
#include "D:\Users\David\Documents\Dropbox\App Works\Arduino\OLEDWeatherStation\oledfonts.c"
/* On Mac:
 #include "/Users/dbryant/Dropbox/App Works/Arduino/OLEDWeatherStation/oledfonts.c"
 */

/*
 * If debugging, send extra output info via the Serial monitor though NOTE that
 * this will confuse any upstream processing of reported data, e.g. via Processing
 * applications on a remote host connected via Serial/USB.  With debugging off
 * we should only output one line per reported data value as described above.
 *
 * Uncomment the line below to enable debugging
 */
boolean debugging = false;
boolean haveDS18B20 = true; /* Set to 'false' during testing without sensors attached */
boolean haveBMP085  = true; /* Set to 'false' during testing without sensors attached */


/*
 * Support a variet of screens and keep track of the one currently displayed
 */
#define SCREEN_MAIN   0
#define SCREEN_MAX    1
#define SCREEN_GRAPH  2
#define SCREEN_STATUS 3
int currentScreen = SCREEN_MAIN;  /* Start with the main screen */

#define SENSOR_PIN  3     /* Pin the DS18B20 sensor output is connected to */
#define BUTTON_PIN  2     /* Pin the display control button is connected to */
int buttonState = HIGH;      /* Button is wired to be pulled HIGH when not pressed */

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
unsigned long sampleDelayMs;   /* Sample delay interval as milliseconds (calculated) */
unsigned long prevCaptureMs = 0;  /* Timestamp for measuring elapsed capture time */
unsigned long prevSampleMs  = 0;  /* Timestamp for measuring elapsed sample time */
unsigned long numReadings = 0;  /* Number of overall sensor readings over capture interval */
unsigned long numCaptures = 0; /* Number of capture intervals observed */

float DSreadingF = 0;  /* Total DS18B20 temperature F reading over capture interval */
float DScurrentTempF = 0;       /* Current reading from outdoor temperature sensor */
float DSavgTempF;            /* Average temperature as reported last capture interval */
float DSminTempF = 1000;   /* Observed minimum temperature */
float DSmaxTempF = -1000;  /* Observed maximum temperature */

float BMPreadingF = 0; /* Total BMP085 temperature F reading over capture interval */
float BMPreadingP = 0; /* Total BMP085 pressure reading over capture interval */
float BMPaverageF = 0; /* Average BMP temperature as reported last capture interval */
float BMPaverageP = 0; /* Average BMP pressure as reported last capture interval */


// Results from reading the temperature & pressure from the BMP085 sensor
short BMP085temp;
float BMP085tempF;
long BMP085press;
float BMP085pressHG;

/* 
 * Initialize the OneWire library, creating an instance to communicate with
 * any OneWire devices (not just the temperature sensor). Then initialize the
 * temperature sensor on that OneWire bus
 */
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

/* Initialize the OLED display */
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define OLED_DC 11
#define OLED_CS 12
#define OLED_CLK 10
#define OLED_MOSI 9
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

void setup()
{
  // Output via the Serial monitor
  Serial.begin(9600); 

  if(haveDS18B20) {
    sensors.begin();      // Start the DS18B20
  }
  if(haveBMP085) {  
    Wire.begin();         // Initialize the Wire protocol (I2C) for the BMP085
    initializeSensors();  // Initialize sensors (just the BMP085 needs this for now...)
  }
  randomSeed(analogRead(0));

  // Enable push button input pin
  pinMode(BUTTON_PIN,INPUT);

  // Initialize OLED
  // By default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done

  display.clearDisplay();
  display.setTextColor(WHITE);
  splashScreen();

  // Output key operating parameters to help interpret overall data set
  Serial.print(F("Sample: ")); 
  Serial.print(sampleDelay); 
  Serial.print(F(" sec, "));
  Serial.print(F("Capture: ")); 
  Serial.print(captureDelay); 
  Serial.println(F(" min."));

  // Calculate capture and storage parameters
  captureDelayMs = captureDelay * 60 * 1000L;   /* convert minutes to milliseconds */
  sampleDelayMs  = sampleDelay * 1000L;         /* converts seconds to milliseconds */
  /*
   * We're storing 96 points to show on our 128 pixel wide display as a plot of the last 24 hours
   * of data.  24 hours = 1560 minutes so 96 values means one every 15 minutes.  Need to figure
   * out how often in our sampling we need to store a value in order to get one every 15 minutes
   * (or as little over 15 minutes as possible).
   */
  // storeRate = (15 + captureDelay - 1)/captureDelay; 
  // Serial.print("Store rate: ");  Serial.println(storeRate); Serial.flush();
  prevCaptureMs = prevSampleMs = millis();
}

void loop()
{
  unsigned long currentMillis = millis();    // Check current timer value
  /* See if it is time to read the sensors */
  if( (currentMillis - prevSampleMs) >= sampleDelayMs) {
    numReadings ++;  // Since we started at 0...
    
    // Read temperature from DS18b20
    if(haveDS18B20) { DScurrentTempF = ds18b20GetTemperatureF(); }
    else {
      long randNumber;
      randNumber = random(-10,10);
      DScurrentTempF = 69 + (((float)randNumber)/10);
    }
    DSreadingF += DScurrentTempF;
  
    // Read values from BMP085
    if(haveBMP085) {
      BMP085temp = bmp085GetTemperature(bmp085ReadUT());
      BMP085tempF = (BMP085temp * 0.18) + 32.0;
      BMP085press = bmp085GetPressure(bmp085ReadUP());
      BMP085pressHG = BMP085press/3386.389;
    }
    else {
      BMP085tempF = 54.3 + (random(-20,20)/10.0);
      BMP085pressHG = 28.99 + (random(-10,10)/10.0);
    }
    BMPreadingF += BMP085tempF;
    BMPreadingP += BMP085pressHG;
    
    prevSampleMs = currentMillis;
    redrawDisplay();
  }

  /* Now check and see if it is time to compute the running average temperature */
  if( (currentMillis - prevCaptureMs) >= captureDelayMs) {
    numCaptures++;  // First, increment capture count (since it started at 0)

    // Calculate average readings for DS18b20 temperature, BMP085 temperature & pressure
    DSavgTempF  =  DSreadingF / numReadings;
    BMPaverageF = BMPreadingF / numReadings;
    BMPaverageP = BMPreadingP / numReadings;
    
    // Update max and min if necessary
    if(DSavgTempF > DSmaxTempF) {  DSmaxTempF = DSavgTempF; } /* New maximum? */
    if(DSavgTempF < DSminTempF) {  DSminTempF = DSavgTempF; } /* New Minimum? */

    // Report results by printing them out the Serial port...
    Serial.print(numCaptures); 
    Serial.print(F(",")); 
    Serial.print(DSavgTempF); 
    Serial.print(F(","));
    Serial.print(BMPaverageP); 
    Serial.print(F(",")); 
    Serial.println(BMPaverageF);

    /* Flash LED */
    // digitalWrite(LEDPin,HIGH); delay(50); digitalWrite(LEDPin,LOW);
    
    /* Update the display */
    redrawDisplay();

    // Reset counters and accumulators
    prevCaptureMs = currentMillis;
    DSreadingF = 0;
    BMPreadingF = 0;
    BMPreadingP = 0;
    numReadings = 0;
  }
  
  /* See if the push button was used to switch screens */
  buttonState = digitalRead(BUTTON_PIN);
  if(buttonState == LOW) {
    switch(currentScreen) {
      case SCREEN_MAIN:   currentScreen = SCREEN_MAX; break;
      case SCREEN_MAX:    currentScreen = SCREEN_GRAPH; break;
      case SCREEN_GRAPH:  currentScreen = SCREEN_STATUS; break;
      case SCREEN_STATUS: currentScreen = SCREEN_MAIN; break;
    }
    delay(100); // Debounce
    redrawDisplay();
  }
  
  /* Delay base sample interval (in milliseconds) */
  // delay(1000*sampleDelay);
}
// END of main loop()

/*
 * Display the current temperature sensor value, intended to be for the
 * outdoor sensor.  This should change every sampleDelay seconds and will give
 * the user the idea something is going on, both during the initial Sampling
 * interval and during regular ongoing operation.
 *
 * Note that the layout of the displayed current temperature is hardcoded here.
 */
void showTempSensorValue(float temp)
{
  switch(currentScreen) {
  case SCREEN_MAIN:
  case SCREEN_MAX:
    display.fillRect(96,0,32,8,BLACK);  // Erase previous displayed value 
    display.setCursor(96,0);
    display.print(temp);                        // Show current sensor value
    display.display();
    break;
  case SCREEN_GRAPH:
  case SCREEN_STATUS:
    break;
  }
}

/*
 * Update the display as appropriate for whatever screen is currently displayed
 * (or needs to be displayed in case the user just pressed the 'next screen' button.
 * There's lots of screen layout hard coded here, but ideally it is only here so it
 * is easy to tweak each of the screens.  (An exception to that is handling display
 * of the current outdoor temperature sensor value, for which layout is handled there
 * so it takes place in a separate, lighter weight way than redrawing the whole display
 * as is done here).
 */
void redrawDisplay()
{
  int i;
  
  /* If no first sample yet and not on the status screen just show a splash screen */
  if(numCaptures == 0 && currentScreen != SCREEN_STATUS) {
    splashScreen();
    display.display();
    return;
  }
  
  /* OK, so something to display...*/
  switch(currentScreen) {
    case SCREEN_MAIN:
      display.clearDisplay();
      showLargeTemp(0,0,DSavgTempF,"Outdoor");
      for(i=0;i<128;i+=2) display.drawPixel(i,39,WHITE);
      showMediumTemp(64,41,BMPaverageF,"Indoor");
      showMediumBP(0,41,BMPaverageP,"Pressure");
      // showTempSensorValue(DScurrentTempF);
      break;    
    case SCREEN_MAX:
      display.clearDisplay();
      showLargeTemp(0,0,DSavgTempF,"Outdoor");
      for(i=0;i<128;i+=2) display.drawPixel(i,39,WHITE);
      showMediumTemp( 0,41,DSminTempF,"Minimum");
      showMediumTemp(64,41,DSmaxTempF,"Maximum");
      // showTempSensorValue(DScurrentTempF);
      break;
    case SCREEN_GRAPH:
      display.clearDisplay();
      display.setCursor(16,32);
      display.print(F("Graph not done yet"));
      break;
    case SCREEN_STATUS:
      display.clearDisplay();
      display.setCursor(0,0);
      display.print(F("# Samples    : ")); 
      display.print(numCaptures);
      display.setCursor(0,10);
      display.print(F("# Readings   : "));
      display.print(numReadings);
      display.setCursor(0,20);
      display.print(F("Outdoor (Cur): ")); 
      display.print(DScurrentTempF);
      display.setCursor(0,30);
      display.print(F("Barometer    : ")); 
      display.println(BMP085pressHG);
      display.setCursor(0,40);
      display.print(F("Indoor  (Cur): ")); 
      display.println(BMP085tempF);
      break;
    }
  display.display();
}

/*
 * Display temperature in large font at (x,y) with a specific label
 */
void showLargeTemp(int x, int y, float tempF, char *label)
{
  int h, t, o, d;
  boolean showzero = false;
  boolean isneg;
  
  if(tempF < 0.0) {
      isneg = true;
      tempF = -1*tempF;
  }
  else {
    isneg = false;
  }
  tempF += 0.05;  /* We're rounding up to tenths of a degree */
  d = ((int) (tempF*10)) % 10;
  o = ((int) (tempF)) % 10;
  t = ((int) (tempF/10)) % 10;
  h = ((int) (tempF/100));
  if(h > 9) h = 9;
  
  display.setCursor(x,y);
  display.print(label);
  
  if(tempF >= 100.0) {
    display.drawBitmap(x,y+9,font_20x28_digits[h],24,28,WHITE);
    showzero = true;
  }
  else {
    if(isneg == true) display.drawBitmap(x,y+8,font_20x28_neg,24,28,WHITE);
  }
  if(t != 0 || showzero) {
      if(t == 1) display.drawBitmap(x+19,y+9,font_20x28_digits[t],24,28,WHITE);
      else       display.drawBitmap(x+15,y+9,font_20x28_digits[t],24,28,WHITE);
  }
  if(o == 1) display.drawBitmap(x+42,y+9,font_20x28_digits[o],24,28,WHITE);
  else       display.drawBitmap(x+38,y+9,font_20x28_digits[o],24,28,WHITE);
  display.drawBitmap(x+60,y+29,font_5x7_bigdpt,8,7,WHITE);
  display.drawBitmap(x+70,y+9,font_20x28_digits[d],24,28,WHITE);
  display.drawBitmap(x+92,y+9,font_5x7_deg,8,7,WHITE);
  display.drawBitmap(x+102,y+9,font_10x14_degf,16,14,WHITE);
}
 
/*
 * Display temperature in medium font at (x,y) with a specific label
 */
void showMediumTemp(int x, int y, float tempF, char *label)
{

  int h, t, o, d;
  boolean showzero = false;
  boolean isneg;

  if(tempF < 0.0) {
    isneg = true;
    tempF = -1*tempF;
  }
  else {
    isneg = false;
  }
  display.setCursor(x,y);
  display.print(label);
  
  tempF += 0.05;  /* We're rounding up to tenths of a degree */
  d = ((int) (tempF*10)) % 10;
  o = ((int) tempF) % 10;
  t = ((int) (tempF/10)) % 10;
  h = ((int) (tempF/100));
  if(h > 9) h = 9;

  if(tempF >= 100.0) {
    display.drawBitmap(x+2,y+9,font_10x14_digits[h],16,14,WHITE);
    showzero = true;
  }
  else {
    if(isneg == true) display.drawBitmap(x,y+9,font_10x14_neg,16,14,WHITE);
  }
  if(t != 0 || showzero) {
    if(t == 1) display.drawBitmap(x+14,y+9,font_10x14_digits[t],16,14,WHITE);
    else       display.drawBitmap(x+12,y+9,font_10x14_digits[t],16,14,WHITE);
  }
  if(o == 1) display.drawBitmap(x+26,y+9,font_10x14_digits[o],16,14,WHITE);
  else       display.drawBitmap(x+24,y+9,font_10x14_digits[o],16,14,WHITE);
  display.drawBitmap(x+36,y+15,font_5x7_dpt,8,7,WHITE);
  display.drawBitmap(x+44,y+9,font_10x14_digits[d],16,14,WHITE);
}

/* 
 ** Display barometric pressure in medium font at (x,y)
 ** with a string label
 */
void showMediumBP(int x, int y, float pressure,char *label)
{
  int t, o, d, m;

  pressure += 0.005;  // We're rounding to hundredths of an inch of mercury
  m = ((int) (pressure*100)) %10;
  d = ((int) (pressure*10)) % 10;
  o = ((int)  pressure) % 10;
  t = ((int) (pressure/10)) % 10;
  display.setCursor(x,y);
  display.print(label);
  display.drawBitmap( x,y+9,font_10x14_digits[t],16,14,WHITE);
  display.drawBitmap(x+12,y+9,font_10x14_digits[o],16,14,WHITE);
  display.drawBitmap(x+24,y+15,font_5x7_dpt,8,7,WHITE);
  display.drawBitmap(x+34,y+9,font_10x14_digits[d],16,14,WHITE);
  display.drawBitmap(x+46,y+9,font_10x14_digits[m],16,14,WHITE);
}



// Show something when there's nothing (yet) to show
void splashScreen()
{
  display.clearDisplay();
  display.setCursor(16,24);   
  display.print(F("Sampling..."));
  display.setCursor(16,40);   
  display.print(F("Please wait..."));
  display.display();
}

void showTemp(int x, int y, int t)
{

  display.setCursor(x,y);
  if(t >= 100) {
    display.print(t);
  }
  else {
    if(t < 0) {
      display.print('-');
      t *= -1;
    }
    else display.print(' ');
    if(t < 10) display.print(' ');
    display.print(t);
  }
}





// ********* BELOW HERE IS SENSOR RELATED CODE (initialization, I/O, etc.) *****

// Single routine called to initialize all our sensors
void initializeSensors()
{
  bmp085Calibration();
}

// Read the temperature F from the DS18B20 sensor
float ds18b20GetTemperatureF()
{  
  // Issue a global temperature request to all devices on the I2C bus
  sensors.requestTemperatures();

  // Now read temperature from DS18B20 sensor #1 (device #0)
  return(sensors.getTempFByIndex(0));
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

