#include <iostream>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <garage_door.hpp>
#include <program_state.hpp>
#include <hardware.hpp>
#include <irq_callback.hpp>
#include <pico/multicore.h>

#include <Countdown.hpp>
#include <IPStack.hpp>
#include "MQTTClient/src/MQTTClient.h"
#include <mqtt.hpp>

using namespace std;

void irq_callback(uint gpio, uint32_t event_mask)
{
    switch (gpio)
    {
        case ROT_SIG_A:
            rot_encoder_callback(event_mask);
            break;
        case SW_0:
            sw0_callback(event_mask);
            break;
        case SW_2:
            sw2_callback(event_mask);
            break;
    }
}

int main()
{
    stdio_init_all();

    cout << "\nBOOT\n" << endl;

    auto ps_ptr = make_shared<ProgramState>();
    auto mqtt_ptr = Mqtt::create();

    GarageDoor garage_door(ps_ptr, mqtt_ptr);
    garage_door.connect_mqtt_client();

    GPIOPin led1(LED_1, -1, false, false),
    led2(LED_2, -1, false, false),
    led3(LED_3, -1, false, false);

    absolute_time_t starttime = get_absolute_time();

    while (true)
    {
        auto ps = ps_ptr->read();
        cout << "Calibrated: " << (int)ps.calibrated << endl;
        cout << "Door position: " << (int)ps.door_position << endl;
        cout << "Is open: " << (int)ps.is_open << endl;
        cout << "Is running: " << (int)ps.is_running << endl;
        cout << "Steps up: " << (int)ps.steps_up << endl;
        cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

        if (!ps.calibrated) {
            uint64_t elapsed = absolute_time_diff_us(starttime, get_absolute_time());

            if ((elapsed / 300000) % 2 == 0) { // even number -> ON
                led1(true);
                led2(true);
                led3(true);
            }

            else {
                led1(false);
                led2(false);
                led3(false);
            }

            garage_door.calibrate_motor();
        } else {
            garage_door.local_control();
        }

        garage_door.reset();

        mqtt_ptr->yield(100);

        tight_loop_contents();
        sleep_ms(100);
    }

    return 0;
}
