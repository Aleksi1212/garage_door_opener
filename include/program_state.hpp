#ifndef PROGRAM_STATE_HPP
#define PROGRAM_STATE_HPP

#include <cstdint>
#include <eeprom.hpp>

struct T_ProgramState
{
    uint16_t steps_up; // steps to bring door up
    uint16_t steps_down; // steps to bring door down
    uint16_t door_position; // current door position (figure out how to use this)
    uint8_t is_running; // door currently opening / closing 0 = no, 1 = yes
    uint8_t calibrated; // motor calibrated
    uint8_t is_open; // door 1 = open, 0 = closed
};

/* --------------------------------------------------------------------
 * EEPROM layout (highest addresses reserve space for ProgramState)
 *
 * 0x7FD1  ┐
 * 0x7FD0  │  is_open               (1 byte)
 * 0x7FCF  │  ~calibrated           (1 byte)
 * 0x7FCE  │  calibrated            (1 byte)
 * 0x7FCD  │  ~is_running           (1 byte)
 * 0x7FCC  │  is_running            (1 byte)
 * 0x7FCB  ┘
 * 0x7FCA  ┐
 * 0x7FC9  │  ~door_position        (low, high)
 * 0x7FC8  │  door_position         (low, high)
 * 0x7FC7  ┘
 * 0x7FC6  ┐
 * 0x7FC5  │  ~steps_down           (low, high)
 * 0x7FC4  │  steps_down            (low, high)
 * 0x7FC3  ┘
 * 0x7FC2  ┐
 * 0x7FC1  │  ~steps_up             (low, high)
 * 0x7FC0  │  steps_up              (low, high)
 * 0x7FBF  ┘
 * ------------------------------------------------------------------ */


#define EE_ADDR_STEPS_UP           0x7FC0  // 2 bytes
#define EE_ADDR_STEPS_UP_INV       0x7FC2  // 2 bytes

#define EE_ADDR_STEPS_DOWN         0x7FC4  // 2 bytes
#define EE_ADDR_STEPS_DOWN_INV     0x7FC6  // 2 bytes

#define EE_ADDR_DOOR_POSITION      0x7FC8  // 2 bytes
#define EE_ADDR_DOOR_POSITION_INV  0x7FCA  // 2 bytes

#define EE_ADDR_IS_RUNNING         0x7FCC  // 1 byte
// its inverse is always +1
// #define EE_ADDR_IS_RUNNING_INV   0x7FCD

#define EE_ADDR_CALIBRATED         0x7FCE  // 1 byte
// #define EE_ADDR_CALIBRATED_INV   0x7FCF

#define EE_ADDR_IS_OPEN            0x7FD0  // 1 byte
// #define EE_ADDR_IS_OPEN_INV      0x7FD1



class ProgramState : private Eeprom
{
    private:
        T_ProgramState state;

        void write_to_eeprom();
        void load_from_eeprom();

    public:
        ProgramState();

        void write(const T_ProgramState& new_state);
        T_ProgramState read();

        void reset_eeprom();
};

#endif
