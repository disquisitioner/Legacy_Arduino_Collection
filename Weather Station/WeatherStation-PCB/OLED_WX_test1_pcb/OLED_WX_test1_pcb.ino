/* 

Test sketch for use in bring-up of my Weather Station PCB.  This test
presumes the OLED and BMP180 temperature & barometric pressure sensor
have been verified to be working properly, and moves on next to test 
the pushbutton switch and associated debounce circuitry. It uses the
OLED to display test status and results, and the push button can be used
to step through screens on the display.

Additional hardware connections for this test
- SW pin from PCB connects to pin A0 on the Arduino

*/

#include <SPI.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   9  // Data
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// BMP180 is connectea via I2C so A4 (SDA) and A5 (SCL)
// You will need to create an SFE_BMP180 object, here called "pressure":
SFE_BMP180 pressure;

/*
 * Support a variety of screens and keep track of the one currently displayed
 */
#define SCREEN_MAIN   0
#define SCREEN_ALT    1
int currentScreen = SCREEN_MAIN;  /* Start with the main screen */
volatile boolean newScreen = false;

/* Control the timing of sampling the sensor(s) */
int sampleDelay = 5;           /* Interval at which temperature sensor is read (in seconds) */
unsigned long sampleDelayMs;   /* Sample delay interval as milliseconds (calculated) */
unsigned long prevSampleMs  = 0;  /* Timestamp for measuring elapsed sample time */
unsigned long numSamples = 0;  /* Number of overall sensor readings over capture interval */
double TempF, Pressure;
boolean tempError = true;      // Don't display temp/pressure until we've read the sensors
boolean pressureError = true;

void setup()
{ 
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  
  display.clearDisplay();   // clears the screen and buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Set-up our interrupts.  For this test it's just the one for the pushbutton
  initialize_interrupts();

  // Initialize the sensor (it is important to get calibration values stored on the device).
  if (pressure.begin()) {
    display.setCursor(0,0);
    display.println(F("BMP180 init success"));
    display.display();
  }
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.
    display.setCursor(0,0);
    display.println(F("BMP180 init failed!"));
    display.display();
    while(1); // Pause forever.
  }
  
  // Calculate sample timing
  sampleDelayMs  = sampleDelay * 1000L;         /* converts seconds to milliseconds */
}

void loop()
{
  unsigned long currentMillis = millis();    // Check current timer value

  /* See if it is time to read the sensors */
  if( (currentMillis - prevSampleMs) >= sampleDelayMs) {
    numSamples ++;  // Since we started at 0...
    
    // Read sensors.  Global variables will be updated as appropriate
    read_sensors();
    
    // Handle sample counter
    if(numSamples >= 9999) numSamples = 0;
    
    // Remember time of this sample
    prevSampleMs = currentMillis;
  }

  
  /* If we're supposed to move to a new screen do that */
  if(newScreen) {
    next_screen();
    newScreen = false;
  }
  display_screen();  // Sub-optimal.  Improve...
}

// Convenience utility called within setup() to initialize interrupt handling
void initialize_interrupts()
{ 
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

void read_sensors()
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

    status = pressure.getTemperature(TempF);
    if (status != 0)
    { 
      tempError = false;
      
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
        status = pressure.getPressure(Pressure,TempF);
        if (status != 0) {
          pressureError = false;
        }
        else {
          pressureError = true;
        }
      }
      else {
        pressureError = true;
      }
    }
    else {
      tempError = true;
    }
  }
  else {
      tempError = true;
  }
}

/* 
 * Utility function called to determine the next screen. Logic about screen 
 * transitions gets implemented here. The actual display transition gets handled 
 * separately.
 */
void next_screen()
{
  int nextScreen;

  switch(currentScreen) {
    case SCREEN_MAIN: 
      nextScreen = SCREEN_ALT;
      break;
    case SCREEN_ALT:
      nextScreen = SCREEN_MAIN;
      break;
  }
    
  // If there's any housekeeping based on actual current screen do it before
  // you get here
  currentScreen = nextScreen;
}

/* Display the current screen */
void display_screen()
{
    switch(currentScreen) {
      case SCREEN_MAIN: 
        main_screen();
        break;
      case SCREEN_ALT:
        alt_screen();
        break;
    }
}

void main_screen()
{
  // OLED layout
  // Line 1: Temperature reading (or indication of error)
  // Line 2: Pressure reading (or indication of error)
  // Line 3: Loop counter, rolls over at 9999
  //
  // Start with a clear OLED display
  display.clearDisplay();
  
  // Display temperature on OLED on line 1
  display.setCursor(0,0);
  display.print(F("Temp F: "));
  if(tempError) {
    display.println("-----");
  }
  else {
    display.println(((9.0/5.0) *TempF + 32.0),2); 
  }
  
  // Display pressure on line 2 of the OLED
  display.setCursor(0,10);
  display.print(F("P inHg: "));
  if(pressureError) {
    display.println(F("-----"));
  }
  else {
    display.println((Pressure * 0.0295333727),2);
  }

  // Display our loop counter
  display.setCursor(0,20);
  display.print(F("Loop #: "));
  display.println(numSamples);
  display.display();
}

void alt_screen()
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("Alternate screen"));
  display.display();
}
