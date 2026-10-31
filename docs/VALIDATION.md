# Kết quả kiểm tra - 2026-10-31

## Đã thực hiện

| Hạng mục | Kết quả | Giới hạn của bằng chứng |
|---|---|---|
| PlatformIO ESP32-S3 DevKitC-1 N8 | Build SUCCESS; RAM 46,372/327,680 B, flash 930,113/3,342,336 B | Build với cấu hình mẫu, chưa nạp lên thiết bị |
| FastAPI | 8/8 test PASS | Dùng mongomock, chưa phải Atlas thực |
| Frontend React/Vite | Build production PASS | Đã mở trang đăng nhập; chưa đăng nhập với cloud thật |
| YOLOv8n | Chạy suy luận thật: ảnh bus mẫu phát hiện 3 người, confidence cao nhất 0.8657 | Không đại diện độ chính xác camera trong khoang xe |
| Editor diagnostics | Không báo lỗi ở main.py, mqtt_bridge.py và main.jsx | Không thay thế kiểm thử chạy thật |
| Serial device list | Không tìm thấy thiết bị serial | Chưa có COM để upload/monitor |

Ảnh thử YOLO lấy từ nguồn chính thức: https://raw.githubusercontent.com/ultralytics/assets/main/im/bus.jpg. Model tải từ Ultralytics assets. Không đưa ảnh mẫu hay model vào Git.

Các test API bao gồm: JWT/device key, login sai; lưu-đọc trạng thái; lịch sử telemetry; chuyển alarm không lặp; dữ liệu âm/state không hợp lệ; web ACK không xóa alarm vật lý; sự kiện kiểm tra không lặp; offline; detection; MQTT không kết nối; WebSocket yêu cầu JWT; publish lệnh và AI camera/ảnh upload được xử lý khác nhau. MQTT publish dùng test double, chưa kết nối broker TLS thật.

## Cần cấu hình và nghiệm thu trên hệ thống thật

1. Điền `.env` với Atlas URI, device key, JWT secret và hash mật khẩu. Cấu hình database user và IP allowlist.
2. Điền broker TLS/ACL và CA trong `bus_config.h`; kiểm tra ESP32 và backend cùng broker.
3. Xác nhận model board, GPIO/cực tính thực và cổng COM. Nạp đúng firmware sau khi cấu hình Wi-Fi/broker, không dùng credential mẫu.
4. Thử cảm biến, relay/còi/quạt, mất mạng, khởi động lại, nhận lệnh và ACK theo [hướng dẫn nghiệm thu](SETUP-AND-TEST.md).
5. Chạy camera thật, ghi nhận độ chính xác thực tế và xác nhận dữ liệu được lưu/đọc lại từ Atlas.

## Chưa triển khai

Không có cloud deploy tự động, Zalo/Telegram, stream video lên web, quản trị nhiều tài khoản/RBAC nhiều vai trò, audit chống sửa bằng mật mã, hàng đợi telemetry bền vững hoặc MQTT QoS1 end-to-end. Các phần này không được coi là đã hoàn tất trong báo cáo.
