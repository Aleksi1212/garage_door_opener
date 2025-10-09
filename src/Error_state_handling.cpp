#include "Error_state_handling.hpp"
#include <gpio.hpp>
#include <hardware.hpp>
#include <iostream>
#include <ostream>
#include <pico/util/queue.h>

using namespace std;

error_status::error_status():
    led1(LED_1, -1, false, false),
    led2(LED_2, -1, false, false),
    led3(LED_3, -1, false, false)
{}

void error_status::report() {
    std::cout << "Door STUCK!" << std::endl;
        led1.write(true);
        led2.write(true);
        led3.write(true);

}




