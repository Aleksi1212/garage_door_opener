#ifndef EEPROM_HPP
#define EEPROM_HPP

#include <cstdint>

#define EEPROM_MAX_ADDRESS 0x7FFF
#define EEPROM_BAUD_RATE 100000	 // 100KHz

class Eeprom
{
    public:
        Eeprom(i2c_inst_t *i2c, uint baudrate);

        // void write_byte(uint16_t memory_address, uint8_t data);
        // uint8_t read_byte(uint16_t memory_address);

        // void write_u16(uint16_t addr, uint16_t value);
        // uint16_t read_u16(uint16_t addr);
};

#endif