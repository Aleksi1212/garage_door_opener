#include <iostream>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <garage_door.hpp>
#include <program_state.hpp>
#include <hardware.hpp>
#include <irq_callback.hpp>

#include <Countdown.hpp>
#include <IPStack.hpp>
#include "MQTTClient/src/MQTTClient.h"
#include "network_info.h"

using namespace std;

void irq_callback(uint gpio, uint32_t event_mask)
{
    switch (gpio)
    {
        case ROT_SIG_A:
            rot_encoder_callback(event_mask);
            break;
    }
}

void messageArrived(MQTT::MessageData &md) {
    MQTT::Message &message = md.message;

    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n",
           message.qos, message.retained, message.dup, message.id);
    printf("Payload %s\n", (char *) message.payload);
}


int main()
{
    stdio_init_all();

    cout << "\nBOOT\n" << endl;

    auto ps_ptr = make_shared<ProgramState>();

    GarageDoor garage_door(ps_ptr);

    GPIOPin led1(LED_1, -1, false, false);
    GPIOPin led2(LED_2, -1, false, false);
    GPIOPin led3(LED_3, -1, false, false);

    led1.write(true);
    led2.write(true);
    led3.write(true);


    const char *command_topic = "garage/door/command";
    const char *response_topic = "garage/door/response";
    const char *status_topic = "garage/door/status";

    IPStack ipstack(SSID, PW);
    auto client = MQTT::Client<IPStack, Countdown>(ipstack);

    int rc = ipstack.connect(HOSTNAME, PORT);
    if (rc != 1) {
        cout << "rc from TCP connect is " << rc << endl;
    }

    led1.write(true);
    led2.write(false);
    led3.write(false);

    cout << "MQTT connecting..." << endl;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)"PicoW-sample";

    rc = client.connect(data);
    if (rc != 0) {
        cout << "rc from MQTT connect is " << rc << endl;
        while (true) {
            tight_loop_contents();

            led1.write(true);
            led2.write(true);
            led3.write(true);
            sleep_ms(500);
            led1.write(false);
            led2.write(false);
            led3.write(false);
            sleep_ms(500);

        }
    }
    cout << "MQTT connected" << endl;
    led1.write(false);
    led2.write(true);
    led3.write(false);

    rc = client.subscribe(command_topic, MQTT::QOS2, messageArrived);
    if (rc != 0) {
        cout << "rc from MQTT subscribe is " << rc << endl;
    }
    led1.write(false);
    led2.write(false);
    led3.write(true);
    // cout << "MQTT subscribed to " << command_topic << endl;


    while (true)
    {
        cout << "MQTT subscribed to " << command_topic << endl;

        // auto ps = ps_ptr->read();
        // cout << "Calibrated: " << (int)ps.calibrated << endl;
        // cout << "Door position: " << (int)ps.door_position << endl;
        // cout << "Is open: " << (int)ps.is_open << endl;
        // cout << "Is running: " << (int)ps.is_running << endl;
        // cout << "Steps up: " << (int)ps.steps_up << endl;
        // cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

        // if (!ps.calibrated) {
        //     garage_door.calibrate_motor();
        // }
        // garage_door.reset();
        sleep_ms(100);
    }

    return 0;
}