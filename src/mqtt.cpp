#include <iostream>
#include <mqtt.hpp>
#include <network_info.h>
#include <hardware.hpp>


using namespace std;

/*
    Init mqtt
*/
Mqtt::Mqtt() : ipstack(SSID, PW), client(ipstack)
{
    data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VER;
    data.clientID.cstring = (char *)CLIENT_ID;

    mqtt_send = make_timeout_time_ms(2000);
    msg_count = 0;

    tcp_rc = 0;
    mqtt_rc = -1;

    queue_init(&mqtt_msg_queue, sizeof(T_MQTT_payload), 10);
}
/* END */

/*
    Creator
*/
weak_ptr<Mqtt> Mqtt::instance_ptr;
void Mqtt::init_instance() { instance_ptr = shared_from_this(); }
shared_ptr<Mqtt> Mqtt::create()
{
    auto ptr = shared_ptr<Mqtt>(new Mqtt());
    ptr->init_instance();
    return ptr;
}
/* END */

bool Mqtt::operator()() { return client.isConnected(); } // Is client connected?

/*
    Try to connect to internet
*/
void Mqtt::tcp_client_connect()
{
    cout << "TCP connecting..." << endl;

    tcp_rc = ipstack.connect(HOSTNAME, PORT);
    if (tcp_rc != 1) {
        cout << "rc from TCP connect is " << tcp_rc << endl;
        return;
    }
    cout << "Connected to " << HOSTNAME << ":" << PORT << endl;
}
/* END */

/*
    Try to subscribe to a mqtt topic
*/
void Mqtt::subscirbe(const char *topic)
{
    mqtt_rc = client.subscribe(topic, MQTT::QOS0, Mqtt::msg_handler_static);
    if (mqtt_rc != 0) {
        cout << "rc from MQTT subsribe to " << topic << " is: " << mqtt_rc << endl;
        return;
    }
    cout << "MQTT subscirbed to " << topic << endl;
}
/* END */

/*
    Try to connect to mqtt server
*/
void Mqtt::connect()
{
    if (tcp_rc != 1) tcp_client_connect();

    cout << "MQTT connecting..." << endl;

    mqtt_rc = client.connect(data);
    if (mqtt_rc != 0) {
        cout << "rc from MQTT connect is: " << mqtt_rc << endl;
        tight_loop_contents();
        return;
    }
    cout << "MQTT connected" << endl;

    subscirbe((char *)COMMAND_TOPIC);
    subscirbe((char *)RESPONSE_TOPIC);
    subscirbe((char *)STATUS_TOPIC);
}
/* END */

/*
    Mqtt message handlers
*/
void Mqtt::msg_handler_instance(MQTT::MessageData &md)
{
    auto &message = md.message;
    auto &topic = md.topicName;

    T_MQTT_payload payload{};

    if (topic.cstring) {
        strncpy(payload.topic, topic.cstring, sizeof(payload.topic));
    } else {
        size_t len = (topic.lenstring.len < sizeof(payload.topic) - 1)
                        ? topic.lenstring.len
                        : sizeof(payload.topic) - 1;
        memcpy(payload.topic, topic.lenstring.data, len);
        payload.topic[len] = '\0';
    }
    snprintf(payload.message, sizeof(payload.message), "%.*s", (int)message.payloadlen, (char *)message.payload);

    // Add message to queue
    queue_try_add(&mqtt_msg_queue, &payload);
}

// static message handler for "client.subscibe" method
void Mqtt::msg_handler_static(MQTT::MessageData &md)
{
    if (auto inst = instance_ptr.lock()) {
        inst->msg_handler_instance(md);
    }
}
/* END */


/*
    Gets mqtt message from queue
*/
bool Mqtt::try_get_mqtt_msg(T_MQTT_payload *payload_buff) 
{
    return queue_try_remove(&mqtt_msg_queue, payload_buff);
}
/* END */

/*
    Sends a message to mqtt server
*/
int Mqtt::send_message(const char *topic, char *format, ...)
{
    MQTT::Message msg{};
    msg.retained = false;
    msg.dup = false;
    msg.qos = MQTT::QOS0;

    va_list args;
    va_start(args, format);
    vsnprintf(publish_buf, MQTT_MSG_SIZE, format, args);
    va_end(args);

    msg.payload = publish_buf;
    msg.payloadlen = strlen(publish_buf);

    return client.publish(topic, msg);
}
/* END */

// Yields client for timeout_ms milliseconds
void Mqtt::yield(unsigned long timeout_ms) { client.yield(timeout_ms); }
