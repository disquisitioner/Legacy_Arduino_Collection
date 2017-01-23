/*
 * Home Weather Station -  Teensy 3.1-based weather station
 *
 * Copyright (c) 2015 David Bryant <david@orangemoose.com>
 *
 * Reads and reports temperature and barometric pressure from several attached
 * sensors.  This version supports the Dallas Semiconductor DS18B20 digital
 * temperature sensor and the BMP180 combination temperature & pressure sensor.
 *
 * Reads wind direction/speed and rainfall using a set of matched sensors from
 * Sparkfun.
 *
 * Provides information for the user via a 128x64 OLED connected via SPI.  
 * Several different display screens are generated on the OLED, with a push 
 * button used to step among the screens.
 *
 * Compatible with the custom printed circuit board I've designed for these
 * components.
 *
 * Author: David Bryant <david@orangemoose.com>
 * Version 1.0, 8 June 2015
 * License: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/ 
 *
 * 
 * Teensy 3.1 Pinouts (for 5-wire SPI connection to OLED)
 *   A1 = Input for Wind Vane to read it as resistor network
 *   A4 = SDA for BMP180 (I2C Wire protocol)
 *   A5 = SCL for BMP180 (I2C Wire protocol)
 *    2 = Anemometer input
 *    3 = Rain Gauge input
 *    4 = Input for push button to cycle through screens (interrupt mode)
 *    5 = DS18B20 temperature sensor (direct digital read)
 *    8 = CS    -> OLED CS
 *    9 = RESET -> OLED RST
 *   10 = DC    -> OLED SAO
 *   11 = CLK   -> OLED CLK
 *   12 = MOSI  -> OLED DATA
 *
 *    8 = MOSI -> OLED DATA
 *    9 = CLK  -> OLED CLK
 *   10 = DC  -> OLED SAO
 *   11 = CS  -> OLED CS
 *   12 = RESET -> OLED RST
 
 *   13 = Onboard LED (optional)
 *
 * Additional connections 
 *    PC Board 5V0 -> VCC (5V)
 *    PC Board Gnd -> GND
 *    PC Board 3V3 -> 3.3V
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

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Dallas Semiconductor DS18B20 requires Dallas Temperature Control Library, which uses the OneWire network
#include <OneWire.h>
#include <DallasTemperature.h>

// Include special fonts for OLED display, which are packaged as part of this sketch (project)
#include "./oledfonts.c"

/* Development & debugging tools */
const boolean haveDS18B20  = true; /* Set to 'false' during testing without DS18B20 attached */
const boolean haveBMP180   = true; /* Set to 'false' during testing without BMP180 attached */
const boolean haveWindRain = true; /* Set to 'false' during testing without wind/rain sensors */

// *********************  BMP180 CONFIGURATION *********************
#define BMP180_ADDRESS          0x77  // I2C address of BMP180
#define BMP180_WHO_AM_I         0xD0  // WHO_AM_I id of BMP180, should return 0x55
#define BMP180_RESET            0xE0
#define BMP180_CONTROL          0xF4
#define BMP180_OUT_MSB          0xF6
#define BMP180_OUT_LSB          0xF7
#define BMP180_OUT_XLSB         0xF8

enum OSS {  // BMP-180 sampling rate
  OSS_0 = 0,  // 4.5 ms conversion time
  OSS_1,      // 7.5
  OSS_2,      // 13.5
  OSS_3       // 25.5
};

// Specify sensor parameters
uint8_t OSS = OSS_3;           // maximum pressure resolution

// These are constants used to calulate the temperature and pressure from the BMP-180 sensor
int16_t ac1, ac2, ac3, b1, b2, mb, mc, md, b5;  
uint16_t ac4, ac5, ac6;
float temperature, pressure, temppress, altitude;

// SFE_BMP180 pressure;
double BMPtempC, BMPpresshPa;      // Outputs temperature in degrees C, pressure in hectopascals
boolean BMPtempError = false;      // If we encounter errors reading the BMP180
boolean BMPpressError = false;

