#include <cstdint>
#include <gpio.hpp>
#include "pico/stdlib.h"
#include <cstdint>
#include <irq_callback.hpp>

GPIOPin::GPIOPin(
    int pin,
    int gpio_func,
    bool input,
    bool pullup,
    bool invert,
    bool callback_enabled,
    uint32_t event_mask
)
{
    if (gpio_func < 0) {
        gpio_init(pin);
        gpio_set_dir(pin, !input);
    } else {
        gpio_set_function(pin, (gpio_function_t)gpio_func);
    }

    if (pullup) {
        gpio_pull_up(pin);
    }

    if (invert) {
        gpio_set_inover(pin, GPIO_OVERRIDE_INVERT);
        gpio_set_outover(pin, GPIO_OVERRIDE_INVERT);
    }

    gpio_set_irq_enabled_with_callback(pin, event_mask, callback_enabled, &irq_callback);

    pin_num = pin;
}

bool GPIOPin::read() { return gpio_get(pin_num); }
bool GPIOPin::operator()() { return read(); }

void GPIOPin::write(bool value) { gpio_put(pin_num, value); }
void GPIOPin::operator()(bool value) { write(value); }

GPIOPin::operator int() { return pin_num; }
