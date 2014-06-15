/*
 * Intervalometer for time-lapse photography
 *
 * Author: David Bryant (david_bryant@yahoo.com)
 * February 19, 2012
 * 
 * Arduino Pinouts -- (without use of a shift register)
 *    
 *    2 = Start/stop button
 *    3 = Menu button
 *    4 = Up/plus button
 *    5 = Down/minus button
 *    6 = PWM control for red backlight LED brightness -> LCD16
 *    7 = LCD RS -> LCD4
 *    8 = LCD EN -> LCD6
 *    9 = LCD DB4 (data) -> LCD11
 *   10 = LCD DB5 (data) -> LCD12
 *   11 = LCD DB6 (data) -> LCD13
 *   12 = LCD DB7 (data) -> LCD14
 *   13 = Trigger for camera
 *
 *
 * Arduino Pinouts -- (if used in 3-wire mode with a latching shift register e.g., 74HC595N)
 *    
 *    2 = Start/stop button
 *    3 = Menu button
 *    4 = Up/plus button
 *    5 = Down/minus button
 *    6 = PCM control for red backlight LED -> LCD16
 *    7 = Shift Register Data   -> 74HC595N pin 14 (Serial Data Input DS)
 *    8 = Shift Register Clock  -> 74HC595N pin 11 (Shift Clock SHCP)
 *    9 = Shift Register Enable -> 74HC595N pin 12 (Storage Clock STCP) 
 *        *AND* LCD Enable      -> LCD6 (EN)
 *   13 = Trigger for camera
 *
 *   And for the 74HC595N shift register
 *     2 (Q2)  = LCD RS -> LCD4
 *     3 (Q3)  = LCD DB4 (data) -> LCD11
 *     4 (Q4)  = LCD DB5 (data) -> LCD12
 *     5 (Q5)  = LCD DB6 (data) -> LCD13
 *     6 (Q6)  = LCD DB7 (data) -> LCD14
 *     8 (GND) = GND
 *    10 (MR)  = VCC  [Master reset, active LOW]
 *    11 (SHCP) = Arduino pin 8
 *    12 (STCP) = Arduino pin 9
 *    13 (OE)  = GND  [Output enable, active LOW]
 *    14 (DS)  = Arcuino pin 7
 *    16 (VCC) = VCC
 *
 * Additional LCD connections (valid for either with or without a shift register)
 *    LCD1 = GND
 *    LCD2 = VCC
 *    LCD3 = Contrast voltage (wiper of contrast pot)
 *    LCD5 = GND
 *    LCD15 = VCC
 *
 */
 
/*
 * Include the LCD library code. The standard Arduino 1.0 LCD library works fine but does not support
 * use of a shift register and so requires 6 control lines from the Arduino to the LCD.  That number
 * can be reduced to 3 control lines through use of a latching shift register but requires a different
 * LiquidCrystal library.  (This frees up 3 pins for other purposes.) The code here uses an
 * updated library from F.Malpartida et. al. at https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
 * which also has the benefit of improved performance relative to the standard Arduino 1.0 library.
 */
#include <LiquidCrystal_SR.h> 

/* Menu pages. Order is governed by logic in the main loop, not by numerical sequence. */
#define PG_MAIN     1
#define PG_SET_ELA  2
#define PG_SET_FPS  3
#define PG_SET_PRG  4
#define PG_SET_LEN  5
#define PG_SET_CAP  6

/* Program Mode */
#define PRG_DURATION  1         
#define PRG_INTERVAL  2

/* Operational Mode */
#define RUNNING  1
#define MENU     2
#define STOPPED  3

/* Debounce delay for buttons (in milliseconds) */
#define DEBOUNCE_DELAY 150

int runButtonPin  = 5;         /* Pin the start/stop button is connected to */
int menuButtonPin = 4;         /* Pin the menu button is connected to */
int upButtonPin   = 3;         /* Pin the up(plus) button is connected to */
int downButtonPin = 2;         /* Pin the down(minus) button is connected to */
int redLEDPin     = 6;         /* Pin that controls red display on LCD panel */
int triggerPin    = 13;        /* Pin that triggers the camera */