/* ********************  DS18B20 CONFIGURATION *********************
 * Initialize the OneWire library, creating an instance to communicate with
 * any OneWire devices (not just the temperature sensor). Then initialize the
 * temperature sensor on that OneWire bus
 */
#define DS18B20_PIN  5     /* Pin the DS18B20 sensor output is connected to */
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);


// **********************  OLED CONFIGURATION **********************
/* Initialize the OLED display */
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define OLED_MOSI 12
#define OLED_CLK  11
#define OLED_DC   10
#define OLED_CS    8
#define OLED_RESET 9
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define SW_BUTTON 4

// *****************  WIND/RAIN SENSOR CONFIGURATION *****************
#define ANEMOMETER_PIN   2  // Digital pin 2, interrupts indicate anemometer rotation
#define RAINGAUGE_PIN    3  // Digital pin 3, interrupts indicate rainfall
#define WINDVANE_PIN     1  // Wind vane connected to analog pin 1

#define WIND_FACTOR 1.492  // One anemometor rotation in one second = 1.492 MPH
#define RAIN_FACTOR 0.011  // One rain gauge measurement = 0.011" of rain
#define WINDVANE_OFFSET  0.0 // Compensate for wind vane's physical direction 
                             // (if needed)
                             
volatile int anemometerCnt = 0;  // Counts anemometer interrupts
volatile int raingaugeCnt  = 0;  // Counts rain gauge interrupts
volatile unsigned long anemom_last = 0;
volatile unsigned long anemom_min  = 0xffffffff;
float aggregateWindDir = 0;
float windSpeed = 0;                    // Wind speed (avg over reporting period)
float windGust  = 0;                    // Fastest winds speed seen this period
float rainFall = 0;
float windDirection = 0;

// Onboard LED 
#define LED_PIN 13
boolean LEDstate = false;

/*
 * Support a variety of screens and keep track of the one currently displayed
 */
#define SCREEN_MAIN   0
#define SCREEN_MAX    1
#define SCREEN_STATUS 2
#define SCREEN_WNDRN  3
#define SCREEN_SPLASH 4
byte currentScreen = SCREEN_SPLASH;  /* Initialize to starting screen */
volatile boolean newScreen = false;

#define MAXUNSIGNEDLONG 4294967295  // 2^31 - 1 on Arduino
#define MAXREPORTS 9999      // Upper limit on report count, rolls over to 0


/*
 * Need to accumulate several pieces of data to calculate ongoing temperature
 *
 * In a moving vehicle good values are a sample delay of 2 seconds and a capture
 * delay (reporting inverval) of 2 minutes.  At a stationary location such as a home
 * weather station good values are a sample delay of 5 seconds and a capture delay
 * of 5 minutes.
 *
 */
const int sampleDelay = 5;           /* Interval at which temperature sensor is read (in seconds) */
const int reportDelay = 5;          /* Interval at which samples are averaged & reported (in minutes) */
const unsigned long reportDelayMs = reportDelay * 60 * 1000L;  /* Calculation interval */
const unsigned long sampleDelayMs = sampleDelay * 1000L;       /* Sample delay interval */
unsigned long prevReportMs = 0;  /* Timestamp for measuring elapsed capture time */
unsigned long prevSampleMs  = 0;  /* Timestamp for measuring elapsed sample time */
unsigned int numSamples = 0;  /* Number of overall sensor readings over reporting interval */
unsigned int numReports = 0; /* Number of capture intervals observed */

float DSreadingF = 0;  /* Total DS18B20 temperature F reading over capture interval */
float DScurrentTempF = 0; /* Current reading from outdoor temperature sensor */
float DSavgTempF;         /* Average temperature as reported last capture interval */
float DSminTempF = 199;   /* Observed minimum temperature */
float DSmaxTempF = -99;   /* Observed maximum temperature */

float BMPreadingF = 0; /* Total BMP180 temperature F reading over capture interval */
float BMPreadingP = 0; /* Total BMP180 pressure reading over capture interval */
float BMPaverageF = 0; /* Average BMP temperature as reported last capture interval */
float BMPaverageP = 0; /* Average BMP pressure as reported last capture interval */


