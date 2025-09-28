#ifndef MOTOR_HPP
#define MOTOR_HPP

#include <memory>
#include <gpio.hpp>
#include <program_state.hpp>

class Motor
{
    private:
        std::shared_ptr<ProgramState> program_state;

        GPIOPin in1;
        GPIOPin in2;
        GPIOPin in3;
        GPIOPin in4;

        GPIOPin sw0;
        GPIOPin sw2;
        GPIOPin led;

        uint16_t steps_up;
        uint16_t steps_down;

        void half_step();

    public:
        Motor(std::shared_ptr<ProgramState> state);
        void calibrate();
};

void limit_switch_1_callback(uint32_t event_mask);
void limit_switch_2_callback(uint32_t event_mask);

#endif