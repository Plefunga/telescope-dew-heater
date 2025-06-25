#ifndef INTERVAL_H
#define INTERVAL_H 1

#include "Arduino.h"

class Interval
{
    public:
    Interval(int milliseconds);

    bool has_passed();

    private:
    int last_time;
    int interval;
};
#endif