void setup()
{
  
  Wire.begin();
    
  // Output via the Serial monitor
  Serial.begin(9600); 
  delay(5000);  // To allow Serial to open...
  
  randomSeed(analogRead(0));
  
  // Enable pull-up resistors on Anemometer, Rain Gauge, and screen control button pins
  pinMode(ANEMOMETER_PIN,INPUT_PULLUP);
  // digitalWrite(ANEMOMETER_PIN,HIGH);
  pinMode(RAINGAUGE_PIN,INPUT_PULLUP);
  // digitalWrite(RAINGAUGE_PIN,HIGH);
  pinMode(SW_BUTTON,INPUT_PULLUP);
  
  // Single entry point to set up all our sensors
  initialize_sensors();
  
  // Generalized entry point for whatever is needed to configure our interrupts
  initialize_interrupts();

  // Initialize OLED
  display.begin();

  display.clearDisplay();
  display.setTextColor(WHITE);

  // Output key operating parameters to help interpret overall data set
  Serial.print(F("Sample: ")); 
  Serial.print(sampleDelay); 
  Serial.print(F(" sec, "));
  Serial.print(F("Report: ")); 
  Serial.print(reportDelay); 
  Serial.println(F(" min."));

  // Remember current clock time
  prevReportMs = prevSampleMs = millis();
  
  // Turn on LED (indicates we're in initial sampling mode)
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,HIGH);
  LEDstate = true;
}

void loop()
{
  unsigned long currentMillis = millis();    // Check current timer value
  
  /* See if it is time to read the sensors */
  if( (currentMillis - prevSampleMs) >= sampleDelayMs) {
    numSamples ++;  // Since we started at 0...
    
    // Show activity via LED
    if(LEDstate == true) {
      digitalWrite(LED_PIN,LOW);
      LEDstate = false;
    }
    else {
      digitalWrite(LED_PIN,HIGH);
      LEDstate = true;
    }
    
    // Read temperature from DS18b20
    if(haveDS18B20) { DScurrentTempF = ds18b20GetTemperatureF(); }
    else {
      long randNumber;
      randNumber = random(-10,10);
      DScurrentTempF = 69 + (((float)randNumber)/10);
    }
    DSreadingF += DScurrentTempF;
  
    // Read temperature & pressure values from BMP085
    if(haveBMP180) {
      read_bmp180();
    }
    else {
      BMPtempC    = 12.3 + (random(-20,20)/10.0);
      BMPpresshPa = 960 + (random(-10,10)/5.0);
    }
    BMPreadingF += (((9.0/5.0)* BMPtempC) +  32.0);
    BMPreadingP += (0.0295333727 * BMPpresshPa);
    
    // Wind vane
    if(haveWindRain) {
      windDirection = readWindVane() + WINDVANE_OFFSET;
      if(windDirection > 360) windDirection -= 360.0;
      aggregateWindDir += windDirection;
    }
    
    // Save sample time and redraw display to update
    prevSampleMs = currentMillis;
    redrawDisplay();
  }

  /* Now check and see if it is time to report averaged values */
  if( (currentMillis - prevReportMs) >= reportDelayMs) {
    if( (numReports == 0) && (currentScreen == SCREEN_SPLASH)) {
      currentScreen = SCREEN_MAIN;  // We can leave the splash screen!
    }
    if(numReports >= MAXREPORTS) numReports = 1;
    else numReports++;

    // Calculate average readings for DS18b20 temperature, BMP085 temperature & pressure
    DSavgTempF  =  DSreadingF / numSamples;
    BMPaverageF = BMPreadingF / numSamples;
    BMPaverageP = BMPreadingP / numSamples;
    
    // Update max and min if necessary
    if(DSavgTempF > DSmaxTempF) {  DSmaxTempF = DSavgTempF; } /* New maximum? */
    if(DSavgTempF < DSminTempF) {  DSminTempF = DSavgTempF; } /* New Minimum? */
    
    /* Calculate average wind speed based on anemometer interrupt count */
    windSpeed = WIND_FACTOR * anemometerCnt *1000 / reportDelayMs;
    windGust  = WIND_FACTOR * 1000000.0 /anemom_min;
    anemometerCnt = 0;        // Zero interrupt counter
    anemom_min = 0xffffffff;  // Reset minimum gust time
    
    // Rain gauge
    rainFall = raingaugeCnt * RAIN_FACTOR;
    raingaugeCnt = 0;   // Zero interrupt counter
    
    // Average the wind vane readings
    windDirection = aggregateWindDir/numSamples;

    // Report results by printing them out the Serial port...
    Serial.print(numReports); 
    Serial.print(F(",")); 
    Serial.print(DSavgTempF); 
    Serial.print(F(","));
    Serial.print(BMPaverageP); 
    Serial.print(F(",")); 
    Serial.print(BMPaverageF);
    Serial.print(F(","));
    Serial.print(windSpeed);
    Serial.print(F(","));
    Serial.print(windGust);
    Serial.print(F(","));
    Serial.println(rainFall);

    /* Update the display */
    redrawDisplay();

    // Reset counters and accumulators
    prevReportMs = currentMillis;
    DSreadingF = 0;
    BMPreadingF = 0;
    BMPreadingP = 0;
    aggregateWindDir = 0;
    numSamples = 0;
  }
  
  /* See if the push button was used to switch screens */
  if(newScreen) {
    switch(currentScreen) {
      case SCREEN_MAIN:   currentScreen = SCREEN_MAX; break;
      case SCREEN_MAX:    currentScreen = SCREEN_WNDRN; break;
      case SCREEN_WNDRN:  currentScreen = SCREEN_STATUS; break;
      case SCREEN_STATUS: currentScreen = SCREEN_MAIN; break;
      case SCREEN_SPLASH: 
        if(numReports != 0) currentScreen = SCREEN_MAIN;
        else                currentScreen = SCREEN_SPLASH;
        break;
    }
    newScreen = false;
    redrawDisplay();
  }
  
  redrawDisplay();
}
// END of main loop()


