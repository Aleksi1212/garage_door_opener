#include <iostream>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <motor.hpp>
#include <program_state.hpp>
#include <hardware.hpp>

using namespace std;

#define LED 20
//test2

int main()
{
    stdio_init_all();

    ProgramState program_state;
    auto ps_ptr = make_shared<ProgramState>(program_state);

    Motor motor(ps_ptr);

    // gpio_init(LED);
    // gpio_set_dir(LED, GPIO_OUT);

    while (true)
    {
        motor.calibrate();
        // gpio_put(LED, true);
        // sleep_ms(1000);
        // gpio_put(LED, false);
        // sleep_ms(1000);
    }

    return 0;
}