# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Users

Nhân viên điều hành tại trung tâm/depot, theo dõi xe BUS-01 trên laptop hoặc màn hình lớn trong ca trực. Việc chính: biết ngay xe có còn người sau khi tắt máy hay không, xác nhận cảnh báo, và điều khiển quạt/còi từ xa trong lúc chờ người kiểm tra thực tế cuối xe.

## Product Purpose

Smart Bus cảnh báo nguy cơ bỏ quên hành khách (đặc biệt trẻ em) trên xe sau khi tắt khóa điện. Thành công là: khi còn người trên xe, điều hành viên thấy cảnh báo, nhìn được khoang xe qua camera, và có người kiểm tra thực tế trước khi sự cố xảy ra.

## Positioning

Ba nguồn quan sát độc lập trên cùng một xe: cảm biến IR đếm lên/xuống, PIR phát hiện chuyển động, và camera chạy YOLOv8 đếm người trong khoang. Hệ thống đối chiếu IR với camera và báo khi chúng lệch nhau.

## Operating Context

- ESP32-S3 trên xe gửi telemetry qua MQTT/TLS (hoặc HTTP LAN) tới FastAPI; backend lưu MongoDB Atlas.
- Camera worker chạy trên laptop, đọc webcam/RTSP, chạy YOLOv8 và gửi số người lên backend mỗi 2 giây; kết quả AI hết hạn sau 15 giây.
- Trạng thái mạch: NORMAL, CHECK (đếm ngược 15 giây kiểm tra), WARNING (còi, đèn đỏ, quạt bật), OFFLINE sau 30 giây không có dữ liệu.
- Nút vật lý cuối xe mới xóa được cảnh báo; xác nhận trên web chỉ ghi nhận đã nhận thông báo.

## Capabilities and Constraints

- Lệnh từ xa: FAN_ON (5 phút), FAN_AUTO, BUZZER_SILENCE (30 giây), BUZZER_AUTO; chỉ gửi được khi mạch trực tuyến và MQTT kết nối.
- Hiện có một xe (bus-01); một tài khoản quản trị.
- Không dùng dữ liệu giả hoặc ảnh mẫu tải lên; mọi số liệu trên màn hình phải đến từ mạch hoặc camera thật.

## Evidence on Hand

Chưa có ảnh chụp thật của xe, khoang xe hay dữ liệu vận hành. Không được bịa số liệu, khách hàng hay chứng nhận.

## Product Principles

- Trạng thái an toàn phải đọc được trong một cái liếc.
- Nói rõ nguồn và độ tươi của mỗi số liệu; dữ liệu cũ không được trông như dữ liệu hiện tại.
- Web hỗ trợ, không thay thế việc kiểm tra thực tế cuối xe.
- Cảnh báo phải nổi bật hơn mọi thứ khác khi xảy ra, và im lặng khi bình thường.
