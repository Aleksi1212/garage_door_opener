#include <program_state.hpp>
#include <cstdint>
#include <eeprom.hpp>
#include <hardware.hpp>
#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <iostream>

ProgramState::ProgramState() : Eeprom(EEPROM_PORT, EEPROM_BAUD_RATE)
{
    state.steps_up = 0;
    state.steps_down = 0;
    state.door_position = 0;
    state.is_running = 0;
    state.calibrated = 0;
    state.is_open = 0;

    load_from_eeprom();
}

void ProgramState::write_to_eeprom()
{
    std::cout << "writing..." << std::endl;
    
    Eeprom::write_u16(EE_ADDR_STEPS_UP_INV, ~state.steps_up);
    Eeprom::write_u16(EE_ADDR_STEPS_UP, state.steps_up);

    Eeprom::write_u16(EE_ADDR_STEPS_DOWN_INV, ~state.steps_down);
    Eeprom::write_u16(EE_ADDR_STEPS_DOWN, state.steps_down);

    Eeprom::write_u16(EE_ADDR_DOOR_POSITION_INV, ~state.door_position);
    Eeprom::write_u16(EE_ADDR_DOOR_POSITION, state.door_position);

    Eeprom::write_byte(EE_ADDR_IS_RUNNING + 1, ~state.is_running);
    Eeprom::write_byte(EE_ADDR_IS_RUNNING, state.is_running);

    Eeprom::write_byte(EE_ADDR_CALIBRATED + 1, ~state.calibrated);
    Eeprom::write_byte(EE_ADDR_CALIBRATED, state.calibrated);

    Eeprom::write_byte(EE_ADDR_IS_OPEN + 1, ~state.is_open);
    Eeprom::write_byte(EE_ADDR_IS_OPEN, state.is_open);

    std::cout << "written." << std::endl;
    
}

void ProgramState::load_from_eeprom()
{
    std::cout << "loading..." << std::endl;

    uint16_t steps_up = Eeprom::read_u16(EE_ADDR_STEPS_UP);
    uint16_t steps_up_inv = Eeprom::read_u16(EE_ADDR_STEPS_UP_INV);

    if (steps_up == (uint16_t)(~steps_up_inv)) {
        state.steps_up = steps_up;
    }

    /* Load steps_down*/
    uint16_t steps_down = Eeprom::read_u16(EE_ADDR_STEPS_DOWN);
    uint16_t steps_down_inv = Eeprom::read_u16(EE_ADDR_STEPS_DOWN_INV);

    if (steps_down == (uint16_t)(~steps_down_inv)) {
        state.steps_down = steps_down;
    }

    /* Load door_position*/
    uint16_t door_position = Eeprom::read_u16(EE_ADDR_DOOR_POSITION);
    uint16_t door_position_inv = Eeprom::read_u16(EE_ADDR_DOOR_POSITION_INV);

    if (door_position == (uint16_t)(~door_position_inv)) {
        state.door_position = door_position;
    }

    /* Load is_running*/
    uint8_t is_running = Eeprom::read_byte(EE_ADDR_IS_RUNNING);
    uint8_t is_running_inv = Eeprom::read_byte(EE_ADDR_IS_RUNNING + 1);

    if ((uint8_t)(is_running ^ is_running_inv) == 0xFF) {
        state.is_running = is_running;
    }

    /* Load calibrated*/
    uint8_t calibrated = Eeprom::read_byte(EE_ADDR_CALIBRATED);
    uint8_t calibrated_inv = Eeprom::read_byte(EE_ADDR_CALIBRATED + 1);

    if ((uint8_t)(calibrated ^ calibrated_inv) == 0xFF) {
        state.calibrated = calibrated;
    }

    /* Load is_open*/
    uint8_t is_open = Eeprom::read_byte(EE_ADDR_IS_OPEN);
    uint8_t is_open_inv = Eeprom::read_byte(EE_ADDR_IS_OPEN + 1);

    if ((uint8_t)(is_open ^ is_open_inv) == 0xFF) {
        state.is_open = is_open;
    }
    std::cout << "loaded." << std::endl;
}

void ProgramState::write(const T_ProgramState& new_state)
{
    state = new_state;
    write_to_eeprom();
}

T_ProgramState ProgramState::read()
{
    load_from_eeprom();
    return state;
}

// for testing
void ProgramState::reset_eeprom()
{
    state.steps_up = 0;
    state.steps_down = 0;
    state.door_position = 0;
    state.is_running = 0;
    state.calibrated = 0;
    state.is_open = 0;

    write_to_eeprom();
}
