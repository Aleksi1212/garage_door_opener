import paho.mqtt.client as mqtt

# MQTT broker settings
BROKER = "192.168.0.10"  # Public broker for testing
PORT = 1884
TOPIC = "garage/door/response"

# Called when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully!")
        client.subscribe(TOPIC)
        print(f"Subscribed to topic: {TOPIC}")
    else:
        print(f"Connection failed with code {rc}")

# Called when a message is received from the broker
def on_message(client, userdata, msg):
    print(f"Message received on topic '{msg.topic}': {msg.payload.decode()}")

# Create MQTT client instance
client = mqtt.Client()

# Attach callback functions
client.on_connect = on_connect
client.on_message = on_message

# Connect to broker
client.connect(BROKER, PORT, 60)

# Start the network loop and listen for messages
client.loop_forever()
