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
    // garage_door.reset();
    garage_door.connect_mqtt_client();

    while (true)
    {
        auto ps = ps_ptr->read();

        if (!ps.calibrated) {
            garage_door.calibrate_motor();
        } else {
            // Local physical control (e.g., button press)
            garage_door.local_control();
        }

        // Handle remote MQTT control (open/close via network)
        garage_door.remote_control();

        // Optional: keep MQTT alive
        mqtt_ptr->yield(100);

        // Optional: EEPROM reset/testing (comment out if not needed)
        // garage_door.reset();
        // garage_door.test_mqtt();

        tight_loop_contents();
        sleep_ms(100);
    }

    return 0;
}
