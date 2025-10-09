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
#include "pico/time.h"

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

    GPIOPin led1(LED_1, -1, false, false);
    GPIOPin led2(LED_2, -1, false, false);
    GPIOPin led3(LED_3, -1, false, false);

    led1(true);

    auto ps_ptr = make_shared<ProgramState>();
    
    auto ps = ps_ptr->read();
    auto mqtt_ptr = Mqtt::create();
    weak_ptr<Mqtt> weak_mqtt = mqtt_ptr;
    
    ps_ptr->add_write_observer([weak_mqtt](const T_ProgramState& state) {
        if (auto mqtt = weak_mqtt.lock()) {
            if ((*mqtt)()) {
                mqtt->send_message(STATUS_TOPIC,
                    "Calibrated: %s\nDoor state %s\nError state: %s",
                    state.calibrated ? "TRUE" : "FALSE",
                    (state.door_position > 0 && state.door_position < state.steps_down) ? "IN BETWEEN" :
                        state.is_open ? "OPEN" : "CLOSED",
                    state.is_door_stuck ? "DOOR STUCK" : "NO ERROR"
                );
            }
        }
    });

    GarageDoor garage_door(ps_ptr, mqtt_ptr);
    garage_door.connect_mqtt_client();

    if ((*mqtt_ptr)()) led3(true);

    absolute_time_t starttime = get_absolute_time();

    while (true)
    {
        ps = ps_ptr->read();
        // Debug prints (left commented to avoid spamming stdout)
        // cout << "Calibrated: " << (int)ps.calibrated << endl;
        // cout << "Door position: " << (int)ps.door_position << endl;
        // cout << "Is open: " << (int)ps.is_open << endl;
        // cout << "Is running: " << (int)ps.is_running << endl;
        // cout << "Steps up: " << (int)ps.steps_up << endl;
        // cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

        if (!ps.calibrated) {
            // led1(true);
            // led2(false);
            // led3(false);

            uint64_t elapsed = absolute_time_diff_us(starttime, get_absolute_time());

            // Blink LEDs while calibrating: toggle roughly every 300ms
            if ((elapsed / 300000) % 2 == 0) { // even -> ON
                led1(true);
                if (ps.is_door_stuck) {
                    led2(true);
                    led3(true);
                }
            } else {
                led1(false);
                led2(false);
                led3(false);
            }

            garage_door.calibrate_motor();
        } else {
            led1(false);
            led2(true);
            garage_door.control_motor();
        }

        garage_door.reset();

        tight_loop_contents();
        sleep_ms(100);
    }

    return 0;
}
