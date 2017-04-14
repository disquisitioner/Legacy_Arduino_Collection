/*
 * Serial test loop, intended to be used as a data generator for 
 * simulating the behavior of a weather station sending readings
 * periodically via an Arduino serial output.  That Arduino would
 * be connected to another system (Raspberry Pi, PC) which would
 * read the data coming in over the serial port, process it locally
 * as required (logging, etc.) then forwarding it to an internet
 * service such as Xively or Dweet.io.
 * 
 * Author: David Bryant (david@orangemoose.com)
 * Version: 1.0 (April 14, 2017)
 * 
 * Simulated data is output as follows
 * SAMPLE#,OUTTEMP,BP,INTEMP,WINDSPEED,WINDGUST,RAINFALL
 * where:
 *    SAMPLE# = index number of current sample from 1-9999, reset to 0 when it
 *              rolls over or the sensor subsystem is restarted
 *    OUTTEMP = Outdoor temperature in degrees Farenheit, averaged over the
 *              reporting interval
 *    BP      = Barometric pressure in inches of mercury, averaged over the
 *              reporting interval
 *    INTEMP  = Indoor temperature in degrees Farenheit, averaged over the
 *              reporting interval
 *    WINDSPEED = Wind speed in MPH, averaged over the reporting interval
 *    WINDGUST = Maxiumum wind speed observed during the reporting interval
 *    RAINFALL = Rainfall in inches during the reporting interval
 */

const unsigned long sampleDelayMs = 5000;   /* Sample delay in milliseconds */

int samplenum = 0;
float otemp, itemp, bp, windspeed, windgust, rainfall;
float otarget,itarget;
boolean omode,imode;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  randomSeed(analogRead(0));

  otemp   = 60.0 + random(-20,20)/5.0;
  otarget = 90.0 - random(100)/10.0;
  omode = true;

  itemp   = 60.0 + random(-20,20)/5.0;
  itarget = 90.0 - random(100)/10.0;
  imode = true;
  
  bp = 27.0 + random (20)/10.0;
  windspeed = 1.0 + random(10);
  windgust = windspeed + random(20);
  rainfall = 0.0 + random(1);
}



// the loop routine runs over and over again forever:
void loop() {
  
  /* Generate reasonable random outside temperatures */
  if(omode) { /* Rising */
      otemp = otemp + random(-50,200)/100.0;
      if(otemp > otarget) {
        omode = false;
        otarget = 40.0 + random(100)/10.0;
      }
  }
  else { /* Falling */
    otemp = otemp - random(-50,200)/100.0;
    if(otemp < otarget) {
      omode = true;
      otarget = 90.0 - random(100)/10.0;
    }
  }

    /* Generate reasonable random inside temperatures */
  if(imode) { /* Rising */
      itemp = itemp + random(-50,75)/100.0;
      if(itemp > itarget) {
        imode = false;
        itarget = 58.0 + random(80)/10.0;
      }
  }
  else { /* Falling */
    itemp = itemp - random(-50,75)/100.0;
    if(itemp < itarget) {
      imode = true;
      itarget = 74.0 - random(80)/10.0;
    }
  }
  
  bp = 27.0 + (random_n(0,200)/100.0);
  windspeed = 3.0 + random_n(-10,40)/10.0;

  /* Trying to be clever about calculating wind gust */
  float g = (random_n(0,500)/10.0) - 25.0;
  if(g<0.0) windgust = windspeed - g;
  else      windgust = windspeed + g;
  
  rainfall = 0.0 + random_n(0,50)/100.0;
  Serial.print(samplenum); Serial.print(",");
  Serial.print(otemp);     Serial.print(",");
  Serial.print(bp);        Serial.print(",");
  Serial.print(itemp);     Serial.print(",");
  Serial.print(windspeed); Serial.print(",");
  Serial.print(windgust);  Serial.print(",");
  Serial.println(rainfall);
  samplenum++;
  delay(sampleDelayMs);        // delay between serial data output
}

float random_tempF(float prev,float maxF, float minF)
{
  float newF;
  int delta;

  delta = maxF - minF;
  newF = prev - random(-1*delta,delta)/200.0;
  if(newF > maxF) newF = maxF - 1.0;
  if(newF < minF) newF = minF + 1.0;
  return(newF);
}

long random_n(int minval, int maxval)
{
  int i;
  long n=0;
  
  for(i=0;i<10;i++) {
    n += random(minval,maxval);
  }
  n /= 10;
    
  return(n);
}


