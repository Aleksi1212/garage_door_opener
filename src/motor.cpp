#include "pico/stdlib.h"
#include <pico/util/queue.h>
#include <cstdint>
#include <motor.hpp>
#include <program_state.hpp>
#include <iostream>
#include <hardware.hpp>
#include <vector>

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
    Motor methods
*/
Motor::Motor(std::shared_ptr<ProgramState> state) :
program_state(state),
in1(MOTOR_IN_1, false, false),
in2(MOTOR_IN_2, false, false),
in3(MOTOR_IN_3, false, false),
in4(MOTOR_IN_4, false, false),
led(LED_1, false, false),
sw0(SW_0, true, true, true),
sw2(SW_2, true, true, true)
{
    T_ProgramState ps = program_state->read_state();
    steps_up = ps.steps_up;
    steps_down = ps.steps_down;

    queue_init(&limit_switch_1_queue, sizeof(uint8_t), 10);
    queue_init(&limit_switch_2_queue, sizeof(uint8_t), 10);

    if (steps_up <= 0 && steps_down <= 0) {
        led.write(true);
    } 
}

void Motor::half_step()
{
    in1.write(STEP_SEQUENCE[steps_up][3]);
    in2.write(STEP_SEQUENCE[steps_up][2]);
    in3.write(STEP_SEQUENCE[steps_up][1]);
    in4.write(STEP_SEQUENCE[steps_up][0]);

    steps_up = (steps_up + 1) % STEP_SEQ_COUNT;
    sleep_ms(1);
}

void Motor::calibrate()
{
    if (sw0() && sw2()) {
        half_step();
        // std::cout << "Calibrating..." << std::endl;
        // if (led()) led.write(false);
    }
}
