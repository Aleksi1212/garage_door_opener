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

    // Init leds
    GPIOPin led1(LED_1, -1, false, false);
    GPIOPin led2(LED_2, -1, false, false);
    GPIOPin led3(LED_3, -1, false, false);

    led1(true);

    // Init program state and mqtt
    auto ps_ptr = make_shared<ProgramState>();
    
    auto ps = ps_ptr->read();
    auto mqtt_ptr = Mqtt::create();
    weak_ptr<Mqtt> weak_mqtt = mqtt_ptr;
    
    // Add observer to program_state that sends a message to "garage/door/status"
    ps_ptr->add_write_observer([weak_mqtt](const T_ProgramState& state) {
        if (auto mqtt = weak_mqtt.lock()) {
            if ((*mqtt)()) {
                if (state.is_door_stuck) {
                    mqtt->send_message(STATUS_TOPIC, "DOOR STUCK :(");
                } else {
                    mqtt->send_message(STATUS_TOPIC,
                        "Calibrated: %s, Door state: %s",
                        state.calibrated ? "TRUE" : "FALSE",
                        (state.door_position > 0 && state.door_position < (state.steps_down + state.steps_up) / 2) ? "IN BETWEEN" :
                            state.is_open ? "OPEN" : "CLOSED"
                    );
                }
            }
        }
    });

    // Init garage door
    GarageDoor garage_door(ps_ptr, mqtt_ptr);
    garage_door.connect_mqtt_client();

    // Send initial status to "garage/door/status" topic on mqtt server
    if ((*mqtt_ptr)()) {
        led3(true);
        mqtt_ptr->send_message(STATUS_TOPIC,
            "Calibrated: %s\nDoor state %s\nError state: %s",
            ps.calibrated ? "TRUE" : "FALSE",
            (ps.door_position > 0 && ps.door_position < ps.steps_down) ? "IN BETWEEN" :
                ps.is_open ? "OPEN" : "CLOSED",
            ps.is_door_stuck ? "DOOR STUCK" : "NO ERROR"
        );
    }

    absolute_time_t starttime = get_absolute_time();

    while (true)
    {
        ps = ps_ptr->read();
        cout << "Calibrated: " << (int)ps.calibrated << endl;
        cout << "Door position: " << (int)ps.door_position << endl;
        cout << "Is open: " << (int)ps.is_open << endl;
        cout << "Is running: " << (int)ps.is_running << endl;
        cout << "Steps up: " << (int)ps.steps_up << endl;
        cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

        if (!ps.calibrated) {
            uint64_t elapsed = absolute_time_diff_us(starttime, get_absolute_time());

            if ((elapsed / 300000) % 2 == 0) { // even -> ON
                led1(true);
                if (ps.is_door_stuck) {
                    led2(true);
                    led3(true);
                }
            } else {
                led1(false);
                led2(false);
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
