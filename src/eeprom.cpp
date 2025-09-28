#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <cstdint>
#include <eeprom.hpp>
#include <hardware.hpp>
#include <gpio.hpp>

Eeprom::Eeprom(i2c_inst_t *i2c, uint baudrate)
{
    i2c_init(i2c, baudrate);

    GPIOPin sda_pin(EEPROM_sda_pin, false, true, false, GPIO_FUNC_I2C);
    GPIOPin scl_pin(EEPROM_sda_pin, false, true, false, GPIO_FUNC_I2C);
}
