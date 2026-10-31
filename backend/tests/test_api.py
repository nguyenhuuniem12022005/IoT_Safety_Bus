import os
import time

import mongomock
import pytest
from fastapi.testclient import TestClient
from pwdlib import PasswordHash

os.environ.update(MONGODB_URI="mongodb://localhost:27017", DEVICE_API_KEY="test-device-key", JWT_SECRET="test-secret-only-" * 4,
                  ADMIN_PASSWORD_HASH=PasswordHash.recommended().hash("test-password"), MQTT_HOST="")
from app import main


@pytest.fixture
def client(monkeypatch):
    mongo = mongomock.MongoClient()
    monkeypatch.setattr(main, "mongo_client", mongo)
    database = mongo.test
    for variable, collection in [("status_collection", "device_status"), ("events_collection", "events"),
                                  ("detections_collection", "detections"), ("history_collection", "telemetry"), ("commands_collection", "commands")]:
        monkeypatch.setattr(main, variable, database[collection])
    with TestClient(main.app) as client:
        yield client


def auth(client):
    response = client.post("/api/login", json={"username": "admin", "password": "test-password"})
    assert response.status_code == 200
    return {"Authorization": "Bearer " + response.json()["access_token"]}


def test_auth_and_device_credentials(client):
    assert client.get("/api/status").status_code == 401
    assert client.post("/api/telemetry", json={}).status_code == 401
    assert client.post("/api/login", json={"username": "admin", "password": "wrong"}).status_code == 401


def test_telemetry_roundtrip_and_alarm_transition(client):
    headers = {"X-Device-Key": "test-device-key"}
    body = {"state": "WARNING", "passenger_count": 1, "fan_on": True}
    for _ in range(3):
        assert client.post("/api/telemetry", json=body, headers=headers).status_code == 200
    status = client.get("/api/status", headers=auth(client)).json()
    assert status["online"] and status["fan_on"] and status["passenger_count"] == 1
    events = client.get("/api/events", headers=auth(client)).json()
    assert len(events) == 1 and events[0]["id"]
    assert main.history_collection.count_documents({}) == 3


def test_invalid_state_and_negative_count(client):
    headers = {"X-Device-Key": "test-device-key"}
    for body in ({"state": "FAKE"}, {"passenger_count": -1}):
        assert client.post("/api/telemetry", json=body, headers=headers).status_code == 422


def test_web_ack_does_not_clear_physical_alarm(client):
    client.post("/api/telemetry", json={"state":"WARNING"}, headers={"X-Device-Key":"test-device-key"})
    assert client.post("/api/alarm/ack", headers=auth(client)).status_code == 200
    assert client.get("/api/status", headers=auth(client)).json()["state"] == "WARNING"


def test_offline_and_inspection_event(client):
    headers = {"X-Device-Key":"test-device-key"}
    for _ in range(2):
        client.post("/api/telemetry", json={"inspection_id":"boot-1"}, headers=headers)
    assert main.events_collection.count_documents({"type":"INSPECTION"}) == 1
    main.status_collection.update_one({"device_id":"bus-01"}, {"$set":{"updated_at":time.time()-31}})
    assert client.get("/api/status", headers=auth(client)).json()["online"] is False


def test_camera_detection_and_unavailable_broker(client):
    response = client.post("/api/detections", json={"person_count":2,"confidence":.9}, headers={"X-Device-Key":"test-device-key"})
    assert response.status_code == 200 and response.json()["id"]
    assert client.get("/api/detections",headers=auth(client)).json()[0]["person_count"] == 2
    assert client.post("/api/commands",json={"action":"FAN_ON"},headers=auth(client)).status_code == 503


def test_websocket_requires_jwt_and_streams_status(client):
    from starlette.websockets import WebSocketDisconnect
    with client.websocket_connect("/api/ws") as socket:
        socket.send_json({"token":"invalid"})
        with pytest.raises(WebSocketDisconnect):
            socket.receive_json()
    token = auth(client)["Authorization"].split()[1]
    with client.websocket_connect("/api/ws") as socket:
        socket.send_json({"token":token})
        assert socket.receive_json()["online"] is False


def test_command_publish_and_camera_fusion(client, monkeypatch):
    class Bridge:
        def __init__(self): self.messages=[]
        def is_connected(self): return True
        def publish(self, topic, payload, **kwargs):
            self.messages.append((topic,payload,kwargs))
            return type("Result",(),{"rc":0})()
    bridge=Bridge()
    monkeypatch.setattr(main.app.state,"bridge",bridge)
    response=client.post("/api/commands",json={"action":"FAN_ON"},headers=auth(client))
    assert response.status_code==200
    assert main.commands_collection.find_one({"command_id":response.json()["command_id"]})["status"]=="sent"
    client.post("/api/detections",json={"person_count":1,"source":"camera"},headers={"X-Device-Key":"test-device-key"})
    import json
    assert json.loads(bridge.messages[-1][1])["action"]=="AI_PRESENCE"
    before=len(bridge.messages)
    client.post("/api/detections",json={"person_count":1,"source":"upload"},headers={"X-Device-Key":"test-device-key"})
    assert len(bridge.messages)==before
