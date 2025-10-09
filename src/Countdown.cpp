//
// Created by Keijo Länsikunnas on 11.2.2024.
// https://gitlab.metropolia.fi/lansk/pico-modbus/-/blob/main/mqtt/IPStack.h?ref_type=heads
//


#include <Countdown.hpp>

Countdown::Countdown() {
    target_time = make_timeout_time_ms(0);
}

Countdown::Countdown(int ms) {
    target_time = make_timeout_time_ms(ms);
}


bool Countdown::expired() {
    return time_reached(target_time);
}


void Countdown::countdown_ms(int ms) {
    target_time = make_timeout_time_ms(ms);
}


void Countdown::countdown(int seconds) {
    target_time = make_timeout_time_ms(seconds * 1000);
}


int Countdown::left_ms() {
    return static_cast<int>(absolute_time_diff_us(get_absolute_time(), target_time) / 1000);
}

