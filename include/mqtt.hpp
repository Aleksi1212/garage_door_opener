#ifndef MQTT_HPP
#define MQTT_HPP

#include <memory>
#include <IPStack.hpp>
#include <Countdown.hpp>
#include "MQTTClient/src/MQTTClient.h"
#include <pico/util/queue.h>

#define MQTT_VER 3
#define CLIENT_ID "PicoW"

#define COMMAND_TOPIC "garage/door/command"
#define RESPONSE_TOPIC "garage/door/response"
#define STATUS_TOPIC "garage/door/status"

#define MQTT_TOPIC_SIZE 64
#define MQTT_MSG_SIZE (MQTT_TOPIC_SIZE) * 2

struct T_MQTT_payload
{
    char topic[MQTT_TOPIC_SIZE];
    char message[MQTT_MSG_SIZE];
};

class Mqtt : public std::enable_shared_from_this<Mqtt>
{
    protected:
        Mqtt();
        void init_instance();
        static std::weak_ptr<Mqtt> instance_ptr;

    private:
        IPStack ipstack;

        MQTT::Client<IPStack, Countdown> client;
        MQTTPacket_connectData data;

        int tcp_rc;
        int mqtt_rc;

        absolute_time_t mqtt_send;
        // int mqtt_qos;
        int msg_count;
        char publish_buf[MQTT_MSG_SIZE];


        queue_t mqtt_msg_queue;

        void tcp_client_connect();
        void subscirbe(const char* topic);

        void msg_handler_instance(MQTT::MessageData &md);
        static void msg_handler_static(MQTT::MessageData &md);

    public:
        static std::shared_ptr<Mqtt> create();

        bool operator() ();
        void connect();
        bool try_get_mqtt_msg(T_MQTT_payload *payload_buff);
        int send_message(const char *topic, char *message);
        void yield(unsigned long timeout_ms);
};

#endif