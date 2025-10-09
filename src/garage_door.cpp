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
    {1, 0, 0, 0},  // Step 1
    {1, 1, 0, 0},  // Step 2
    {0, 1, 0, 0},  // Step 3
    {0, 1, 1, 0},  // Step 4
    {0, 0, 1, 0},  // Step 5
    {0, 0, 1, 1},  // Step 6
    {0, 0, 0, 1},  // Step 7
    {1, 0, 0, 1}   // Step 8
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

/*
    GarageDoor methods
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

    led1(LED_1, -1, false, false),
    led2(LED_2, -1, false, false),
    led3(LED_3, -1, false, false),

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

    led1.write(true);
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

        while(!lim_sw1()) half_step_motor();
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
            half_step_motor();
            ps.steps_up++;
        }

        ps.door_position = 0;
        ps.is_running = 0;
        ps.calibrated = 1;

        program_state->write(ps);

        led1.write(false);
        led2.write(true);
    }
}

void GarageDoor::connect_mqtt_client()
{
    if (!(*mqtt_client)()) {
        std::cout << "connect" << std::endl;
        mqtt_client->connect();
    }
}

void GarageDoor::local_control()
{
    auto ps = program_state->read();
    int door_position = ps.door_position;

    // offset is power off during running
    if (ps.is_running && sw1())
    {
        if (ps.is_open) {
            ps.is_open = 0;
            program_state->write(ps);

            while(!lim_sw1()) half_step_motor(false);
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
    else if (ps.is_running == 0 && sw1())
    {
        ps.is_running = 1;
        program_state->write(ps);

        if (ps.is_open == 0) {
            ps.is_open = 1;
            program_state->write(ps);

            for (int i = 0; i < ((ps.steps_down + ps.steps_up) / 2) - ps.door_position; ++i) {
                if (sw1()) {
                    ps.is_running = 0;
                    ps.door_position = door_position;
                    program_state->write(ps);
                    return;
                }
                half_step_motor(true);
                door_position++;
            }
            ps.is_running = 0;
            ps.door_position = door_position;
            program_state->write(ps);
        } else {
            ps.is_open = 0;
            program_state->write(ps);
            
            for (int i = 0; i < ps.door_position; ++i) {
                 if (sw1()) {
                    ps.is_running = 0;
                    ps.door_position = door_position;
                    program_state->write(ps);
                    return;
                }
                half_step_motor(false);
                door_position--;
            }
            ps.is_running = 0;
            ps.door_position = door_position;
            program_state->write(ps);
        }
    }
}

void GarageDoor::remote_control()
{
    if (!(*mqtt_client)()) {
        std::cout << "Client not connected :(" << std::endl;
        return;
    }

    T_MQTT_payload payload_buff{};
    if (mqtt_client->try_get_mqtt_msg(&payload_buff) &&
        strcmp(payload_buff.topic, COMMAND_TOPIC) == 0)
    {
        to_upper(payload_buff.message);

        int command = (
            strcmp(payload_buff.message, "OPEN")    == 0    ? 1 :
            strcmp(payload_buff.message, "CLOSE")   == 0    ? 2 :
            strcmp(payload_buff.message, "STOP")    == 0    ? 3 : 0
        );

        int rc = -1;
        switch (command) {
            case 1:
                std::cout << "door opening..." << std::endl;
                rc = mqtt_client->send_message(RESPONSE_TOPIC, "door opened");
                break;
            case 2:
                std::cout << "door closing..." << std::endl;
                rc = mqtt_client->send_message(RESPONSE_TOPIC, "door closed");
                break;
            case 3:
                std::cout << "door stopping..." << std::endl;
                rc = mqtt_client->send_message(RESPONSE_TOPIC, "door stopped");
                break;
            default:
                std::cout << "invalid command" << std::endl;
                rc = mqtt_client->send_message(RESPONSE_TOPIC, "invalid command");
                break;
        }

        if (rc != 0)
            std::cout << "failed " << rc << std::endl;
        else
            std::cout << "success" << std::endl;
    }
}

/*
    FOR TESTING
*/
void GarageDoor::reset()
{
    if (rot_sw())
        program_state->reset_eeprom();
}

void GarageDoor::test_mqtt()
{
    if ((*mqtt_client)()) {
        T_MQTT_payload payload_buff;
        if (mqtt_client->try_get_mqtt_msg(&payload_buff)) {
            std::cout << "MQTT message recieved: \n"
                << "Topic: " << payload_buff.topic << "\n"
                << "Message: " << payload_buff.message << "\n"
                << "Command topic cmp: " << strcmp(payload_buff.topic, COMMAND_TOPIC) << "\n"
                << "Response topic cmp: " << strcmp(payload_buff.topic, RESPONSE_TOPIC) << "\n"
                << "Status topic cmp: " << strcmp(payload_buff.topic, STATUS_TOPIC) << std::endl;
        }
    }
}
