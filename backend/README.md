# Backend IoT Safety Bus

Xem [hướng dẫn cấu hình hiện hành](../docs/SETUP-AND-TEST.md). Ngoài Atlas phải đặt DEVICE_API_KEY, JWT_SECRET và ADMIN_PASSWORD_HASH; MQTT/TLS cần thông tin broker.

Tạo file `.env` trong thư mục `backend`:

```env
MONGODB_URI=mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority
MONGODB_DATABASE=iot_safety_bus
```

Sau đó chạy:

```powershell
cd backend
py -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
uvicorn app.main:app --reload --port 8000
```

Swagger: http://localhost:8000/docs. Dữ liệu được lưu trên MongoDB Atlas, không dùng SQLite.

Để bật YOLOv8: cài `requirements-ai.txt`; chạy `camera_worker.py --source 0 --preview`. Worker gửi kết quả đếm người vào `POST /api/detections` và khung hình có bounding box vào `POST /api/camera/frame`; bàn trực xem qua `GET /api/camera/frame`. Không có database giả ở runtime. Bộ test dùng mongomock riêng.
