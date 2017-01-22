/* 
 * Example program for Arduino external interrupts, in this case to read
 * an anemometer connected to digital pin 2 (which is INT0), and a rain
 * gauge connected to digital pin 3 (which is INT1).
 *
 * Author: David Bryant (david@orangemoose.com)
 */

volatile int anemometerCnt = 0;  // Counts anemometer interrupts
volatile int raingaugeCnt  = 0;  // Counts rain gauge interrupts

#define WIND_FACTOR 1.492  // One anemometor rotation in one second = 1.492 MPH
#define RAIN_FACTOR 0.011  // One rain gauge measurement = 0.011" of rain

unsigned long sampleDelayMs = 30*1000;  // Sampling interval (milliseconds)
unsigned long prevSampleMs = 0;         // Timestamp for previous sample

void setup()
{
  Serial.begin(9600);
  Serial.println("Gathering data...");
  
  // Attach anemometer interrupt service routine to INT0
  attachInterrupt(0,anemometer_cnt,RISING);
  attachInterrupt(1,raingauge_cnt,RISING);
}

void loop()
{
  float windspeed, rainfall;
  
  unsigned long currentMillis = millis();    // Get current timer value
  /* See if it is time to calculate wind speed */
  if( (currentMillis - prevSampleMs) >= sampleDelayMs) {
    
    windspeed = anemometerCnt * WIND_FACTOR * 1000 / sampleDelayMs;
    Serial.print("Anemometer: ");
    Serial.print(windspeed);
    Serial.print(" MPH (");
    Serial.print(anemometerCnt);
    Serial.println(" rotations)" );
    
    rainfall = raingaugeCnt * RAIN_FACTOR;
    Serial.print("Rain Gauge: ");
    Serial.print(rainfall);
    Serial.print(" inches (");
    Serial.print(raingaugeCnt);
    Serial.println(" buckets)");
 
    prevSampleMs = currentMillis;   // Remember clock time
    anemometerCnt = raingaugeCnt = 0;               // Reset counters
  }
}

/* 
 * Interrupt service routine called when the anemometer switch closes.
 * All it needs to do is increment the interrupt counter.
 */
void anemometer_cnt() 
{
  anemometerCnt++;
}

/* 
 * Interrupt service routine called when the rain gauge switch closes.
 * All it needs to do is increment the interrupt counter.
 */
void raingauge_cnt() 
{
  raingaugeCnt++;
}
