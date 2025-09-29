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

static queue_t limit_switch_1_queue;
static queue_t limit_switch_2_queue;

void limit_switch_1_callback(uint32_t event_mask)
{
    if ((event_mask & GPIO_IRQ_EDGE_FALL)) {
        uint8_t hit_right = 1;
        queue_try_add(&limit_switch_1_queue, &hit_right);
    }
}
void limit_switch_2_callback(uint32_t event_mask)
{
    if ((event_mask & GPIO_IRQ_EDGE_FALL)) {
        uint8_t hit_left = 1;
        queue_try_add(&limit_switch_2_queue, &hit_left);
    }
}

/*
    GarageDoor methods
*/
GarageDoor::GarageDoor(std::shared_ptr<ProgramState> state) :
    program_state(state),

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

    lim_sw1(LIMIT_SW_1, -1, true, true, true, true, GPIO_IRQ_EDGE_FALL),
    lim_sw2(LIMIT_SW_2, -1, true, true, true, true, GPIO_IRQ_EDGE_FALL)
{}

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
    T_ProgramState ps = program_state->read();

    led1.write(true);

    if ((sw0() && sw2())) {
        ps.is_running = 1;
        program_state->write(ps);

        while(!lim_sw1()) half_step_motor();
        while(!lim_sw2()) {
            half_step_motor(true);
            steps_up++;
        }
        while(!lim_sw1()) {
            half_step_motor();
            steps_down++;
        }

        ps.steps_up = steps_up;
        ps.steps_down = steps_down;
        ps.door_position = 0;
        ps.is_running = 0;
        ps.calibrated = 1;

        program_state->write(ps);

        led1.write(false);
        led2.write(true);
    }
}

// for testing
void GarageDoor::reset()
{
    if (sw1()) {
        program_state->reset_eeprom();
    }
}
