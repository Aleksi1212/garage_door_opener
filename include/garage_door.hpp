#ifndef GARAGE_DOOR_HPP
#define GARAGE_DOOR_HPP

#include <memory>
#include <gpio.hpp>
#include <program_state.hpp>
#include <mqtt.hpp>

class GarageDoor
{
private:
    std::shared_ptr<ProgramState> program_state;
    std::shared_ptr<Mqtt> mqtt_client;

    GPIOPin in1;
    GPIOPin in2;
    GPIOPin in3;
    GPIOPin in4;

    GPIOPin sw0;
    GPIOPin sw1;
    GPIOPin sw2;

    GPIOPin led1;
    GPIOPin led2;
    GPIOPin led3;

    GPIOPin lim_sw1;
    GPIOPin lim_sw2;

    GPIOPin rot_a;
    GPIOPin rot_b;

    uint16_t steps_up = 0;
    uint16_t steps_down = 0;

    int curr_step = 0;

    void half_step_motor(bool reverse = false);

public:
    GarageDoor(
        std::shared_ptr<ProgramState> state,
        std::shared_ptr<Mqtt> mqtt
    );

    void calibrate_motor();
    void connect_mqtt_client();
    void remote_control();
    void local_control();

    /* TESTING */
    void reset();
    void test_mqtt();
};

void sw0_callback(uint32_t event_mask);
void sw1_callback(uint32_t event_mask);
void sw2_callback(uint32_t event_mask);
void rot_encoder_callback(uint32_t event_mask);

#endif
