# Cấu hình và nghiệm thu Smart Bus

## Phạm vi

Firmware đang nhắm ESP32-S3 DevKitC-1; GPIO giữ theo firmware gốc. PDF phân công không chứa sơ đồ đấu chân, vì vậy không thể suy ra đấu dây thực tế từ PDF. Không khẳng định đã nạp/chạy mạch nếu chưa có kiểm tra phần cứng.

## 1. Backend và Atlas

Mở PowerShell ở thư mục gốc repo:

```powershell
py -3.13 -m venv backend/.venv
backend/.venv/Scripts/python.exe -m pip install -r backend/requirements.txt
Copy-Item backend/.env.example backend/.env
backend/.venv/Scripts/python.exe backend/hash_password.py
```

Điền `MONGODB_URI`, `DEVICE_API_KEY`, `JWT_SECRET`, `ADMIN_PASSWORD_HASH` trong file `.env`; hash mật khẩu là kết quả lệnh trên. Không đưa các bí mật này lên Git. URI Atlas phải dùng database user có quyền readWrite với database `iot_safety_bus`. Trong Atlas Network Access, thêm địa chỉ IP máy chạy backend. Password trong URI cần URL-encode nếu chứa ký tự đặc biệt.

Điền broker TLS: `MQTT_HOST`, `MQTT_PORT=8883`, `MQTT_USERNAME`, `MQTT_PASSWORD`. Cấu hình broker ACL để ESP32 chỉ publish telemetry/acks và subscribe commands của đúng `bus-01`; backend có quyền tương ứng. Backend dùng CA hệ điều hành; có thể đặt `MQTT_CA_FILE` khi dùng CA riêng.

```powershell
backend/.venv/Scripts/python.exe -m uvicorn app.main:app --app-dir backend --host 0.0.0.0 --port 8000
```

Backend dừng khi thiếu cấu hình hoặc không kết nối được Atlas, không tự dùng database giả. API: `/docs`; health: `/api/health`. Collection: `device_status`, `telemetry`, `events`, `detections`, `commands`. Snapshot giữ trạng thái cuối, telemetry giữ lịch sử. Sự kiện cảnh báo được ghi khi chuyển trạng thái, không ghi lại mỗi heartbeat.

## 2. Frontend

```powershell
npm.cmd --prefix frontend ci
npm.cmd --prefix frontend run dev
```

Truy cập http://localhost:5173; đăng nhập bằng `ADMIN_USERNAME` và mật khẩu đã hash. Vite proxy chuyển REST/WebSocket tới backend. Frontend sử dụng trạng thái thật, không có nút ghi dữ liệu demo vào xe. Khi deploy, phục vụ frontend và proxy `/api` (bao gồm WebSocket) tới backend, hoặc cấu hình `VITE_API_URL` và CORS tương ứng.

## 3. Firmware

```powershell
Copy-Item esp32s3_bus_safety/config.example.h esp32s3_bus_safety/bus_config.h
backend/.venv/Scripts/python.exe -m pip install platformio
backend/.venv/Scripts/python.exe -m platformio run
backend/.venv/Scripts/python.exe -m platformio device list
# Sau khi xác định đúng COM:
# backend/.venv/Scripts/python.exe -m platformio run -t upload --upload-port COM5
# backend/.venv/Scripts/python.exe -m platformio device monitor --port COM5 --baud 115200
```

Sửa `bus_config.h`: Wi-Fi, `DEVICE_ID=bus-01`, broker credentials và PEM root CA hợp lệ. `MQTT_ROOT_CA` có thể dùng raw C++ string nhiều dòng. Chờ NTP đồng bộ trước khi TLS kết nối; không tắt kiểm tra certificate. Nếu chưa có broker, để `MQTT_HOST` trống để dùng HTTP LAN và đặt `API_BASE_URL` thành IP máy backend, `DEVICE_API_KEY` trùng backend. Chế độ HTTP không có lệnh điều khiển ngược/AI qua MQTT.

MQTT topics:

| Topic | Chiều | Nội dung |
|---|---|---|
| `bus/bus-01/telemetry` | ESP32 → backend | Sĩ số, PIR, khóa điện, state, quạt, đếm ngược, tổng lên/xuống |
| `bus/bus-01/commands` | Backend → ESP32 | FAN_ON, FAN_AUTO, AI_PRESENCE; command_id và hạn dùng |
| `bus/bus-01/acks` | ESP32 → backend | Xác nhận đã áp dụng lệnh |

