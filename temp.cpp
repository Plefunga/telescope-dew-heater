/*
 * temp.cpp
 * contains functions for reading the DHTs
 */

#include "temp.h"

/* Temp constructor
 * wraps the temperature and humidity into a struct
 * :: int t :: the temperature
 * :: int d :: the humidity
 */
Temp::Temp(int t, int h)
{
  this->temperature = t;
  this->humidity = h;
}

/* void read
 * ::SimpleDHT11 dht :: a DHT11 object to read the temperature from
 * acts in place on the object that this is called on.
 */
void Temp::read(SimpleDHT11 dht)
{
  // init variables for reading from the DHT
  int err = SimpleDHTErrSuccess;
  byte t = 0;
  byte h = 0;

  // read the dht
  err = dht.read(&t, &h, NULL);

  // if error:
  if(err != SimpleDHTErrSuccess)
  {
    Serial.print("Read DHT11 failed, err=");
    Serial.print(SimpleDHTErrCode(err));
    Serial.print(", ");
    Serial.println(SimpleDHTErrDuration(err));
    this->error = true;
    //Serial.println("trying again in 10s...");
    //delay(10000);
  }

  // set the temp and humidity of the current object
  this->temperature = (int)t;
  this->humidity = (int)h;
}

/**
 * assumes that we are using the TMP 36 connected to A0
 */
void Temp::read(float aref_level)
{

    float average_temp = 0.0;

    for(int i = 0; i < 500; i++)
    {
        float mV = aref_level * analogRead(A0)/1024.0;
        float temp = (mV - 500.0)/10.0;
        average_temp += temp;
        delay(1);
    }
    

    this->temperature = (int)(average_temp/500.0);
}

/**
 * Calibrate a TMP36
 * by modifying the 3.3 voltage lvl
 */
float Temp::calibrate(int actual)
{
    int measurement = 0.0;
    for(int i = 0; i < 1000; i++)
    {
        measurement += analogRead(A0);
        delay(2);
    }

    float estimated_temp = (3300.0 * (measurement/1000.0)/1024.0 - 500.0)/10.0;

    if(abs(estimated_temp - actual) < 20.0 && (float)measurement/1000.0 != 0)
    {
        return 1024.0 * (actual * 10.0 + 500.0) / ((float)measurement / 1000.0);
    }
    else
    {
        Serial.println(String("Calibration Failed. Reason: ") + String(abs(estimated_temp-actual) > 20 ? "Measurements too far apart: " + String(estimated_temp) + " " + String(actual) : "temp sensor measurement basically zero"));
        return 3300.0;
    }
}

/* float dew_point
 * calculates the dew point given the temperature and humidity
 * :: int temp    :: the temperature
 * :: int humidity:: the humidity
 * returns the approximate dew point
 */
float dew_point(int temperature, int humidity)
{
  return temperature - (100 - humidity)/5;
}