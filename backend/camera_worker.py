"""USB/RTSP YOLOv8 worker: python camera_worker.py --source 0.

Posts real detections, not fabricated demo values, and streams the annotated
frame to the web dashboard. Press q in preview to stop.
"""
import argparse
import os
import time
from collections import deque
from statistics import median
from pathlib import Path
import cv2
import httpx
from dotenv import load_dotenv
from ultralytics import YOLO


def encode_frame(frame, max_width: int) -> bytes | None:
    height, width = frame.shape[:2]
    if width > max_width:
        frame = cv2.resize(frame, (max_width, int(height * max_width / width)), interpolation=cv2.INTER_AREA)
    ok, buffer = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 70])
    return buffer.tobytes() if ok else None


def main():
    load_dotenv(Path(__file__).parent / ".env")
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default="0")
    parser.add_argument("--api", default="http://127.0.0.1:8000")
    parser.add_argument("--device", default="bus-01")
    parser.add_argument("--preview", action="store_true")
    parser.add_argument("--frame-interval", type=float, default=1.0, help="Seconds between frames sent to the dashboard")
    parser.add_argument("--frame-width", type=int, default=960)
    args = parser.parse_args()
    key = os.environ["DEVICE_API_KEY"]
    model = YOLO("yolov8n.pt")
    capture = cv2.VideoCapture(int(args.source) if args.source.isdigit() else args.source)
    if not capture.isOpened():
        raise RuntimeError("Cannot open camera/video source")
    last_sent = 0
    last_frame = 0
    recent_counts = deque(maxlen=5)
    try:
        with httpx.Client(timeout=5, headers={"X-Device-Key": key}) as client:
            while True:
                ok, frame = capture.read()
                if not ok:
                    break
                result = model.predict(frame, classes=[0], conf=.35, verbose=False)[0]
                recent_counts.append(len(result.boxes))
                count = int(median(recent_counts))
                annotated = result.plot()
                if time.monotonic() - last_sent >= 2:
                    try:
                        response = client.post(args.api + "/api/detections", json={"device_id": args.device, "person_count": count, "confidence": float(result.boxes.conf.max()) if len(result.boxes) else 0, "source": "camera"})
                        response.raise_for_status()
                    except httpx.HTTPError as exc:
                        print("Detection delivery failed:", type(exc).__name__)
                    last_sent = time.monotonic()
                if time.monotonic() - last_frame >= args.frame_interval:
                    jpeg = encode_frame(annotated, args.frame_width)
                    if jpeg:
                        try:
                            response = client.post(args.api + "/api/camera/frame", params={"device_id": args.device, "person_count": count}, content=jpeg, headers={"Content-Type": "image/jpeg"})
                            response.raise_for_status()
                        except httpx.HTTPError as exc:
                            print("Frame delivery failed:", type(exc).__name__)
                    last_frame = time.monotonic()
                if args.preview:
                    cv2.imshow("Smart Bus YOLOv8", annotated)
                    if cv2.waitKey(1) & 0xFF == ord("q"):
                        break
    finally:
        capture.release()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