PubSubClient publish telemetry/ack QoS 0; backend publish lệnh QoS 1. Không tuyên bố QoS 1 end-to-end. Chưa có hàng đợi bền vững khi mất mạng; vòng cảm biến/cảnh báo tại chỗ vẫn chạy độc lập. Backend đánh dấu offline sau 30 giây không nhận dữ liệu.

GPIO: IR1=4, IR2=6, PIR=5, SDA=8, SCL=9, ignition=10, nút=11/7, buzzer=12, relay=13, RGB=14/15/16. Ignition nối 3V3 khi ON, pulldown khi OFF. IR mặc định active LOW; nút active LOW; relay active LOW. Kiểm tra đúng cực tính module. GPIO chỉ nhận logic 3.3V, không nối điện xe vào GPIO. Quạt cấp qua relay bằng nguồn phù hợp và nối GND chung theo mạch gốc.

## 4. YOLOv8

```powershell
backend/.venv/Scripts/python.exe -m pip install -r backend/requirements-ai.txt
backend/.venv/Scripts/python.exe backend/camera_worker.py --source 0 --preview
```

Camera worker chạy trên laptop, không chạy YOLO trên ESP32. Webcam là UVC; có thể dùng đường dẫn video hoặc RTSP ở `--source`. Model `yolov8n.pt` được tải ở lần đầu. Kết quả gửi mỗi 2 giây kèm device key, lưu Atlas và chuyển AI_PRESENCE xuống mạch qua MQTT; dữ liệu AI hết hạn sau 15 giây, không dùng kết quả cũ như quan sát hiện tại. Trang web cũng hỗ trợ upload JPEG/PNG để kiểm thử model; ảnh upload không điều khiển mạch. Worker có bounding box ở cửa sổ preview, chưa stream video lên web.

## 5. Nghiệm thu trên mạch

1. Khởi động: tất cả output an toàn; OLED hoạt động; PIR warmup 30 giây.
2. Ignition ON; đi hết IR1 → IR2 rồi bỏ che: tăng 1. Chiều ngược lại: giảm 1, không âm. Kiểm tra quay đầu, che cả hai đồng thời và timeout 1.5 giây; không đếm trùng khi giữ beam.
3. Có người, ignition OFF: CHECK đếm 15 giây; hết hạn WARNING, còi + đỏ + quạt ON.
4. Nút cuối xe không xóa khi PIR hoặc AI còn người; khi kiểm tra và đã hết phát hiện, nút xóa alarm/count và tạo inspection event. Xác nhận trên web chỉ ghi nhận thông báo.
5. Tắt Wi-Fi: cảm biến/còi/quạt vẫn hoạt động. Bật lại: dữ liệu trạng thái tiếp tục về backend. Không kỳ vọng khôi phục toàn bộ lịch sử trong thời gian offline.
6. Web bật quạt: kiểm tra `commands.status=applied` và `fan_on` từ ESP32. FAN_AUTO không tắt quạt của alarm; bật từ xa tự hết sau 5 phút. BUZZER_SILENCE tạm tắt còi 30 giây, sau đó còi tự trở lại nếu vẫn alarm; không xóa sự cố.
7. IR đếm 0, camera thấy người, ignition OFF: AI qua MQTT phải khởi động CHECK, rồi WARNING nếu không kiểm tra. Thử người ngồi/nằm và camera bị che để đo giới hạn thực tế, không coi YOLO/PIR chính xác tuyệt đối.
8. Atlas: kiểm tra snapshot, lịch sử telemetry, sự kiện, detections và commands. Chụp kết quả thật để đưa báo cáo.
9. Tắt/bật nguồn ESP32: sĩ số, tổng lên/xuống và alarm được khôi phục từ NVS. Trạng thái CHECK không lưu qua mất nguồn, nhưng nếu sĩ số còn người khi OFF, kiểm tra sẽ khởi động lại.

## 6. Kiểm thử phần mềm

```powershell
backend/.venv/Scripts/python.exe -m pip install pytest mongomock
cd backend
.venv/Scripts/python.exe -m pytest tests -q
cd ..
npm.cmd --prefix frontend run build
```

Bộ test dùng MongoDB in-memory (`mongomock`) để kiểm API độc lập. Đây không phải bằng chứng đã kết nối Atlas/broker thật. Không triển khai cloud hoặc nạp thiết bị tự động trong bước kiểm thử.
