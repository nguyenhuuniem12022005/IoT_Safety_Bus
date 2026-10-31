from __future__ import annotations

import time
import secrets
import uuid
import json
import asyncio
from pathlib import Path
from contextlib import asynccontextmanager
from typing import Literal
from typing import Any

from fastapi import FastAPI, HTTPException, Depends, Header, Request, Response, WebSocket, WebSocketDisconnect
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
import jwt
from pwdlib import PasswordHash
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
from pydantic_settings import BaseSettings, SettingsConfigDict
from pymongo import DESCENDING, MongoClient
from pymongo.errors import PyMongoError
from fastapi.responses import JSONResponse


class Settings(BaseSettings):
    mongodb_uri: str
    mongodb_database: str = "iot_safety_bus"
    device_api_key: str
    jwt_secret: str = Field(min_length=32)
    admin_username: str = "admin"
    admin_password_hash: str
    mqtt_host: str = ""
    mqtt_port: int = 8883
    mqtt_username: str = ""
    mqtt_password: str = ""
    mqtt_ca_file: str | None = None
    model_config = SettingsConfigDict(env_file=Path(__file__).resolve().parents[1] / ".env", extra="ignore")


settings = Settings()
mongo_client = MongoClient(settings.mongodb_uri, serverSelectionTimeoutMS=5000)
mongo = mongo_client[settings.mongodb_database]
status_collection = mongo["device_status"]
events_collection = mongo["events"]
detections_collection = mongo["detections"]
history_collection = mongo["telemetry"]
commands_collection = mongo["commands"]
bearer = HTTPBearer(auto_error=False)


def operator(credentials: HTTPAuthorizationCredentials | None = Depends(bearer)):
    try:
        payload = jwt.decode(credentials.credentials if credentials else "", settings.jwt_secret, algorithms=["HS256"], options={"require":["exp","sub","role"]})
        if payload.get("role") != "admin":
            raise ValueError("role")
        return payload
    except (jwt.InvalidTokenError, ValueError):
        raise HTTPException(401, "Authentication required")


def device(x_device_key: str = Header(default="")):
    if not secrets.compare_digest(x_device_key, settings.device_api_key):
        raise HTTPException(401, "Invalid device key")


class Telemetry(BaseModel):
    device_id: str = Field(default="bus-01", pattern=r"^[a-zA-Z0-9_-]{1,40}$")
    state: Literal["NORMAL", "CHECK", "WARNING"] = "NORMAL"
    ignition_on: bool = False
    passenger_count: int = Field(default=0, ge=0, le=999)
    total_boarded: int = Field(default=0, ge=0)
    total_alighted: int = Field(default=0, ge=0)
    check_remaining: int = Field(default=0, ge=0, le=15)
    pir_detected: bool = False
    fan_on: bool = False
    buzzer_on: bool = False
    pir_ready: bool = False
    inspection_id: str | None = None


class Detection(BaseModel):
    device_id: str = Field(default="bus-01", pattern=r"^[a-zA-Z0-9_-]{1,40}$")
    person_count: int = Field(default=0, ge=0)
    confidence: float = Field(default=0.0, ge=0, le=1)
    source: Literal["camera"] = "camera"


MAX_FRAME_BYTES = 2 * 1024 * 1024
latest_frames: dict[str, dict[str, Any]] = {}


@asynccontextmanager
async def lifespan(app):
    mongo_client.admin.command("ping")
    status_collection.create_index("device_id", unique=True)
    for collection in (events_collection, detections_collection, history_collection):
        collection.create_index([("device_id", 1), ("created_at", -1)])
    from .mqtt_bridge import start_mqtt
    bridge = start_mqtt(settings, ingest_telemetry, Telemetry, commands_collection)
    app.state.bridge = bridge
    try:
        yield
    finally:
        if bridge:
            bridge.disconnect()
            bridge.loop_stop()
        mongo_client.close()


app = FastAPI(title="IoT Safety Bus API", version="1.0.0", lifespan=lifespan)


@app.exception_handler(PyMongoError)
async def database_error(request, exc):
    return JSONResponse(status_code=503, content={"detail": "Database unavailable. Check backend logs and Atlas connection."})
app.add_middleware(CORSMiddleware, allow_origins=["http://localhost:5173", "http://127.0.0.1:5173"], allow_credentials=True, allow_methods=["*"], allow_headers=["*"], expose_headers=["X-Captured-At", "X-Person-Count"])


class Login(BaseModel):
    username: str
    password: str