int runBtnState;               /* Variable for reading the run button status */
int lastRunBtnState = LOW;     /* Store last button state */
int menuBtnState;              /* Variable for reading the menu button status */
int lastMenuBtnState = LOW;    /* Store last button state */
int upBtnState;                /* Variable for reading the up (plus) button status */
int lastUpBtnState = LOW;      /* Store last button state */
int downBtnState;              /* Variable for reading the down (minus) button status */
int lastDownBtnState = LOW;    /* Store last button state */

int brightness = 0;           /* LCD brightness, 0=max, 255=off. 100 good @ 5V, 0 @ 3.3V */
int currentPage = PG_MAIN;     /* Currerntly displayed page (defaults to main page) */
int mode = STOPPED;            /* Current operating mode (defaults to stopped) */
unsigned long prevMillis = 0;  /* Timestamp of previous picture, used to calculate when to take next one */

/* Exposure parameters */
int progMode = PRG_DURATION;   /* Default program mode is to configure via video duration */
long elapsedTime = 60;         /* How long in clock time to capture in the video (in minutes) */
int frameRate = 15;            /* Frames per second in the final video */
long videoLength = 1;          /* How long should the final video be? (in minutes) */
long captureDelay = 0;         /* Interval between pictures (in seconds) */
unsigned long captureDelayMs;  /* capture delay in milliseconds */
int numPictures;               /* Number of pictures to take */
int remPictures;               /* Number of pictures remaining to be taken (when running) */


/* Initialize the LCD library with the numbers of the interface pins */
// LiquidCrystal lcd(7, 8, 9, 10, 11, 12);    /* if using 4-bit parallel interface */
LiquidCrystal_SR lcd(7, 8, 9);                /* if using 3 wire shift register interface */
 
 
void setup() {
  
  pinMode(triggerPin,OUTPUT);
  
  /* Enable pullup resistors on button input pins */
  pinMode(runButtonPin,INPUT);
  digitalWrite(runButtonPin,HIGH);
  pinMode(menuButtonPin,INPUT);
  digitalWrite(menuButtonPin,HIGH);
  pinMode(upButtonPin,INPUT);
  digitalWrite(upButtonPin,HIGH);
  pinMode(downButtonPin,INPUT);
  digitalWrite(downButtonPin,HIGH);
  
  Serial.begin(9600);
  
  /* Initialize the LCD, specify that it is 16x2 */
  lcd.begin(16, 2);
    
  /* Set display brightness.  0 is max, 255 is min */
  analogWrite(redLEDPin,brightness);
  
  /* Display the boot screen */
  bootscreen();
  Serial.println("*** Arduino Intervalometer ***");
  
  /* Make initial calculation based on default parameters and display to user */
  recalculate();
  updateDisplay(true);
}
 
