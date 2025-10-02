#ifndef GPIO_HPP
#define GPIO_HPP

#include <cstdint>

class GPIOPin
{
    private:
        int pin_num = 0;

    public:
        GPIOPin(
            int pin,
            int gpio_func = -1,
            bool input = true,
            bool pullup = true,
            bool invert = false,
            bool callback = false,
            uint32_t event_mask = 0
            
        );
        GPIOPin(const GPIOPin &) = delete;

        bool read();
        bool operator() ();
        void write(bool value);
        void operator() (bool);
        operator int();
};

#endif