@app.post("/api/login")
def login(body: Login):
    valid = False
    if body.username == settings.admin_username:
        try:
            valid = PasswordHash.recommended().verify(body.password, settings.admin_password_hash)
        except Exception:
            pass
    if not valid:
        raise HTTPException(401, "Invalid username or password")
    return {"access_token": jwt.encode({"sub": body.username, "role": "admin", "exp": int(time.time()) + 3600}, settings.jwt_secret, algorithm="HS256"), "token_type": "bearer"}


def record_event(device_id: str, event_type: str, message: str, severity: str, payload: dict[str, Any]) -> None:
    events_collection.insert_one({"device_id": device_id, "type": event_type, "message": message, "severity": severity, "payload": payload, "created_at": time.time()})


@app.get("/api/health")
def health() -> dict[str, str]:
    try:
        mongo_client.admin.command("ping")
        return {"status": "ok", "database": "mongodb-atlas", "service": "iot-safety-bus-api"}
    except Exception as exc:
        raise HTTPException(status_code=503, detail="MongoDB Atlas unavailable") from exc


@app.get("/api/status", dependencies=[Depends(operator)])
def status(device_id: str = "bus-01") -> dict[str, Any]:
    row = status_collection.find_one({"device_id": device_id}, {"_id": 0})
    if row is None:
        return {"device_id": device_id, "state": "OFFLINE", "online": False, "passenger_count": 0}
    result = dict(row)
    result["online"] = time.time() - result["updated_at"] < 30
    for key in ("ignition_on", "pir_detected", "fan_on"):
        result[key] = bool(result[key])
    return result


@app.get("/api/events", dependencies=[Depends(operator)])
def events(limit: int = 30, device_id: str = "bus-01") -> list[dict[str, Any]]:
    limit = max(1, min(limit, 100))
    rows = list(events_collection.find({"device_id": device_id}).sort("created_at", DESCENDING).limit(limit))
    for row in rows:
        row["id"] = str(row.pop("_id"))
    return rows


@app.get("/api/detections", dependencies=[Depends(operator)])
def detections(limit: int = 30, device_id: str = "bus-01") -> list[dict[str, Any]]:
    limit = max(1, min(limit, 100))
    return list(detections_collection.find({"device_id": device_id}, {"_id": 0}).sort("created_at", DESCENDING).limit(limit))


@app.post("/api/telemetry", dependencies=[Depends(device)])
def ingest_telemetry(telemetry: Telemetry) -> dict[str, Any]:
    now = time.time()
    previous = status_collection.find_one({"device_id": telemetry.device_id})
    status_collection.replace_one({"device_id": telemetry.device_id}, {**telemetry.model_dump(), "updated_at": now}, upsert=True)
    history_collection.insert_one({**telemetry.model_dump(), "created_at": now})
    if previous and (previous.get("total_boarded", 0) != telemetry.total_boarded or previous.get("total_alighted", 0) != telemetry.total_alighted):
        record_event(telemetry.device_id, "PASSENGER_COUNT", f"Sĩ số: {telemetry.passenger_count}; tổng lên: {telemetry.total_boarded}; tổng xuống: {telemetry.total_alighted}", "info", telemetry.model_dump())
    if telemetry.state == "CHECK" and (not previous or previous["state"] != "CHECK"):
        record_event(telemetry.device_id, "SAFETY_CHECK", "Tắt khóa điện khi còn người: bắt đầu đếm ngược kiểm tra khoang xe", "warning", telemetry.model_dump())
    if telemetry.state == "WARNING" and (not previous or previous["state"] != "WARNING"):
        record_event(telemetry.device_id, "ALARM", "Phát hiện nguy cơ còn người trên xe", "critical", telemetry.model_dump())
    if telemetry.inspection_id and (not previous or previous.get("inspection_id") != telemetry.inspection_id):
        record_event(telemetry.device_id, "INSPECTION", "Đã bấm nút kiểm tra cuối xe", "info", telemetry.model_dump())
    return {"accepted": True, "updated_at": now}


@app.post("/api/detections", dependencies=[Depends(device)])
def ingest_detection(detection: Detection) -> dict[str, Any]:
    previous = detections_collection.find_one({"device_id": detection.device_id}, sort=[("created_at", DESCENDING)])
    result = detections_collection.insert_one({**detection.model_dump(), "created_at": time.time()})
    if detection.person_count > 0 and (not previous or previous["person_count"] == 0):
        record_event(detection.device_id, "YOLO_DETECTION", f"YOLO phát hiện {detection.person_count} người", "warning", detection.model_dump())
    bridge = getattr(app.state, "bridge", None)
    if bridge and bridge.is_connected():
        bridge.publish(f"bus/{detection.device_id}/commands", json.dumps({"device_id": detection.device_id, "action": "AI_PRESENCE", "person_count": detection.person_count, "command_id": uuid.uuid4().hex, "expires_at": int(time.time()) + 15}), qos=1, retain=False)
    return {"accepted": True, "id": str(result.inserted_id)}


