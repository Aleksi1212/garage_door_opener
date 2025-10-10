#include "pico/stdlib.h"
#include <pico/util/queue.h>
#include <cstdint>
#include <garage_door.hpp>
#include <program_state.hpp>
#include <iostream>
#include <hardware.hpp>
#include <array>
#include <cstdarg>
#include <cstdio>

#define STEP_SEQ_COUNT 8

/*
    Stepper motor steps
*/
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
/* END */

/*
    Callback queues and methods for gpio pins
*/
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
/* END */

/*
    Util functions
*/
static void to_upper(char *str)
{
    if (!str) return;
    while (*str) {
        *str = std::toupper(static_cast<unsigned char>(*str));
        ++str;
    }
}

static uint64_t millis_now() {
    return time_us_64() / 1000; // convert to milliseconds
}
/* END */


/*
    Garage door methods
*/
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

    uint64_t lastEncoderTime = millis_now();

    if (sw0_pressed && sw2_pressed) {
        ps.is_running = 1;
        program_state->write(ps);

        uint8_t rot = 0;
        while(queue_try_remove(&rot_encoder_queue, &rot)); // clear queue

        /*
            If motor is in the middle offset it until it hits the limit switch 1
        */
        while(!lim_sw1()) {
            half_step_motor();
            
            if (queue_try_remove(&rot_encoder_queue, &rot)) {
                lastEncoderTime = millis_now();
            }
            if (millis_now() - lastEncoderTime > 1000) {
                handle_door_stuck(ps, false);
                return;
            }
        }

        /*
            Run motor until hits limit switch 2, and count how many steps it takes
        */
        while(!lim_sw2()) {
            half_step_motor(true);
            ps.steps_down++;

            if (queue_try_remove(&rot_encoder_queue, &rot)) {
                ps.is_open = 1;
                lastEncoderTime = millis_now();
            }
            if (millis_now() - lastEncoderTime > 1000) {
                handle_door_stuck(ps, false);
                return;
            }
        }

        /*
            Run motor until hits limit switch 1, and count how many steps it takes
        */
        while(!lim_sw1()) {
            half_step_motor();
            ps.steps_up++;

            if (queue_try_remove(&rot_encoder_queue, &rot)) {
                ps.is_open = 0;
                lastEncoderTime = millis_now();
            }
            if (millis_now() - lastEncoderTime > 1000) {
                handle_door_stuck(ps, false);
                return;
            }
        }

        ps.door_position = 0;
        ps.is_running = 0;
        ps.calibrated = 1;
        ps.is_door_stuck = 0;

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

void GarageDoor::handle_door_stop(T_ProgramState& ps, bool remote_cmd, int door_position, int is_running)
{
    ps.is_running = is_running;
    ps.door_position = door_position;
    program_state->write(ps);

    if (remote_cmd) {
        mqtt_client->send_message(RESPONSE_TOPIC, "Door stopped.");
    }
    return;
}

void GarageDoor::handle_door_stuck(T_ProgramState& ps, bool remote_cmd)
{
    ps.calibrated = 0;
    ps.steps_down = 0;
    ps.steps_up = 0;
    ps.is_door_stuck = 1;
    program_state->write(ps);

    if (remote_cmd) {
        mqtt_client->send_message(RESPONSE_TOPIC,
            "Error while %s door.", ps.is_open ? "opening" : "closing");
    }
    return;
}

void GarageDoor::control_motor()
{
    auto ps = program_state->read();
    int door_position = ps.door_position;
    bool run_cmd = false, stop_cmd = false;
    uint8_t rot;
    uint64_t lastEncoderTime = millis_now();

    int count = 0;

    if ((ps.steps_down + ps.steps_up) / 2 > 15000) return;

    /*
        Door was running when starting to run
    */
    if (ps.is_running && (sw1() || (run_cmd = get_remote_command("RUN"))))
    {
        if (ps.is_open) {
            ps.is_open = 0;
            program_state->write(ps);

            // Run motor until hits limit switch 1
            while(!lim_sw1()) {
                if (count % 100 == 0) mqtt_client->yield(1); // yield mqtt client every 100th step
                half_step_motor(false);

                // Door stopped
                if (sw1() || (stop_cmd = get_remote_command("STOP"))) {
                    handle_door_stop(ps, stop_cmd, 0, 1);
                    return;
                }

                if (queue_try_remove(&rot_encoder_queue, &rot)) {
                    lastEncoderTime = millis_now();
                }
                // door gets stuck
                if (millis_now() - lastEncoderTime > 1000) {
                    handle_door_stuck(ps, run_cmd);
                    return;
                }
                count++;
            }
            ps.door_position = 0;
        } else {
            ps.is_open = 1;
            program_state->write(ps);

            // Run motor until hits limit switch 2
            while(!lim_sw2()) {
                if (count % 100 == 0) mqtt_client->yield(1); // yield mqtt client every 100th step
                half_step_motor(true);

                // door stopped
                if (sw1() || (stop_cmd = get_remote_command("STOP"))) {
                    handle_door_stop(ps, stop_cmd, 0, 1);
                    return;
                }

                if (queue_try_remove(&rot_encoder_queue, &rot)) {
                    lastEncoderTime = millis_now();
                }
                // door gets stuck
                if (millis_now() - lastEncoderTime > 1000) {
                    handle_door_stuck(ps, run_cmd);
                    return;
                }
                count++;
            }
            ps.door_position = ps.steps_down;
        }

        ps.is_running = 0;
        program_state->write(ps);

        // If remote control send a repsonse
        if (run_cmd) {
            mqtt_client->send_message(RESPONSE_TOPIC,
                "Door %s.", ps.is_open ? "opened" : "closed");
        }
    }

    /*
        Door was not running when starting to run
    */
    else if (ps.is_running == 0 && (sw1() || (run_cmd = get_remote_command("RUN"))))
    {
        ps.is_running = 1;
        program_state->write(ps);

        if (ps.is_open == 0) {
            ps.is_open = 1; // 
            program_state->write(ps);

            // run door for step average - door position
            for (int i = 0; i < ((ps.steps_down + ps.steps_up) / 2) - ps.door_position; ++i) {
                if (i % 100 == 0) mqtt_client->yield(1); // yield mqtt client every 100th step
                half_step_motor(true);

                // if stop
                if (sw1() || (stop_cmd = get_remote_command("STOP"))) {
                    handle_door_stop(ps, stop_cmd, door_position);
                    return;
                }

                if (queue_try_remove(&rot_encoder_queue, &rot)) {
                    lastEncoderTime = millis_now();
                }
                // if stuck
                if (millis_now() - lastEncoderTime > 1000) {
                    handle_door_stuck(ps, run_cmd);
                    return;
                }

                door_position++;
            }

            ps.is_running = 0;
            ps.door_position = door_position;
            program_state->write(ps);

            // if remote command send response
            if (run_cmd) {
                mqtt_client->send_message(RESPONSE_TOPIC, "Door opened.");
            }
        } else {
            ps.is_open = 0;
            program_state->write(ps);

            // run door for door position amount of steps
            for (int i = 0; i < ps.door_position; ++i) {
                if (i % 100 == 0) mqtt_client->yield(1); // yield mqtt client every 100th step

                half_step_motor(false);

                // if stop
                if (sw1() || (stop_cmd = get_remote_command("STOP"))) {
                    handle_door_stop(ps, stop_cmd, door_position);
                    return;
                }

                if (queue_try_remove(&rot_encoder_queue, &rot)) {
                    lastEncoderTime = millis_now();
                }

                // if stuck
                if (millis_now() - lastEncoderTime > 1000) {
                    handle_door_stuck(ps, run_cmd);
                    return;
                }

                door_position--;
            }

            ps.is_running = 0;
            ps.door_position = door_position;
            program_state->write(ps);

            // if remote command send response
            if (run_cmd) {
                mqtt_client->send_message(RESPONSE_TOPIC, "Door closed.");
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

/*
    FOR TESTING
*/
void GarageDoor::reset()
{
    if (rot_sw())
        program_state->reset_eeprom();
}
