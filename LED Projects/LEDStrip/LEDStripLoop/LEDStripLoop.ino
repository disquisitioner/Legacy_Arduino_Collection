// color swirl! connect an RGB LED to the PWM pins as indicated
// in the #defines
// public domain, enjoy!
 
#define REDPIN 5
#define GREENPIN 6
#define BLUEPIN 3

// Patterns
#define WHITE 0
#define FADE  1
#define RGB   2
#define CHASE 3
#define VIOLET 4 

int mode = FADE;
 
#define FADESPEED 5     // make this higher to slow down
#define RGBSPEED 500  // make this higher to slow down
#define CHASESPEED 250
 
void setup() {
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
}
 
void loop() {
  int r, g, b;
 
  switch(mode) {
    case WHITE:
      setrgb(100,255,255);
      break;
    case VIOLET:
      setrgb(100,0,255);
      break;
    case CHASE:
      dochase();
      break;
    case RGB:
      dorgb();
      break;
    case FADE:
    default:
      dofade();
      break;
  }
}

void setrgb(int r, int g, int b)
{
    analogWrite(REDPIN,r);
    analogWrite(BLUEPIN,b);
    analogWrite(GREENPIN,g);
}

// Implements one instance of the FADE cycle
void dofade()
{
    int r, g, b;

    // fade from blue to violet
    for (r = 0; r < 256; r++) { 
      analogWrite(REDPIN, r);
      delay(FADESPEED);
    } 
    // fade from violet to red
    for (b = 255; b > 0; b--) { 
      analogWrite(BLUEPIN, b); delay(FADESPEED);
    } 
    // fade from red to yellow
    for (g = 0; g < 256; g++) { 
      analogWrite(GREENPIN, g); delay(FADESPEED);
    } 
    // fade from yellow to green
    for (r = 256; r > 0; r--) { 
      analogWrite(REDPIN, r); delay(FADESPEED);
    } 
    // fade from green to teal
    for (b = 0; b < 256; b++) { 
      analogWrite(BLUEPIN, b); delay(FADESPEED);
    } 
    // fade from teal to blue
    for (g = 255; g > 0; g--) { 
      analogWrite(GREENPIN, g); delay(FADESPEED);
    } 
}

// Implenments one instance of the RGB cycle
void dorgb()
{
  setrgb(255,  0,  0);  delay(RGBSPEED);
  setrgb(  0,255,  0);  delay(RGBSPEED);
  setrgb(  0,  0,255);  delay(RGBSPEED);
}

void dochase()
{
  setrgb(100,  0,  0); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(100,  0,127); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(100,  0,255); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb( 50,  0,255); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(  0,  0,255); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(  0,127,255); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(  0,255,255); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(  0,255,127); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(  0,255,  0); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb( 50,255,  0); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(100,255,  0); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
  setrgb(100,127,  0); delay(CHASESPEED);
  setrgb(  0,  0,  0); delay(CHASESPEED);
}
