/*
 * Home Weather Station -  Arduino-based weather station
 *
 * Copyright (c) 2015 David Bryant <david@orangemoose.com>
 *
 * Reads and reports temperature and barometric pressure from several attached
 * sensors.  This version is supports the Dallas Semiconductor DS18B20 digital
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
 * Version 1.0, 1 March 2015
 * License: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/ 
 *
 * 
 * Arduino Pinouts (for 5-wire SPI connection to OLED)
 *   A0 = Input for push button to cycle through screens (interrupt mode)
 *   A1 = Input for Wind Vane to read it as resistor network
 *   A4 = SDA for BMP180 (I2C Wire protocol)
 *   A5 = SCL for BMP180 (I2C Wire protocol)
 *    2 = Anemometer input (via INT0)
 *    3 = Rain Gauge input (via INT1)
 *    5 = DS18B20 temperature sensor (direct digital read)
 *    9 = MOSI -> OLED DATA
 *   10 = CLK  -> OLED CLK
 *   11 = DC  -> OLED SAO
 *   12 = CS  -> OLED CS
 *   13 = RESET -> OLED RST
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
#include <SFE_BMP180.h>

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
SFE_BMP180 pressure;
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

#define OLED_DC 11
#define OLED_CS 12
#define OLED_CLK 10
#define OLED_MOSI 9
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);


// *****************  WIND/RAIN SENSOR CONFIGURATION *****************
#define ANEMOMETER_INT   0  // INT0, so Anemometer is on digital pin 2
#define RAINGAUGE_INT    1  // INT1, so Rain Gauge is on digital pin 3
#define ANEMOMETER_PIN   2  // Digital pin 2, must agree with ANEMOMETER_INT value
#define RAINGAUGE_PIN    3  // Digital pin 3, must agree with RAINGAUGE_INT value
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
  // Output via the Serial monitor
  Serial.begin(9600); 

  randomSeed(analogRead(0));
  
  // Enable pull-up resistors on Anemometer & Rain Gauge pins
  pinMode(ANEMOMETER_PIN,INPUT);
  digitalWrite(ANEMOMETER_PIN,HIGH);
  pinMode(RAINGAUGE_PIN,INPUT);
  digitalWrite(RAINGAUGE_PIN,HIGH);
  
  // Single entry point to set up all our sensors
  initialize_sensors();
  
  // Generalized entry point for whatever is needed to configure our interrupts
  initialize_interrupts();

  // Initialize OLED
  // By default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done

  display.clearDisplay();
  display.setTextColor(WHITE);
  // splashScreen();

  // Output key operating parameters to help interpret overall data set
  Serial.print(F("Sample: ")); 
  Serial.print(sampleDelay); 
  Serial.print(F(" sec, "));
  Serial.print(F("Report: ")); 
  Serial.print(reportDelay); 
  Serial.println(F(" min."));

  // Remember current clock time
  prevReportMs = prevSampleMs = millis();
}

void loop()
{
  unsigned long currentMillis = millis();    // Check current timer value
  
  /* See if it is time to read the sensors */
  if( (currentMillis - prevSampleMs) >= sampleDelayMs) {
    numSamples ++;  // Since we started at 0...
    
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
    attachInterrupt(ANEMOMETER_INT,anemometerISR,RISING);
    attachInterrupt(RAINGAUGE_INT ,raingaugeISR ,RISING);
  }
  
  /*
   * We're using pin A0 to read the push button so we need to configure the right
   * interrupt settings to enable a pin change interrupt on A0. Based on
   * http://playground.arduino.cc/Learning/Pins we see that A0 is on Port C
   * as PCINT8, with PCMSK of 0 within PCMSK1.  The '1' in the PCMKS1 value means
   * that we need to clear/enable interrupts and provide an interrupt service
   * routine on port/bank 1 overall.
   * 
   * If you want to use a different pin then you'll need to figure out which
   * port/mask values to use here...
   */
  cli();                 // Disable interrupts while adjusting interrupt settings
  PCIFR  |= bit(PCIF1);  // Clear any pending interrupts on Port C (#1)
  PCICR  |= bit(PCIE1);  // Enable interrupts on port 1 (pins A0 -> A5 inclusive)
  PCMSK1 |= bit(PCINT8); // Enable interrupts just for pin A0 (which is also PCINT8)
  sei();                 // Re-enable interrupts now that we have everything set up
}

/*
 * We're using a pin change interrupt to read the 'next screen' button which means
 * the associated port interrupt service routine will be called here when that
 * button's value changes.  We therefore handle that as a 'next screen' button push.
 * To keep things simple and short here, we just set a flag that'll get handled 
 * in the main loop.
 */
ISR(PCINT1_vect)
{
  /*
   * If pin A0 is now HIGH then button was pushed.  If LOW the button was released. 
   */
   if(digitalRead(A0)==HIGH) newScreen = true;
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
  int i, diff;
  unsigned int vanereading, lastdiff;
  
  // Analog reading of wind vane value from 0 - 1023
  vanereading = analogRead(WINDVANE_PIN);
  
  // Find the vaneValue[] that is closest to the current reading.  The 
  // corresponding element in vaneDirections[] gives us the compass direction
  // from 0 to 360 degrees.
  lastdiff = 2048;
  for(i=0;i<16;i++) {
    diff = pgm_read_word_near(vaneValues + i) - vanereading;
    diff = abs(diff);
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
  display.clearDisplay();
  display.setCursor(16,24);   
  display.print(F("Sampling..."));
  display.setCursor(16,40);   
  display.print(F("Please wait..."));
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
    if (pressure.begin()) {
      display.setCursor(0,0);
      display.println(F("BMP180 init success"));
      display.display();
    }
    else
    {
      // Oops, something went wrong, this is usually a connection problem,
      display.setCursor(0,0);
      display.println(F("BMP180 init failed!"));
      display.display();
      while(1); // Pause forever.
    }
  }
}

// Adapted from the sample program provided with the Sparkfun BMP180 library
void read_bmp180()
{
  char status;
  
  // If you want to measure altitude, and not pressure, you will instead need
  // to provide a known baseline pressure. This is shown at the end of the sketch.

  // You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(BMPtempC);
    if (status != 0)
    { 
          BMPtempError = false;
      
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.
      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.
        status = pressure.getPressure(BMPpresshPa,BMPtempC);
        if (status != 0) {
          BMPpressError = false;
        }
        else {
          BMPpressError = true;
        }
      }
      else {
        BMPpressError = true;
      }
    }
    else {
      BMPtempError = true;
    }
  }
  else {
      BMPtempError = true;
  }
}


// Read the temperature F from the DS18B20 sensor
float ds18b20GetTemperatureF()
{  
  // Issue a global temperature request to all devices on the I2C bus
  sensors.requestTemperatures();

  // Now read temperature from DS18B20 sensor #1 (device #0)
  return(sensors.getTempFByIndex(0));
}
