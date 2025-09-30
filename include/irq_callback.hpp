#ifndef IRQ_CALLBACK_HPP
#define IRQ_CALLBACK_HPP

#include "pico/stdlib.h"

void irq_callback(uint gpio, uint32_t event_mask);

#endif