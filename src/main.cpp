#include <iostream>
#include <pico/stdlib.h>
#include <hardware/gpio.h>

using namespace std;

#define LED 20
//test

int main()
{
    stdio_init_all();

    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    while (true)
    {
        gpio_put(LED, true);
        sleep_ms(1000);
        gpio_put(LED, false);
        sleep_ms(1000);
    }

    return 0;
}