void loop()
{
  /* Behave according to current operational mode */
  switch(mode) {
    case RUNNING:
      /* If running, only button to check for is run/stop */
      runBtnState = digitalRead(runButtonPin);
      if( (runBtnState != lastRunBtnState) && (runBtnState == LOW) ) {
        mode = STOPPED;
        currentPage = PG_MAIN;
        Serial.print("Stopped. Current page #"); Serial.println(currentPage);
        recalculate();
        updateDisplay(true);
        delay(DEBOUNCE_DELAY);     // debounce
      }
      else {
        /* If not stopping then check to see if we've taken the last picture */
        if(remPictures == 0) {
          mode = STOPPED;
          currentPage = PG_MAIN;
          Serial.print("Stopped - last picture. Current page #"); Serial.println(currentPage);
          recalculate();
          updateDisplay(true);
        }
        else {
          /* Still pictures to take, so see if enough time has passed to take one */
          unsigned long currentMillis = millis();
          if( (currentMillis - prevMillis) >= captureDelayMs ) {
            triggerCamera();
            updateDisplay(false);
            prevMillis = currentMillis;    // save current time as basis for next picture
          }
        }
      }
      lastRunBtnState = runBtnState;         // save current button state
      break;
      
    case STOPPED:
      /* First, see if the run button has been pressed, in which case we start running */
      runBtnState = digitalRead(runButtonPin);
      if( (runBtnState != lastRunBtnState) && (runBtnState == LOW) ) {
        mode = RUNNING;
        currentPage = PG_MAIN;
        Serial.print("Running. Current page #"); Serial.println(currentPage);
        prevMillis = millis();     // present time is reference for taking first picture
        updateDisplay(true);
        delay(DEBOUNCE_DELAY);    // debounce
      }
      else {
        /* Run button wasn't pressed, so check for a menu button press and if so show the first menu */
        menuBtnState = digitalRead(menuButtonPin);
        if( (menuBtnState != lastMenuBtnState) && (menuBtnState == LOW) ) {
          mode = MENU;
          currentPage = PG_SET_ELA;
          Serial.print("Current page #"); Serial.println(currentPage);
          updateDisplay(true);
          delay(DEBOUNCE_DELAY);    // debounce
        }
        lastMenuBtnState = menuBtnState;
      }
      lastRunBtnState = runBtnState; 
      break;
      
    case MENU:
      /* First check to see if the menu button was pressed, and if so go to the next menu page */
      menuBtnState = digitalRead(menuButtonPin);
      if( (menuBtnState != lastMenuBtnState) && (menuBtnState == LOW) ) {
        switch(currentPage) {
          case PG_SET_ELA: currentPage = PG_SET_FPS; break;
          case PG_SET_FPS: currentPage = PG_SET_PRG; break;
          case PG_SET_PRG: 
            if(progMode == PRG_DURATION) currentPage = PG_SET_LEN;
            else currentPage = PG_SET_CAP;
            break;
          case PG_SET_LEN: 
          case PG_SET_CAP:
            currentPage = PG_MAIN; 
            recalculate(); 
            mode = STOPPED;
            break;
        }
        Serial.print("Current page #"); Serial.println(currentPage);
        updateDisplay(true);
        delay(DEBOUNCE_DELAY);    // debounce
      }
      else {
        /* Menu wasn't pressed, so check to see if up or down buttons were used to change parameter value */
        upBtnState = digitalRead(upButtonPin);
        if( (upBtnState != lastUpBtnState) && (upBtnState == LOW) ) {
          switch(currentPage) {
            case PG_SET_ELA: elapsedTime++; break;
            case PG_SET_FPS: frameRate++;   break;
            case PG_SET_PRG:
              if(progMode == PRG_DURATION) progMode = PRG_INTERVAL;
              else progMode = PRG_DURATION;
              break;
            case PG_SET_LEN: videoLength++; break;
            case PG_SET_CAP: captureDelay++; break;
          }
          Serial.println("Menu setting ++");
          updateDisplay(false);
          delay(DEBOUNCE_DELAY);    // debounce
        }
        else {
          downBtnState = digitalRead(downButtonPin);
          if( (downBtnState != lastDownBtnState) && (downBtnState == LOW) ) {
            switch(currentPage) {
              case PG_SET_ELA: if(elapsedTime >1) elapsedTime--; break;
              case PG_SET_FPS: if(frameRate >1)   frameRate--;   break;
              case PG_SET_PRG:
                if(progMode == PRG_DURATION) progMode = PRG_INTERVAL;
                else progMode = PRG_DURATION;
                break;
              case PG_SET_LEN: if(videoLength >1) videoLength--; break;
              case PG_SET_CAP: if(captureDelay >1) captureDelay--; break;
              }
              Serial.println("Menu setting --");
              updateDisplay(false);
              delay(DEBOUNCE_DELAY);    // debounce
          }
          lastDownBtnState = downBtnState;
        }
        lastUpBtnState = upBtnState;
      }
      lastMenuBtnState = menuBtnState;
      break;
  } 
}

