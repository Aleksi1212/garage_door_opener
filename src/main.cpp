#include <iostream>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <garage_door.hpp>
#include <program_state.hpp>
#include <hardware.hpp>
#include <irq_callback.hpp>

using namespace std;

void irq_callback(uint gpio, uint32_t event_mask)
{
    switch (gpio)
    {
        case LIMIT_SW_1:
            limit_switch_1_callback(event_mask);
            break;
        case LIMIT_SW_2:
            limit_switch_2_callback(event_mask);
            break;
    }
}

int main()
{
    stdio_init_all();

    auto ps_ptr = make_shared<ProgramState>();

    GarageDoor garage_door(ps_ptr);

    while (true)
    {
        T_ProgramState ps = ps_ptr->read();
        cout << "Calibrated: " << (int)ps.calibrated << endl;
        cout << "Door position: " << (int)ps.door_position << endl;
        cout << "Is running: " << (int)ps.is_running << endl;
        cout << "Steps up: " << (int)ps.steps_up << endl;
        cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

        if (!ps.calibrated) {
            garage_door.calibrate_motor();
        }
        garage_door.reset();
        sleep_ms(100);
    }

    return 0;
}