#include "pico/stdlib.h"
#include <pico/util/queue.h>
#include <cstdint>
#include <garage_door.hpp>
#include <program_state.hpp>
#include <iostream>
#include <hardware.hpp>
#include <array>

#define STEP_SEQ_COUNT 8

static const uint8_t STEP_SEQUENCE[STEP_SEQ_COUNT][4] = {
	{1, 0, 0, 0},  // Step 1
	{1, 1, 0, 0},  // Step 2
	{0, 1, 0, 0},  // Step 3
	{0, 1, 1, 0},  // Step 4
	{0, 0, 1, 0},  // Step 5
	{0, 0, 1, 1},  // Step 6
	{0, 0, 0, 1},  // Step 7
	{1, 0, 0, 1}   // Step 8
};

static queue_t rot_encoder_queue;

void rot_encoder_callback(uint32_t event_mask)
{
    if ((event_mask & GPIO_IRQ_EDGE_RISE)) {
        uint8_t rot_direction = 1;

        if (gpio_get(ROT_SIG_B) == 0) {
            queue_try_add(&rot_encoder_queue, &rot_direction);
        } else {
            rot_direction = 0;
            queue_try_add(&rot_encoder_queue, &rot_direction);
        }   
    }
}

/*
    GarageDoor methods
*/
GarageDoor::GarageDoor(
    std::shared_ptr<ProgramState> state,
    std::shared_ptr<Mqtt> mqtt
) :
    program_state(state),
    mqtt_client(mqtt),

    in1(MOTOR_IN_1, -1, false, false),
    in2(MOTOR_IN_2, -1, false, false),
    in3(MOTOR_IN_3, -1, false, false),
    in4(MOTOR_IN_4, -1, false, false),

    sw0(SW_0, -1, true, true, true),
    sw1(SW_1, -1, true, true, true),
    sw2(SW_2, -1, true, true, true),

    led1(LED_1, -1, false, false),
    led2(LED_2, -1, false, false),
    led3(LED_3, -1, false, false),

    lim_sw1(LIMIT_SW_1, -1, true, true, true),
    lim_sw2(LIMIT_SW_2, -1, true, true, true),

    rot_a(ROT_SIG_A, -1, true, false, false, true, GPIO_IRQ_EDGE_RISE),
    rot_b(ROT_SIG_B, -1, true, false, false)
{
    queue_init(&rot_encoder_queue, sizeof(uint8_t), 10);
}

void GarageDoor::half_step_motor(bool reverse)
{
    std::array<decltype(in1)*, 4> pins = { &in1, &in2, &in3, &in4 };
    size_t pin_count = pins.size();

    for (size_t i = 0; i < pin_count; ++i) {
        int idx = reverse ? (pin_count - i) : i;
        pins[i]->write(STEP_SEQUENCE[curr_step][idx]);
    }

    curr_step = (curr_step + 1) % STEP_SEQ_COUNT;
    sleep_ms(1);
}

void GarageDoor::calibrate_motor()
{
    auto ps = program_state->read();

    led1.write(true);

    if (ps.is_running || (sw0() && sw2())) {
        ps.is_running = 1;
        program_state->write(ps);

        uint8_t rot = 0;
        while(queue_try_remove(&rot_encoder_queue, &rot));

        while(!lim_sw1()) half_step_motor();
        while(!lim_sw2()) {
            if (queue_try_remove(&rot_encoder_queue, &rot)) {
                ps.is_open = 1;
            }
            half_step_motor(true);
            ps.steps_down++;
        }
        while(!lim_sw1()) {
            if (queue_try_remove(&rot_encoder_queue, &rot)) {
                ps.is_open = 0;
            }
            half_step_motor();
            ps.steps_up++;
        }

        ps.door_position = 0;
        ps.is_running = 0;
        ps.calibrated = 1;

        program_state->write(ps);

        led1.write(false);
        led2.write(true);
    }
}

/*
    FOR TESTING
*/
void GarageDoor::reset()
{
    if (sw0() && sw1() && sw2()) {
        program_state->reset_eeprom();
    }
}

void GarageDoor::test_mqtt()
{
    if (!(*mqtt_client)()) {
        led1.write(true);
        mqtt_client->connect();
        led1.write(false);
    }

    char msg_buf[MQTT_MSG_SIZE];
    if (mqtt_client->try_get_mqtt_msg(msg_buf, sizeof(msg_buf))) {
        std::cout << "MQTT message recieved: " << msg_buf << std::endl;
    }
}
