#ifndef PROGRAM_STATE_HPP
#define PROGRAM_STATE_HPP

#include <cstdint>

struct T_ProgramState
{
    uint16_t steps_up;
    uint16_t steps_down;
    uint16_t door_position;
    uint8_t is_running;
};

class ProgramState
{
    private:
        T_ProgramState state;

    public:
        ProgramState(
            uint16_t _steps_up = 0,
            uint16_t _steps_down = 0,
            uint16_t _door_position = 0,
            uint8_t _is_running = 0
        );

        void write_state(T_ProgramState& new_state);
        T_ProgramState read_state();
};

#endif
