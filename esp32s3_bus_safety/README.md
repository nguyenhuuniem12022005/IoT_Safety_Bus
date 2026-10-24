# Firmware ESP32-S3 cho mạch xe buýt

Firmware này dùng cặp cảm biến IR trong ảnh để đếm người qua cửa, PIR HC-SR501 để phát hiện chuyển động khi xe tắt máy, và điều khiển còi, relay/quạt, LED RGB cùng OLED.

## Sơ đồ GPIO theo tài liệu nối mạch

| GPIO | Thiết bị |
| --- | --- |
| 4 | IR 1 OUT |
| 5 | PIR HC-SR501 OUT |
| 6 | IR 2 OUT |
| 8 | OLED SDA |
| 9 | OLED SCL |
| 10 | Công tắc khóa điện, HIGH = ON |
| 11 | Nút KY-004 xác nhận kiểm tra cuối xe |
| 7 | Nút KY-004 thứ hai (nếu đã nối theo sơ đồ cập nhật); cả hai nút đều xác nhận kiểm tra |
| 12 | KY-012 buzzer |
| 13 | Relay IN |
| 14, 15, 16 | KY-016 R, G, B |

Nguồn theo mô-đun: OLED và cảm biến IR cấp 3.3V; PIR cấp 5V; relay cấp 5V; quạt trong ảnh là quạt DC 5V, 0.20A. Nối GND chung theo sơ đồ của mạch. Quạt lấy nguồn qua tiếp điểm COM/NO của relay, không cấp từ chân GPIO.

## Luồng hoạt động

- IR 1 rồi IR 2 trong vòng 1.5 giây: cộng 1 người.
- IR 2 rồi IR 1 trong vòng 1.5 giây: trừ 1 người; sĩ số không xuống dưới 0.
- Khi khóa điện OFF mà sĩ số còn người hoặc PIR phát hiện chuyển động, OLED hiển thị đếm ngược 15 giây để tài xế xuống kiểm tra.
- Bấm nút GPIO11 hoặc GPIO7 khi PIR đã hết phát hiện: xác nhận đã kiểm tra, xóa báo động và đưa sĩ số về 0.
- Hết 15 giây không xác nhận: còi kêu, đèn đỏ bật, relay cấp điện cho quạt; báo động được giữ đến khi bấm nút sau khi PIR hết phát hiện.
- HC-SR501 được chờ 30 giây sau khi bật nguồn để ổn định.

## Nạp chương trình

1. Mở `esp32s3_bus_safety.ino` bằng Arduino IDE.
2. Cài **Adafruit SSD1306** và **Adafruit GFX Library** trong Library Manager.
3. Chọn board tương ứng với module, thường là **ESP32S3 Dev Module**, rồi chọn cổng COM của ESP32-S3.
4. Nạp code và mở Serial Monitor ở **115200 baud**.

OLED mặc định dùng địa chỉ `0x3C`. Nếu màn hình không lên, kiểm tra SDA/SCL và thử đổi `OLED_ADDRESS` thành `0x3D`.

## Nếu tín hiệu cảm biến bị đảo

Các module LM393 trong ảnh có biến trở chỉnh ngưỡng. Code mặc định coi OUT xuống LOW là bị che (`IR_BLOCKED_LEVEL = LOW`). Nếu Serial Monitor cho thấy cảm biến báo ngược, đổi thành `HIGH`. Chỉnh biến trở và kiểm tra từng cảm biến riêng trước khi thử đếm chiều.

Nếu relay bật khi hệ thống đang bình thường, đổi `RELAY_ACTIVE_LOW` ở đầu file thành `false`. Nếu nút KY-004 không nhận khi bấm, kiểm tra chân S/+/- và đổi `SAFETY_BUTTON_ACTIVE_LOW` nếu module xuất mức HIGH khi bấm. Nếu RGB hiển thị màu ngược, chỉnh `RGB_COMMON_ANODE`.

Chỉ đưa tín hiệu công tắc 3.3V vào GPIO10; không nối điện khóa xe 12V/24V trực tiếp vào ESP32. Firmware này điều khiển mạch mô hình tại chỗ, chưa kết nối Wi-Fi/MQTT, camera AI hay WebApp.