// Convenience utility called within setup() to initialize interrupt handling
void initialize_interrupts()
{
  // Attach interrupt service routines to the respective interrupts
  if(haveWindRain) {
    attachInterrupt(ANEMOMETER_PIN,anemometerISR,RISING);
    attachInterrupt(RAINGAUGE_PIN ,raingaugeISR ,RISING);
  }
  attachInterrupt(SW_BUTTON,nextscreenISR,RISING);
}

/*
 * We're using a pin change interrupt to read the 'next screen' button which means
 * the associated port interrupt service routine will be called here when that
 * button's value changes.  We therefore handle that as a 'next screen' button push.
 * To keep things simple and short here, we just set a flag that'll get handled 
 * in the main loop.
 */
void nextscreenISR()
{
   newScreen = true;
} 

/* 
 * Interrupt service routine (ISR) called when anemometer closes its internal switch.
 * A wind speed of 1.492 MPH (2.4 km/h) causes the switch to close once per second.
 * Also keep track of shortest observed interrupt interval as that reflects the
 * maximum observed wind speed (gust).
 */
void anemometerISR()
{
  unsigned long anemom_now, adeltat;
  
  /* 
   * Calculate microseconds elapsed since last rotation, which helps us figure out
   * instantaneous wind gusts.  We do this using the micros() function, which is safe
   * for use within ISR but which rolls over ever 2^31 - 1 microseconds (about every
   * 72 minutes) so we need to be alert for rollover.
   */
  anemom_now = micros();
  if(anemom_now > anemom_last) {
    adeltat = anemom_now - anemom_last;  // No micros() rollover so just subtract
  }
  else {
    adeltat = MAXUNSIGNEDLONG - anemom_last + anemom_now; // Handle rollover case
  }
  if(adeltat < anemom_min) anemom_min = adeltat;   // New min rotation time?
  
  anemom_last = anemom_now;                        // New current interrupt time
  anemometerCnt++;                                 // Count this interrupt

}


// Interrupt service routine called when the rain gauge reports an accumulation
void raingaugeISR()
{
      raingaugeCnt++;
}

// Read the wind vane and return direction (0 - 360 degrees)
PROGMEM const int vaneValues[]     = 
    {  66, 84, 92, 127, 184, 244, 287,406,461, 600, 631, 702,786, 827, 889, 946};
