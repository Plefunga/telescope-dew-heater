/*
 * temp.h
 * header file for the temperature class and dew point function
 */

#ifndef TEMP_H
#define TEMP_H 1

#include <Arduino.h>
#include <SimpleDHT.h>

/* Temp
 * wraps the temperature and humidity into a class
 * 
 */
typedef class Temp
{
  public:
    int temperature;
    int humidity;
    Temp(int t, int d);
    void read(SimpleDHT11 dht);

    float calibrate(int actual);

    void read(float aref_level);

    bool error = false;

} Temp;

/* float dew_point
 * calculates the dew point given the temperature and humidity
 * :: int temp    :: the temperature
 * :: int humidity:: the humidity
 * returns the approximate dew point
 */
float dew_point(int temp, int humidity);

#endif