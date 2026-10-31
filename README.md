# IoT Safety Bus

Hệ thống giám sát hành khách và cảnh báo người còn trên xe: ESP32-S3, FastAPI, MongoDB Atlas, MQTT/TLS, React và YOLOv8.

## Tệp chính

- [Firmware Arduino](esp32s3_bus_safety/esp32s3_bus_safety.ino)
- [Hướng dẫn nạp và sơ đồ GPIO](esp32s3_bus_safety/README.md)
- [Backend và cấu hình](backend/README.md)
- [Camera worker YOLOv8](backend/camera_worker.py)
- [Frontend React](frontend/src/main.jsx)

Xem [hướng dẫn cấu hình, nạp và nghiệm thu](docs/SETUP-AND-TEST.md) trước khi chạy. Cấu hình firmware từ `config.example.h`; không đưa bí mật lên Git.

[Kết quả kiểm tra và giới hạn nghiệm thu](docs/VALIDATION.md).
