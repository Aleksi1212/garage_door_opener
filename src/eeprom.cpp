#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <cstdint>
#include <eeprom.hpp>
#include <hardware.hpp>
#include <gpio.hpp>

Eeprom::Eeprom(i2c_inst_t *_i2c, uint baudrate) : i2c(_i2c)
{
    i2c_init(i2c, baudrate);

    // GPIOPin sda_pin(EEPROM_sda_pin, false, true, false, GPIO_FUNC_I2C);
    // GPIOPin scl_pin(EEPROM_scl_pin, false, true, false, GPIO_FUNC_I2C);

    GPIOPin sda_pin(EEPROM_SDA_PIN, GPIO_FUNC_I2C, false, true, false);
    GPIOPin scl_pin(EEPROM_SCL_PIN, GPIO_FUNC_I2C, false, true, false);
}

uint8_t Eeprom::read_byte(uint16_t memory_address)
{
    uint8_t buff[2];

	buff[0] = memory_address >> 8;	  // High byte
	buff[1] = memory_address & 0xFF;  // Low byte

	i2c_write_blocking(i2c, EEPROM_ADDR, buff, 2, true);  // send address

	uint8_t result;
	i2c_read_blocking(i2c, EEPROM_ADDR, &result, 1, false);

	return result;
}
void Eeprom::write_byte(uint16_t memory_address, uint8_t data)
{
    uint8_t buff[3];

	buff[0] = memory_address >> 8;	  // High byte
	buff[1] = memory_address & 0xFF;  // Low byte

	buff[2] = data;

	i2c_write_blocking(i2c, EEPROM_ADDR, buff, 3, false);
	sleep_ms(10);
}

uint16_t Eeprom::read_u16(uint16_t addr)
{
	uint16_t lo = read_byte(addr);
	uint16_t hi = read_byte(addr + 1);
	return (uint16_t) ((hi << 8) | lo);
}
void Eeprom::write_u16(uint16_t addr, uint16_t value)
{
	write_byte(addr, (uint8_t) (value & 0xFF));
	write_byte(addr + 1, (uint8_t) ((value >> 8) & 0xFF));
	sleep_ms(10);
}