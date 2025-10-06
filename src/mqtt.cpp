#include <iostream>
#include <mqtt.hpp>
#include <network_info.h>

using namespace std;

Mqtt::Mqtt() : ipstack(SSID, PW), client(ipstack)
{
    data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VER;
    data.clientID.cstring = (char *)CLIENT_ID;

    mqtt_send = make_timeout_time_ms(2000);
    mqtt_qos = 0;
    msg_count = 0;

    tcp_rc = 0;
    mqtt_rc = -1;

    queue_init(&mqtt_msg_queue, MQTT_MSG_SIZE, 10);
}

weak_ptr<Mqtt> Mqtt::instance_ptr;
void Mqtt::init_instance() { instance_ptr = shared_from_this(); }
shared_ptr<Mqtt> Mqtt::create()
{
    auto ptr = shared_ptr<Mqtt>(new Mqtt());
    ptr->init_instance();
    return ptr;
}

bool Mqtt::operator()() { return client.isConnected(); }
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
void Mqtt::subscirbe(const char *topic)
{
    mqtt_rc = client.subscribe(topic, MQTT::QOS2, Mqtt::msg_handler_static);
    if (mqtt_rc != 0) {
        cout << "rc from MQTT subsribe to " << topic << " is: " << mqtt_rc << endl;
        return;
    }
    cout << "MQTT subscirbed to " << topic << endl;
}
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

void Mqtt::msg_handler_instance(MQTT::MessageData &md)
{
    MQTT::Message &message = md.message;

    char buffer[MQTT_MSG_SIZE];

    snprintf(buffer, sizeof(buffer), "%.*s", (int)message.payloadlen, (char *)message.payload);

    queue_try_add(&mqtt_msg_queue, &buffer);
}

void Mqtt::msg_handler_static(MQTT::MessageData &md)
{
    if (auto inst = instance_ptr.lock()) {
        inst->msg_handler_instance(md);
    }
}

bool Mqtt::try_get_mqtt_msg(char *buffer, size_t buffer_size)
{
    client.yield(100);

    char local_buf[MQTT_MSG_SIZE];
    if (queue_try_remove(&mqtt_msg_queue, &local_buf)) {
        strncpy(buffer, local_buf, buffer_size);
        buffer[buffer_size - 1] = '\0';
        return true;
    }
    return false;
}
