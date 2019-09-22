#ifndef LEDControl_h
#define LEDControl_h

// Display modes for the LEDs
#define MODE_UNDEF  0
#define MODE_OFF    1
#define MODE_ON     2
#define MODE_RUNFWD 3
#define MODE_RUNREV 4
#define MODE_RAINBF 5
#define MODE_RAINBR 6
#define MODE_CYLON  7
#define MODE_BITMAP 8
#define MODE_MARQUEE	9
#define NUM_MODES   10

#include "Arduino.h"
class LEDControl
{
  public:
    LEDControl(int num_leds, CRGB leds[]);
    int getMode();
    void setOneColor(CRGB color);
    void setRunFwd(CRGB color);
    void setRunRev(CRGB color);
    void setCylon(CRGB color);
    void setRainbowFwd();
    void setRainbowRev();
    void setPattern(CRGB color, unsigned long bitmap);
    void setProgress(CRGB color, int percent);
    void setMarquee(CRGB color, unsigned long bitmap);
    void shiftFwd();
    void shiftRev();
    void update();
  private:
    int _ledCount;
    CRGB *_leds;
    boolean _newMode;
    int _mode;
    CRGB _color;
    int _curdir;  // Used to keep track of direction in bi-directional runs
    unsigned long _bitmap;  // limited to 32 leds
    int _config;
};

#endif
