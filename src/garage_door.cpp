#include "pico/stdlib.h"
#include <pico/util/queue.h>
#include <cstdint>
#include <garage_door.hpp>
#include <program_state.hpp>
#include <iostream>
#include <hardware.hpp>
#include <array>

#define STEP_SEQ_COUNT 8

static const uint8_t STEP_SEQUENCE[STEP_SEQ_COUNT][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

static queue_t rot_encoder_queue;
static queue_t sw0_queue;
static queue_t sw1_queue;
static queue_t sw2_queue;

void rot_encoder_callback(uint32_t event_mask)
{
    if ((event_mask & GPIO_IRQ_EDGE_RISE)) {
        uint8_t rot_direction = 1;

        if (gpio_get(ROT_SIG_B) == 0) {
            queue_try_add(&rot_encoder_queue, &rot_direction);
        } else {
            rot_direction = 0;
            queue_try_add(&rot_encoder_queue, &rot_direction);
        }
    }
}

void sw0_callback(uint32_t event_mask)
{
    if ((event_mask & GPIO_IRQ_EDGE_FALL)) {
        uint8_t event = 1;
        queue_try_add(&sw0_queue, &event);
    }
}
void sw1_callback(uint32_t event_mask)
{
    if ((event_mask & GPIO_IRQ_EDGE_FALL)) {
        uint8_t event = 1;
        queue_try_add(&sw1_queue, &event);
    }
}
void sw2_callback(uint32_t event_mask)
{
    if ((event_mask & GPIO_IRQ_EDGE_FALL)) {
        uint8_t event = 1;
        queue_try_add(&sw2_queue, &event);
    }
}

static void to_upper(char *str)
{
    if (!str) return;
    while (*str) {
        *str = std::toupper(static_cast<unsigned char>(*str));
        ++str;
    }
}

GarageDoor::GarageDoor(
    std::shared_ptr<ProgramState> state,
    std::shared_ptr<Mqtt> mqtt
) :
    program_state(state),
    mqtt_client(mqtt),

    in1(MOTOR_IN_1, -1, false, false),
    in2(MOTOR_IN_2, -1, false, false),
    in3(MOTOR_IN_3, -1, false, false),
    in4(MOTOR_IN_4, -1, false, false),

    sw0(SW_0, -1, true, true, true, true, GPIO_IRQ_EDGE_FALL),
    sw1(SW_1, -1, true, true, true, true, GPIO_IRQ_EDGE_FALL),
    sw2(SW_2, -1, true, true, true, true, GPIO_IRQ_EDGE_FALL),

    lim_sw1(LIMIT_SW_1, -1, true, true, true),
    lim_sw2(LIMIT_SW_2, -1, true, true, true),

    rot_a(ROT_SIG_A, -1, true, false, false, true, GPIO_IRQ_EDGE_RISE),
    rot_b(ROT_SIG_B, -1, true, false, false),
    rot_sw(ROT_SW, -1, true, true, true)
{
    queue_init(&sw0_queue, sizeof(uint8_t), 10);
    queue_init(&sw1_queue, sizeof(uint8_t), 10);
    queue_init(&sw2_queue, sizeof(uint8_t), 10);
    queue_init(&rot_encoder_queue, sizeof(uint8_t), 10);
}

void GarageDoor::half_step_motor(bool reverse)
{
    std::array<decltype(in1)*, 4> pins = { &in1, &in2, &in3, &in4 };
    size_t pin_count = pins.size();

    for (size_t i = 0; i < pin_count; ++i) {
        int idx = reverse ? (pin_count - i) : i;
        pins[i]->write(STEP_SEQUENCE[curr_step][idx]);
    }

    curr_step = (curr_step + 1) % STEP_SEQ_COUNT;
    sleep_ms(1);
}

void GarageDoor::calibrate_motor()
{
    auto ps = program_state->read();

    uint8_t event_sw0;
    uint8_t event_sw2;
    bool sw0_pressed = false;
    bool sw2_pressed = false;

    if (queue_try_remove(&sw0_queue, &event_sw0)) sw0_pressed = true;
    if (queue_try_remove(&sw2_queue, &event_sw2)) sw2_pressed = true;

    if (sw0_pressed && sw2_pressed) {
        ps.is_running = 1;
        program_state->write(ps);

        uint8_t rot = 0;
        while(queue_try_remove(&rot_encoder_queue, &rot));

        while(!lim_sw1())  half_step_motor();
        while(!lim_sw2()) {
            if (queue_try_remove(&rot_encoder_queue, &rot)) {
                ps.is_open = 1;
            }
            half_step_motor(true);
            ps.steps_down++;
        }
        while(!lim_sw1()) {
            if (queue_try_remove(&rot_encoder_queue, &rot)) {
                ps.is_open = 0;
            }
            half_step_motor(false);
            ps.steps_up++;
        }

        ps.door_position = 0;
        ps.is_running = 0;
        ps.calibrated = 1;

        program_state->write(ps);
    }
}

void GarageDoor::connect_mqtt_client()
{
    if (!(*mqtt_client)()) {
        std::cout << "connect" << std::endl;
        mqtt_client->connect();
    }
}

void GarageDoor::handle_door_stop(T_ProgramState& ps, bool remote_cmd, int door_position)
{
    ps.is_running = 0;
    ps.door_position = door_position;
    program_state->write(ps);

    if (remote_cmd) {
        mqtt_client->send_message(RESPONSE_TOPIC, "Door stopped.");
    }
    return;
}

uint64_t millis_now() {
    return time_us_64() / 1000; // convert to milliseconds
}

void GarageDoor::control_motor()
{
    auto ps = program_state->read();
    int door_position = ps.door_position;
    bool run_cmd = false, stop_cmd = false;
    uint8_t rot;
    uint64_t lastEncoderTime = millis_now();

    if (ps.is_running && (sw1() || (run_cmd = get_remote_command("RUN"))))
    {
        if (ps.is_open) {
            ps.is_open = 0;
            program_state->write(ps);

            while(!lim_sw1()) {
                half_step_motor(false);
                if (queue_try_remove(&rot_encoder_queue, &rot)) {
                    lastEncoderTime = millis_now();
                }
                if (millis_now() - lastEncoderTime > 1000) {
                    program_state->reset_eeprom();
                    return;
                }
            }
            ps.door_position = 0;
        } else {
            ps.is_open = 1;
            program_state->write(ps);

            while(!lim_sw2()) half_step_motor(true);
            ps.door_position = ps.steps_down;
        }

        ps.is_running = 0;
        program_state->write(ps);
    }
    else if (ps.is_running == 0 && (sw1() || (run_cmd = get_remote_command("RUN"))))
    {
        ps.is_running = 1;
        program_state->write(ps);

        if (ps.is_open == 0) {
            ps.is_open = 1;
            program_state->write(ps);

            for (int i = 0; i < ((ps.steps_down + ps.steps_up) / 2) - ps.door_position; ++i) {
                mqtt_client->yield(1);
                half_step_motor(true);

                if (sw1() || (stop_cmd = get_remote_command("STOP"))) {
                    handle_door_stop(ps, stop_cmd, door_position);
                    return;
                }

                if (queue_try_remove(&rot_encoder_queue, &rot)) {
                    lastEncoderTime = millis_now();
                }
                if (millis_now() - lastEncoderTime > 1000) {
                    program_state->reset_eeprom();
                    return;
                }

                door_position++;
            }

            ps.is_running = 0;
            ps.door_position = door_position;
            program_state->write(ps);

            if (run_cmd) {
                mqtt_client->send_message(RESPONSE_TOPIC, "Door opened.");
            }
        } else {
            ps.is_open = 0;
            program_state->write(ps);

            for (int i = 0; i < ps.door_position; ++i) {
                mqtt_client->yield(1);
                half_step_motor(false);

                if (sw1() || (stop_cmd = get_remote_command("STOP"))) {
                    handle_door_stop(ps, stop_cmd, door_position);
                    return;
                }

                if (queue_try_remove(&rot_encoder_queue, &rot)) {
                    lastEncoderTime = millis_now();
                }
                if (millis_now() - lastEncoderTime > 1000) {
                    program_state->reset_eeprom();
                    return;
                }

                door_position--;
            }

            ps.is_running = 0;
            ps.door_position = door_position;
            program_state->write(ps);

            if (run_cmd) {
                mqtt_client->send_message(RESPONSE_TOPIC, "Door closed...");
            }
        }
    }
    mqtt_client->yield(50);
}

bool GarageDoor::get_remote_command(char *cmd)
{
    if (!(*mqtt_client)()) {
        std::cout << "Client not connected :(" << std::endl;
        return false;
    }

    T_MQTT_payload payload{};
    if (mqtt_client->try_get_mqtt_msg(&payload)) {
        to_upper(payload.message);
        if (strcmp(payload.message, cmd) == 0) {
            return true;
        }
    }
    return false;
}

// void GarageDoor::remote_control()
// {
//     if (!(*mqtt_client)()) {
//         std::cout << "Client not connected :(" << std::endl;
//         return;
//     }

//     T_MQTT_payload payload_buff{};
//     if (mqtt_client->try_get_mqtt_msg(&payload_buff) &&
//         strcmp(payload_buff.topic, COMMAND_TOPIC) == 0)
//     {
//         to_upper(payload_buff.message);

//         int command = (
//             strcmp(payload_buff.message, "OPEN")    == 0    ? 1 :
//             strcmp(payload_buff.message, "CLOSE")   == 0    ? 2 :
//             strcmp(payload_buff.message, "STOP")    == 0    ? 3 : 0
//         );

//         int rc = -1;
//         switch (command) {
//             case 1:
//                 rc = mqtt_client->send_message(RESPONSE_TOPIC, "Opening door...");
//                 if (rc == 0) {

//                 }
//                 break;
//             case 2:
//                 rc = mqtt_client->send_message(RESPONSE_TOPIC, "Closing door...");
//                 break;
//             case 3:
//                 rc = mqtt_client->send_message(RESPONSE_TOPIC, "Stopping door...");
//                 break;
//             default:
//                 rc = mqtt_client->send_message(RESPONSE_TOPIC, "invalid command");
//                 break;
//         }

//         if (rc != 0)
//             std::cout << "failed " << rc << std::endl;
//         else
//             std::cout << "success" << std::endl;
//     }
// }

/*
    FOR TESTING
*/
void GarageDoor::reset()
{
    if (rot_sw())
        program_state->reset_eeprom();
}

// void GarageDoor::test_mqtt()
// {
//     if ((*mqtt_client)()) {
//         T_MQTT_payload payload_buff;
//         if (mqtt_client->try_get_mqtt_msg(&payload_buff)) {
//             std::cout << "MQTT message recieved: \n"
//                 << "Topic: " << payload_buff.topic << "\n"
//                 << "Message: " << payload_buff.message << "\n"
//                 << "Command topic cmp: " << strcmp(payload_buff.topic, COMMAND_TOPIC) << "\n"
//                 << "Response topic cmp: " << strcmp(payload_buff.topic, RESPONSE_TOPIC) << "\n"
//                 << "Status topic cmp: " << strcmp(payload_buff.topic, STATUS_TOPIC) << std::endl;
//         }
//     }
// }