/*
** Recalculate control parameters based on the specified programming mode:
**
**  By Interval: User has set overall elapsed time, capture delay between
**       pictures, and frame rate.  Achieving millisecond accuracy in timing is
**       straightforward (only need long precision integers)
**
**  By Duration: User has set overall elapsed time, length of time lapse video
**       to be created, and frame rate.  Capture delay needs to be calculated
**       and should be in milliseconds for good accuracy, though will only be
**       displayed to the user in seconds.  Use floating point math for precision
**       (NOTE: though should be able to do this using unsigned longs...)
*/
void recalculate()
{
  float val;
  
  switch(progMode) {
    case PRG_INTERVAL:
      numPictures =  elapsedTime * 60 / captureDelay;
      videoLength =  numPictures / (frameRate * 60);
      captureDelayMs = captureDelay * 1000L;
      break;
    case PRG_DURATION:
    default:
      numPictures = videoLength * 60 * frameRate;
      val = elapsedTime * 60.0 * 1000.0 / numPictures;
      captureDelayMs = val + 0.5;
      captureDelay = captureDelayMs / 1000L;
      break;
  }
  remPictures = numPictures;
  dumpParameters();
}

void dumpParameters() 
{
  Serial.println("--- Recalculated Parameters ---");
  Serial.print("Capture delay: "); Serial.println(captureDelay);
  Serial.print("Elapsed time: ");  Serial.println(elapsedTime);
  Serial.print("Video Length: ");  Serial.println(videoLength);
  Serial.print("# of Pictures: "); Serial.println(numPictures);
  Serial.print("Frame rate: ");    Serial.println(frameRate);
  Serial.println("--------------------------------");
}

/*
** Update the display.  What's shown in the LCD depends on which page is currently active 
*/
void updateDisplay(boolean doClear)
{
  if(doClear == true) lcd.clear();
  switch(currentPage) {
    case PG_MAIN:
      lcd.setCursor(0,0);
      lcd.print("C:");
      lcd.print(captureDelay);
      lcd.print(" E:");
      lcd.print(elapsedTime);
      lcd.print(" L:");
      lcd.print(videoLength);
      lcd.setCursor(0,1);
      if(mode == STOPPED) {
        lcd.print("P:");
        lcd.print(numPictures);
        lcd.print("    *Stop*");
      }
      else {
        lcd.print(remPictures);
        lcd.print("/");
        lcd.print(numPictures);
        lcd.print("  *Run*");
      }
      break;
    case PG_SET_ELA:
      lcd.setCursor(0,0);
      lcd.print("Elapsed time: ");
      lcd.setCursor(0,1);
      lcd.print("     ");
      lcd.print(elapsedTime);
      lcd.print(" min  "); 
      break;
    case PG_SET_FPS:
      lcd.setCursor(0,0);
      lcd.print("Frame rate:   ");
      lcd.setCursor(0,1);
      lcd.print("     ");
      lcd.print(frameRate);
      lcd.print(" fps   ");
      break;
    case PG_SET_PRG:
      lcd.setCursor(0,0);
      lcd.print("Program mode:  ");
      lcd.setCursor(0,1);
      if(progMode == PRG_DURATION) {
        lcd.print("   Duration   "); 
      }
      if(progMode == PRG_INTERVAL) {
        lcd.print("   Interval   ");
      }
      break;
    case PG_SET_LEN:
      lcd.setCursor(0,0);
      lcd.print("Length of video:");
      lcd.setCursor(0,1);
      lcd.print("     ");
      lcd.print(videoLength);
      lcd.print(" min  ");
      break;
    case PG_SET_CAP:
      lcd.setCursor(0,0);
      lcd.print("Capture delay:");
      lcd.setCursor(0,1);
      lcd.print("     ");
      lcd.print(captureDelay);
      lcd.print(" sec  "); 
      break;
  }
}

/*
** Take a picture, and count down so we can tell when we are done.
*/
void triggerCamera()
{
  digitalWrite(triggerPin,HIGH);
  /* 
  ** Keep the trigger active long enough to make sure the camera sees it and can react,
  ** and also avoid any bouncing that could result in multiple pictures.  The delay
  ** value here is a guess...
  */
  delay(50);
  digitalWrite(triggerPin,LOW);
  remPictures--;
  Serial.print("Camera triggered. Pictures remaining: ");
  Serial.println(remPictures);
}

/* Display a special message on powering up */
void bootscreen()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    Arduino");
  lcd.setCursor(0,1);
  lcd.print(" Intervalometer ");
  delay(3000);
}