@app.post("/api/alarm/ack", dependencies=[Depends(operator)])
def acknowledge_alarm(device_id: str = "bus-01") -> dict[str, bool]:
    record_event(device_id, "ACKNOWLEDGED", "Điều hành đã nhận cảnh báo; vẫn cần kiểm tra thực tế cuối xe", "info", {})
    return {"accepted": True}


@app.post("/api/camera/frame", dependencies=[Depends(device)])
async def ingest_frame(request: Request, device_id: str = "bus-01", person_count: int = 0) -> dict[str, bool]:
    if not request.headers.get("content-type", "").startswith("image/jpeg"):
        raise HTTPException(415, "Expected image/jpeg")
    content = await request.body()
    if not content or len(content) > MAX_FRAME_BYTES:
        raise HTTPException(413, "Frame must be a JPEG up to 2 MB")
    latest_frames[device_id] = {"jpeg": content, "captured_at": time.time(), "person_count": max(0, person_count)}
    return {"accepted": True}


@app.get("/api/camera/frame", dependencies=[Depends(operator)])
def camera_frame(device_id: str = "bus-01") -> Response:
    frame = latest_frames.get(device_id)
    if frame is None:
        raise HTTPException(404, "Camera has not sent a frame yet")
    return Response(frame["jpeg"], media_type="image/jpeg", headers={
        "Cache-Control": "no-store",
        "X-Captured-At": f"{frame['captured_at']:.3f}",
        "X-Person-Count": str(frame["person_count"]),
    })


class Command(BaseModel):
    device_id: str = Field(default="bus-01", pattern=r"^[a-zA-Z0-9_-]{1,40}$")
    action: Literal["FAN_ON", "FAN_AUTO", "BUZZER_SILENCE", "BUZZER_AUTO"]


@app.post("/api/commands", dependencies=[Depends(operator)])
def command(body: Command):
    bridge = getattr(app.state, "bridge", None)
    if not bridge or not bridge.is_connected():
        raise HTTPException(503, "MQTT broker not connected")
    message = {**body.model_dump(), "command_id": uuid.uuid4().hex, "expires_at": int(time.time()) + 30}
    commands_collection.insert_one({**message, "status": "pending", "created_at": time.time()})
    result = bridge.publish(f"bus/{body.device_id}/commands", json.dumps(message), qos=1, retain=False)
    if result.rc != 0:
        commands_collection.update_one({"command_id":message["command_id"]},{"$set":{"status":"failed"}})
        raise HTTPException(503, "MQTT publish failed")
    commands_collection.update_one({"command_id":message["command_id"],"status":"pending"},{"$set":{"status":"sent"}})
    record_event(body.device_id, "COMMAND", body.action, "info", message)
    return {"accepted": True, "command_id": message["command_id"], "status": "sent"}


@app.get("/api/history", dependencies=[Depends(operator)])
def history(device_id: str = "bus-01", limit: int = 100):
    return list(history_collection.find({"device_id": device_id}, {"_id": 0}).sort("created_at", DESCENDING).limit(max(1,min(limit,500))))


@app.get("/api/commands", dependencies=[Depends(operator)])
def command_history(device_id: str = "bus-01"):
    return list(commands_collection.find({"device_id":device_id},{"_id":0}).sort("created_at",DESCENDING).limit(30))


@app.websocket("/api/ws")
async def live(websocket: WebSocket):
    await websocket.accept()
    try:
        data = await asyncio.wait_for(websocket.receive_json(), timeout=10)
        claims = jwt.decode(data.get("token", ""), settings.jwt_secret, algorithms=["HS256"], options={"require":["exp","sub","role"]})
        if claims.get("role") != "admin":
            await websocket.close(code=1008)
            return
        while time.time() < claims["exp"]:
            snapshot = await asyncio.to_thread(status)
            await websocket.send_json(snapshot)
            await asyncio.sleep(1)
        await websocket.close(code=1008)
    except (jwt.InvalidTokenError, asyncio.TimeoutError):
        await websocket.close(code=1008)
    except WebSocketDisconnect:
        pass
