import json
import logging

import paho.mqtt.client as mqtt

log = logging.getLogger(__name__)


def start_mqtt(settings, ingest, schema, commands):
    if not settings.mqtt_host:
        return None
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="safety-bus-backend")
    client.username_pw_set(settings.mqtt_username, settings.mqtt_password)
    client.tls_set(ca_certs=settings.mqtt_ca_file)
    client.reconnect_delay_set(1, 30)

    def connected(client, userdata, flags, reason, properties):
        if reason == 0:
            client.subscribe("bus/+/telemetry", qos=1)
            client.subscribe("bus/+/acks", qos=1)

    def received(client, userdata, message):
        try:
            parts = message.topic.split("/")
            data = json.loads(message.payload)
            if data.get("device_id") != parts[1]:
                raise ValueError("device/topic mismatch")
            if parts[2] == "telemetry":
                ingest(schema.model_validate(data))
            else:
                commands.update_one({"command_id": data["command_id"], "device_id": parts[1]}, {"$set": {"status": "applied"}})
        except Exception:
            log.exception("Rejected MQTT message on %s", message.topic)

    client.on_connect = connected
    client.on_message = received
    client.connect_async(settings.mqtt_host, settings.mqtt_port, 30)
    client.loop_start()
    return client