PROGMEM const int vaneDirections[] = 
    {1125,675,900,1575,1350,2025,1800,225,450,2475,2250,3375,  0,2925,3150,2700};
float readWindVane()
{
  int i;
  unsigned int vanereading, lastdiff, diff;
  
  // Analog reading of wind vane value from 0 - 1023
  vanereading = analogRead(WINDVANE_PIN);
  
  // Find the vaneValue[] that is closest to the current reading.  The 
  // corresponding element in vaneDirections[] gives us the compass direction
  // from 0 to 360 degrees.
  lastdiff = 2048;
  for(i=0;i<16;i++) {
    diff = abs(pgm_read_word_near(vaneValues + i) - vanereading);
    if(diff == 0) return(pgm_read_word_near(vaneDirections+i)/10.0);
    if(diff > lastdiff) return(pgm_read_word_near(vaneDirections+i-1)/10.0);
    lastdiff = diff;
  }
  return(pgm_read_word_near(vaneDirections+15)/10.0);
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
  
  /* OK, so something to display...*/
  switch(currentScreen) {
    case SCREEN_MAIN:
      display.clearDisplay();
      for(i=0;i<128;i+=2) display.drawPixel(i,39,WHITE);
      showLargeValue(0,0,DSavgTempF,true,F("Outdoor"));
      showMediumTemp(64,41,BMPaverageF,F("Indoor"));
      showMediumBP(0,41,BMPaverageP,F("Pressure"));
      break;    
    case SCREEN_MAX:
      display.clearDisplay();
      for(i=0;i<128;i+=2) display.drawPixel(i,39,WHITE);
      showLargeValue(0,0,DSavgTempF,true,F("Outdoor"));
      showMediumTemp( 0,41,DSminTempF,F("Minimum"));
      showMediumTemp(64,41,DSmaxTempF,F("Maximum"));
      break;
    case SCREEN_WNDRN:
      display.clearDisplay();
      showLargeValue(0, 0,windSpeed,false,F("Wind (MPH)"));
      for(i=0;i<128;i+=2) display.drawPixel(i,39,WHITE);
      showMediumTemp( 0,41,windGust,F(" Gust"));
      showMediumBP(64,41,rainFall,F(" Rain (in)"));
      // showMediumTemp( 0,40,windDirection,F("Dir"));
      break;
    case SCREEN_STATUS:
      display.clearDisplay();
      display.setCursor(0,0);
      display.print(F("Outdoor (Cur): ")); 
      display.print(DScurrentTempF);
      display.setCursor(0,10);
      display.print(F("Barometer    : ")); 
      display.println((BMPpresshPa * 0.0295333727));
      display.setCursor(0,20);
      display.print(F("Indoor  (Cur): ")); 
      display.println((((9.0/5.0)* BMPtempC) +  32.0));
      display.setCursor(0,40);
      display.print(F("Int# W: "));
      display.print(anemometerCnt);
      display.print(F(", R: "));
      display.print(raingaugeCnt);
      display.setCursor(0,50);
      display.print(F("#Samp: "));
      display.print(numSamples);
      display.setCursor(56,50);
      display.print(F(" #Rep: "));
      display.print(numReports);
      break;
    case SCREEN_SPLASH:
      splashScreen();
      break;
    }
  display.display();
}

/*
 * Display temperature in large font at (x,y) with a specific label.
 * NOTE: This version requires that the label string be in flash memory,
 * so the function should be called as:
 * 
 *   showLargeTemp(0,10,73.4,true,F("Outdoor"));
 */
// void showLargeTemp(int x, int y, float value, boolean isTemp, char *label)
void showLargeValue(int x, int y, float value, boolean isTemp, const __FlashStringHelper *label)

