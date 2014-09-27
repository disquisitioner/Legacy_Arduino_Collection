/*
 * Binary Thermometer
 * Read temperature from TMP36 sensor & display in binary using LEDs
 *
 * Author: David Bryant (david@orangemoose.com)
 * Date: January 29, 2012
 *
 * This was one of my first Arduino programs, having gotten a Starter Kit for
 * Christmas 2011 that included an Arduino Uno, a TMP36 analog temperature sensor, 
 * and some LEDs.  The first step was to get readings from the TMP36, which proved
 * pretty easy but I had no direct means of displaying the temperature readings
 * other than via the Serial monitor and that required having the Arduino connected
 * to a computer.  I wanted to wander around the house and take readings outside,
 * in the refrigerator/freezer, etc. but unfortunately my Starter Kit didn't include
 * a display of any kind.  It then occurred to me that I could use LEDs and display
 * the temperature in binary, and this program was born.
 * 
 * I still remember the excitement of using the program to read the temperature in
 * our freezer, having built a probe out of the TMP36 by inserting it in an old
 * transistor socket soldered to three long pieces of wire.  That same evening I
 * ordered an LCD display so I could build something more practical.
 */
 
boolean debugging = true;   /* If debugging, output via Serial monitor */
 
int sensorPin = 0;  /* Pin the TMP36 sensor output is connected to */

int pins[] = {13,12,11,10,9};  /* LED pins */
int numpins = 5;

int i = 0;

void setup() {                
  // initialize the digital output pins
  for(i=0;i<numpins;i++) {
    pinMode(pins[i],OUTPUT);
  }
  
  // send debugging info via the Serial monitor
  Serial.begin(9600);
}

/* Main execution (infinite) loop */

void loop()
{
  int reading, displayTemp;
  float voltage, temperatureC, temperatureF;
  
  /* Read temperature sensor */
  reading = analogRead(sensorPin);
  
  /* Convert reading to voltage */
  voltage = (reading * 5.0);
  voltage /= 1024.0;
   
  /* Convert to temperature */
  temperatureC = (voltage - 0.5) * 100;  // 10mV per degree with 500mV offset
  temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  
  displayTemp = (int) (temperatureC + 0.5);
  
  if(debugging) {
    Serial.print(displayTemp); Serial.print(" degrees C [");
    Serial.print(temperatureC); Serial.print(" C, ");
    Serial.print(temperatureF); Serial.print(" F, (");
    Serial.print(voltage); Serial.println(" volts)]");
  }
  
  displayLED(displayTemp);
  
  delay(1000);   /* Delay */
}

void displayLED(int value)
{
  /* Check value range */
  if(value < 0) value = 0;
  
  for(i=0;i<numpins;i++) {
      if(value & (1<<i)) digitalWrite(pins[i],HIGH);
      else               digitalWrite(pins[i],LOW);
  } 
}
