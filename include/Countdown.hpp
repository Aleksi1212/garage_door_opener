#ifndef COUNTDOWN_HPP
#define COUNTDOWN_HPP
#include "pico/time.h"


class Countdown
{
public:
    Countdown();
    explicit Countdown(int ms);
    bool expired();
    void countdown_ms(int ms);
    void countdown(int seconds);
    int left_ms();
private:
    absolute_time_t target_time;
};


#endif //COUNTDOWN_HPP