{
  byte h, t, o, d;
  boolean showzero = false;
  boolean isneg;
  
  if(value < 0.0) {
      isneg = true;
      value = -1*value;
  }
  else {
    isneg = false;
  }
  value += 0.05;  /* We're rounding up to tenths of a degree */
  d = ((int) (value*10)) % 10;
  o = ((int) (value)) % 10;
  t = ((int) (value/10)) % 10;
  h = ((int) (value/100));
  if(h > 9) h = 9;
  
  display.setCursor(x,y);
  display.print(label);
  
  if(value >= 100.0) {
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
  if(isTemp) {
    display.drawBitmap(x+92,y+9,font_5x7_deg,8,7,WHITE);
    display.drawBitmap(x+102,y+9,font_10x14_degf,16,14,WHITE);
  }
}
 
/*
 * Display temperature in medium font at (x,y) with a specific label
 */
void showMediumTemp(int x, int y, float tempF, const __FlashStringHelper *label)
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
void showMediumBP(int x, int y, float value,const __FlashStringHelper *label)
{
  int t, o, d, m;

  value += 0.005;  // We're rounding to hundredths
  m = ((int) (value*100)) %10;
  d = ((int) (value*10)) % 10;
  o = ((int)  value) % 10;
  t = ((int) (value/10)) % 10;
  display.setCursor(x,y);
  display.print(label);
  
  if(value >= 10.0) {
    display.drawBitmap( x,y+9,font_10x14_digits[t],16,14,WHITE);
  }
  display.drawBitmap(x+12,y+9,font_10x14_digits[o],16,14,WHITE);
  display.drawBitmap(x+24,y+15,font_5x7_dpt,8,7,WHITE);
  display.drawBitmap(x+34,y+9,font_10x14_digits[d],16,14,WHITE);
  display.drawBitmap(x+46,y+9,font_10x14_digits[m],16,14,WHITE);
}



// Show something when there's nothing (yet) to show
void splashScreen()
{
  int i;
  
  display.clearDisplay();
  for(i=0;i<128;i++) display.drawPixel(i,0,WHITE);
  for(i=1;i<63;i++) display.drawPixel(0,i,WHITE);
  for(i=1;i<63;i++) display.drawPixel(127,i,WHITE);
  for(i=0;i<128;i++) display.drawPixel(i,63,WHITE);
  display.setCursor(16,24);   
  display.print("Sampling...");
  display.setCursor(16,40);   
  display.print("Please wait...");

  display.display();
}


// ********* BELOW HERE IS SENSOR RELATED CODE (initialization, I/O, etc.) *****

void initialize_sensors()
{
  if(haveDS18B20) {
    sensors.begin();      // Start the DS18B20
  }
  if(haveBMP180) {  
    // Initialize the BMP180 sensor (it is important to get calibration values stored on the device).
    // Read the WHO_AM_I register of the BMP-180, this is a good test of communication
    uint8_t c = readByte(BMP180_ADDRESS, BMP180_WHO_AM_I);  // Read WHO_AM_I register A for BMP-180
    if (c == 0x55) { // WHO_AM_I must be 0x55 (BMP-180 id) to proceed
      // Serial.println("BMP180 is online...");  
      // Initialize devices for active mode read of pressure and temperature
      BMP180Calibration();
      // Serial.println("BMP-180 calibration completed....");
    }
    else
    {
      // Oops, something went wrong, this is usually a connection problem,
      display.clearDisplay();
      display.setCursor(0,10);
      display.println("BMP180 init failed!");
      display.display();
      while(1); // Pause forever.
    }
  }
}

//===================================================================================================================
//====== Set of useful function to access pressure and temperature data
//===================================================================================================================

void read_bmp180()
{
  BMPtempC = (float) BMP180GetTemperature()/10.;
  BMPpresshPa = (float) BMP180GetPressure()/100.;
}
  
// Stores all of the BMP180's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
// These BMP-180 functions were adapted from Jim Lindblom of SparkFun Electronics
void BMP180Calibration()
{
  ac1 = readByte(BMP180_ADDRESS, 0xAA) << 8 | readByte(BMP180_ADDRESS, 0xAB);
  ac2 = readByte(BMP180_ADDRESS, 0xAC) << 8 | readByte(BMP180_ADDRESS, 0xAD);
  ac3 = readByte(BMP180_ADDRESS, 0xAE) << 8 | readByte(BMP180_ADDRESS, 0xAF);
  ac4 = readByte(BMP180_ADDRESS, 0xB0) << 8 | readByte(BMP180_ADDRESS, 0xB1);
  ac5 = readByte(BMP180_ADDRESS, 0xB2) << 8 | readByte(BMP180_ADDRESS, 0xB3);
  ac6 = readByte(BMP180_ADDRESS, 0xB4) << 8 | readByte(BMP180_ADDRESS, 0xB5);
  b1  = readByte(BMP180_ADDRESS, 0xB6) << 8 | readByte(BMP180_ADDRESS, 0xB7);
  b2  = readByte(BMP180_ADDRESS, 0xB8) << 8 | readByte(BMP180_ADDRESS, 0xB9);
  mb  = readByte(BMP180_ADDRESS, 0xBA) << 8 | readByte(BMP180_ADDRESS, 0xBB);
  mc  = readByte(BMP180_ADDRESS, 0xBC) << 8 | readByte(BMP180_ADDRESS, 0xBD);
  md  = readByte(BMP180_ADDRESS, 0xBE) << 8 | readByte(BMP180_ADDRESS, 0xBF);
}

// Temperature returned will be in units of 0.1 deg C
int16_t BMP180GetTemperature()
{
  int16_t ut = 0;
  writeByte(BMP180_ADDRESS, BMP180_CONTROL, 0x2E); // start temperature measurement
  delay(5);
  uint8_t rawData[2] = {0, 0};
  readBytes(BMP180_ADDRESS, 0xF6, 2, &rawData[0]); // read raw temperature measurement
  ut = (int16_t)(((int16_t) rawData[0] << 8) | rawData[1]);
 
 long x1, x2;
  
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return  ((b5 + 8)>>4);  
}

// Calculate pressure read calibration values  
// b5 is also required so BMP180GetTemperature() must be called first.
// Value returned will be pressure in units of Pa.
long BMP180GetPressure()
{
  long up = 0;
  writeByte(BMP180_ADDRESS, BMP180_CONTROL, 0x34 | OSS << 6); // Configure pressure measurement for highest resolution
  delay(5 + 7*OSS); // delay 5 ms at lowest resolution, 26 ms at highest
  uint8_t rawData[3] = {0, 0, 0};
  readBytes(BMP180_ADDRESS, BMP180_OUT_MSB, 3, &rawData[0]); // read raw pressure measurement of 19 bits
  up = (((long) rawData[0] << 16) | ((long)rawData[1] << 8) | rawData[2]) >> (8 - OSS);

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

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++ Useful Display Functions++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 // Wire.h read and write protocols
 void writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
	Wire.beginTransmission(address);  // Initialize the Tx buffer
	Wire.write(subAddress);           // Put slave register address in Tx buffer
	Wire.write(data);                 // Put data in Tx buffer
	Wire.endTransmission();           // Send the Tx buffer
}

  uint8_t readByte(uint8_t address, uint8_t subAddress)
{
	uint8_t data; // `data` will store the register data	 
	Wire.beginTransmission(address);         // Initialize the Tx buffer
	Wire.write(subAddress);	                 // Put slave register address in Tx buffer
	Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
	Wire.requestFrom(address, (uint8_t) 1);  // Read one byte from slave register address 
	data = Wire.read();                      // Fill Rx buffer with result
	return data;                             // Return data read from slave register
}

 void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{  
	Wire.beginTransmission(address);   // Initialize the Tx buffer
	Wire.write(subAddress);            // Put slave register address in Tx buffer
	Wire.endTransmission(false);       // Send the Tx buffer, but send a restart to keep connection alive
	uint8_t i = 0;
        Wire.requestFrom(address, count);  // Read bytes from slave register address 
	while (Wire.available()) {
        dest[i++] = Wire.read(); }         // Put read results in the Rx buffer
}

// Read the temperature F from the DS18B20 sensor
float ds18b20GetTemperatureF()
{  
  // Issue a global temperature request to all devices on the I2C bus
  sensors.requestTemperatures();

  // Now read temperature from DS18B20 sensor #1 (device #0)
  return(sensors.getTempFByIndex(0));
}
