#include <program_state.hpp>
#include <cstdint>

ProgramState::ProgramState(
    uint16_t _steps_up,
    uint16_t _steps_down,
    uint16_t _door_position,
    uint8_t _is_running
)
{
    state.steps_up = _steps_up;
    state.steps_down = _steps_down;
    state.door_position = _door_position;
    state.is_running = _is_running;
}

void ProgramState::write_state(T_ProgramState& _state)
{
    state = _state;
}

T_ProgramState ProgramState::read_state()
{
    return state;
}
