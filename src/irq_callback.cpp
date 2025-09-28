#include <pico/stdlib.h>
#include <hardware.hpp>
#include <irq_callback.hpp>
#include <motor.hpp>

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
