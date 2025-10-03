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
 * 0x7FFF  ┐
 * 0x7FFE  │  steps_up           (low, high)
 * 0x7FFD  │  ~steps_up          (low, high)
 * 0x7FFC  ┘
 * 0x7FFB  ┐
 * 0x7FFA  │  steps_down         (low, high)
 * 0x7FF9  │  ~steps_down        (low, high)
 * 0x7FF8  ┘
 * 0x7FF7  ┐
 * 0x7FF6  │  door_position      (low, high)
 * 0x7FF5  │  ~door_position     (low, high)
 * 0x7FF4  ┘
 * 0x7FF3  is_running            (1 byte)
 * 0x7FF2  ~is_running           (1 byte)
 * 0x7FF1  calibrated            (1 byte)
 * 0x7FF0  ~calibrated           (1 byte)
 * 0x7FEF  is_open               (1 byte)
 * 0x7FEE  ~is_open              (1 byte)
 * ------------------------------------------------------------------ */

#define EE_ADDR_IS_OPEN          0x7FEF  // 1 byte

#define EE_ADDR_CALIBRATED       0x7FF1  // 1 byte

#define EE_ADDR_IS_RUNNING       0x7FF3  // 1 byte


#define EE_ADDR_DOOR_POSITION    0x7FF6  // 2 bytes
#define EE_ADDR_DOOR_POSITION_INV 0x7FF4 // 2 bytes

#define EE_ADDR_STEPS_DOWN       0x7FFA  // 2 bytes
#define EE_ADDR_STEPS_DOWN_INV   0x7FF8  // 2 bytes

#define EE_ADDR_STEPS_UP         0x7FFE  // 2 bytes
#define EE_ADDR_STEPS_UP_INV     0x7FFC  // 2 bytes




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
