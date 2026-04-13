########################################################################################################
#                                                                                                      #
#   MQTT Paho Documentation - https://eclipse.dev/paho/index.php?page=clients/python/docs/index.php    #
#                                                                                                      #
########################################################################################################
import paho.mqtt.client as mqtt
from random import randint
from json import dumps, loads
from time import sleep

class MQTT:

    ID = f"IOT_B_{randint(1,1000000)}"

    sub_topics = [("620172489_pub", 0), ("620172489", 0), ("620172489_sub", 0)]

    def __init__(self, mongo):
        self.randint  = randint
        self.loads    = loads
        self.dumps    = dumps
        self.sleep    = sleep
        self.mongo    = mongo

        self.client = mqtt.Client(
            client_id=self.ID,
            clean_session=True,
            reconnect_on_failure=True
        )
        self.client.on_connect    = self.on_connect
        self.client.on_message    = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.client.on_subscribe  = self.on_subscribe

        self.client.message_callback_add("620172489",     self.update)
        self.client.message_callback_add("620172489_pub", self.toggle)

        self.client.connect_async("www.yanacreations.com", 1883, 60)

        self.client.loop_start()

    def connack_string(self, rc):
        msgs = {
            0: "Connection successful",
            1: "Connection refused - incorrect protocol version",
            2: "Connection refused - invalid client identifier",
            3: "Connection refused - server unavailable",
            4: "Connection refused - bad username or password",
            5: "Connection refused - not authorised",
        }
        return msgs.get(rc, f"Unknown rc={rc}")

    def on_connect(self, client, userdata, flags, rc):
        print("\n\nMQTT: " + self.connack_string(rc),
              " ID: ", client._client_id.decode("utf-8"))
        client.subscribe(self.sub_topics)

    def on_subscribe(self, client, userdata, mid, granted_qos):
        print("MQTT: Subscribed to", [t[0] for t in self.sub_topics])

    def on_disconnect(self, client, userdata, rc):
        if rc != 0:
            print("MQTT: Unexpected Disconnection.")

    def on_message(self, client, userdata, msg):
        try:
            print(msg.topic + " " + msg.payload.decode("utf-8"))
        except Exception as e:
            print(f"MQTT: onMessage Error: {str(e)}")

    def publish(self, topic, payload):
        """Publish a string payload to a topic. Returns True on success."""
        try:
            if not isinstance(payload, str):
                payload = self.dumps(payload)
            info = self.client.publish(topic, payload, qos=1)
            info.wait_for_publish(timeout=5)
            success = info.rc == mqtt.MQTT_ERR_SUCCESS
            if success:
                print(f"MQTT: Published to {topic}: {payload}")
            else:
                print(f"MQTT: Publish returned rc={info.rc}")
            return success
        except Exception as e:
            print(f"MQTT: Publish failed - {str(e)}")
            return False

    def update(self, client, userdata, msg):
        try:
            payload = msg.payload.decode("utf-8")
            print(payload)
            update = self.loads(payload)
            print(update)
            result = self.mongo.update(update)
            if result:
                print("[DB] Saved to MongoDB successfully.")
            else:
                print("[DB] Failed to save to MongoDB.")
        except Exception as e:
            print(f"MQTT: update Error: {str(e)}")

    def toggle(self, client, userdata, msg):
        try:
            payload = msg.payload.decode("utf-8")
            update  = self.loads(payload)
            print(update)
        except Exception as e:
            print(f"MQTT: toggle Error - {str(e)}")