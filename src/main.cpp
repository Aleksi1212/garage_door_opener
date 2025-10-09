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
        case ROT_SIG_A:
            rot_encoder_callback(event_mask);
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
        auto ps = ps_ptr->read();
        cout << "Calibrated: " << (int)ps.calibrated << endl;
        cout << "Door position: " << (int)ps.door_position << endl;
        cout << "Is open: " << (int)ps.is_open << endl;
        cout << "Is running: " << (int)ps.is_running << endl;
        cout << "Steps up: " << (int)ps.steps_up << endl;
        cout << "Steps down: " << (int)ps.steps_down << "\n\n" << endl;

        if (!ps.calibrated) {
            garage_door.calibrate_motor();
        }
        else {
            garage_door.local_control();
        }
        garage_door.reset();
        sleep_ms(100);
    }

    return 0;
}