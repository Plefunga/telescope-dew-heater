#include "interval.h"
#include "Arduino.h"

Interval::Interval(int milliseconds)
{
    this->interval = milliseconds;
    this->last_time = millis();
}

bool Interval::has_passed()
{
    if(millis() >= last_time + this->interval)
    {
        last_time = millis();
        return true;
    }
    else
    {
        return false;
    }
}