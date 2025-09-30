#ifndef EEPROM_HPP
#define EEPROM_HPP

#include <cstdint>
#include <hardware/i2c.h>

#define EEPROM_MAX_ADDRESS 0x7FFF
#define EEPROM_BAUD_RATE 100000	 // 100KHz

class Eeprom
{
    private:
        i2c_inst_t *i2c;

    public:
        Eeprom(i2c_inst_t *_i2c, uint baudrate);

        uint8_t read_byte(uint16_t memory_address);
        void write_byte(uint16_t memory_address, uint8_t data);

        uint16_t read_u16(uint16_t addr);
        void write_u16(uint16_t addr, uint16_t value);
};

#endif