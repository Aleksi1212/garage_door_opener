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

// std::shared_ptr<Mqtt> mqtt_ptr;

// void run_garage_door()
// {
//     stdio_init_all();

//     cout << "\nBOOT\n" << endl;

//     auto ps_ptr = make_shared<ProgramState>();
//     mqtt_ptr = Mqtt::create();

//     GarageDoor garage_door(ps_ptr, mqtt_ptr);
//     garage_door.connect_mqtt_client();

//     // multicore_launch_core1(core1_entry);

//     while (true)
//     {
//         auto ps = ps_ptr->read();
//         cout << "Calibrated: " << (int)ps.calibrated << endl;
//         cout << "Door position: " << (int)ps.door_position << endl;
//         cout << "Is open: " << (int)ps.is_open << endl;
//         cout << "Is running: " << (int)ps.is_running << endl;
//         cout << "Steps up: " << (int)ps.steps_up << endl;
//         cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

//         if (!ps.calibrated) {
//             garage_door.calibrate_motor();
//         } else {
//             garage_door.local_control();
//         }

//         garage_door.reset();

//         // mqtt_ptr->yield(100);

//         // tight_loop_contents();
//         sleep_ms(100);
//     }
// }

// Function to run on core1
// void core1_entry() {
//     while (true) {
//         if (mqtt_ptr && (*mqtt_ptr)()) {
//             mqtt_ptr->yield(100); // Run MQTT processing
//         } else {
//             sleep_ms(100);
//         }
//     }
// }

// repeating_timer_t mqtt_timer;

// bool mqtt_poll_callback(repeating_timer_t *t) {
//     auto mqtt = reinterpret_cast<Mqtt *>(t->user_data);
//     mqtt->yield(0);    // non-blocking poll
//     return true;       // keep repeating
// }

int main()
{
    stdio_init_all();
    cout << "\nBOOT\n" << endl;

    // multicore_launch_core1(run_garage_door);

    auto ps_ptr = make_shared<ProgramState>();
    auto mqtt_ptr = Mqtt::create();

    GarageDoor garage_door(ps_ptr, mqtt_ptr);
    garage_door.connect_mqtt_client();

    // Optionally: setup a repeating timer to poll MQTT in the background (commented)
    // add_repeating_timer_ms(100, mqtt_poll_callback, mqtt_ptr.get(), &mqtt_timer);

    // Optional multicore entry (commented)
    // multicore_launch_core1(core1_entry);

    // LED pins for calibration blink feedback
    GPIOPin led1(LED_1, -1, false, false);
    GPIOPin led2(LED_2, -1, false, false);
    GPIOPin led3(LED_3, -1, false, false);

    absolute_time_t starttime = get_absolute_time();

    while (true)
    {
        auto ps = ps_ptr->read();
        // Debug prints (left commented to avoid spamming stdout)
        // cout << "Calibrated: " << (int)ps.calibrated << endl;
        // cout << "Door position: " << (int)ps.door_position << endl;
        // cout << "Is open: " << (int)ps.is_open << endl;
        // cout << "Is running: " << (int)ps.is_running << endl;
        // cout << "Steps up: " << (int)ps.steps_up << endl;
        // cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

        if (!ps.calibrated) {
            uint64_t elapsed = absolute_time_diff_us(starttime, get_absolute_time());

            // Blink LEDs while calibrating: toggle roughly every 300ms
            if ((elapsed / 300000) % 2 == 0) { // even -> ON
                led1(true);
                led2(true);
                led3(true);
            } else {
                led1(false);
                led2(false);
                led3(false);
            }

            garage_door.calibrate_motor();
        } else {
            garage_door.local_control();
        }

        // Optional MQTT polling (non-blocking) -- kept commented to preserve single-core behaviour
        // if (absolute_time_diff_us(last_poll, get_absolute_time()) > 50000) {
        //     mqtt_ptr->yield(0);  // non-blocking
        //     last_poll = get_absolute_time();
        // }

        garage_door.reset();

        // mqtt_ptr->yield(100);

        tight_loop_contents();
        sleep_ms(100);
    }

    return 0;
}
