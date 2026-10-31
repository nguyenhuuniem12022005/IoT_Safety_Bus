---
version: 1
slug: "frontend-src-main-jsx"
primary_target: "frontend/src/main.jsx"
related_targets: ["frontend/src/style.css"]
---

# Bàn trực an toàn (frontend/src/main.jsx)

Scope: đăng nhập + bàn trực một xe (BUS-01). Mode: Operate.

Audience/job: điều hành viên ở depot, laptop hoặc màn lớn, ca ngày và ca đêm dưới đèn huỳnh quang. Việc: biết xe còn người hay không, nhìn khoang xe qua camera YOLO, xác nhận cảnh báo, điều khiển quạt/còi.

Constraints: không có tải ảnh; mọi số liệu đến từ mạch hoặc camera thật; mỗi số liệu ghi rõ nguồn và độ tươi.

## Direction contract

THESIS: Màn hình sơn như thân chiếc xe đang được canh. Từ chối kiểu dashboard thẻ số liệu bằng nhau với thanh bên tối và accent xanh.

OWN-WORLD: Vỏ vàng thân xe (#F2B01E) chỉ ở thanh trên; nền thép sơn xám nhạt; camera trong "dải kính" đen bo góc như cửa sổ xe; trạng thái chạy trên dải phản quang đỏ trắng toàn chiều ngang. Một họ chữ Be Vietnam Pro, số tabular. Đường kẻ 1px, bo góc nhỏ.

STORY: Liếc là biết trạng thái qua dải phản quang; nhìn camera để thấy người; đối chiếu IR với camera; bấm đã nhận cảnh báo và điều khiển quạt/còi; xem lệnh nào mạch đã áp dụng.

FIRST VIEWPORT: Thanh vàng 56px (biển BUS-01, tên, kết nối API/ESP32/Camera, đăng xuất). Dải phản quang 52px. Dưới: trái là khung camera 16:9 trong dải kính và sơ đồ khoang xe; phải 380px là bảng đối chiếu ai đang trên xe, điều khiển, nhật ký lệnh. Hành động chính (Đã nhận cảnh báo) nằm ngay trên dải phản quang khi WARNING.

FORM: Thân xe đưa đón học sinh, ứng viên 7/7. seed:e63e79b4ac33421c. Signature move: sơ đồ khoang xe nhìn từ trên, chỉ đường 90/45 độ, đặt IR ở cửa, PIR và nút ở cuối xe, sáng theo trạng thái thật. Raises: một màu trạng thái sở hữu cả dải; lệnh không biến mất mà đổi trạng thái; một biểu đồ sĩ số chiếm trọn ô, đánh dấu lúc tắt khóa điện.

FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance
