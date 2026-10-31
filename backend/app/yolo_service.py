from __future__ import annotations

import asyncio
import io
from functools import lru_cache
from threading import Lock
from PIL import Image, UnidentifiedImageError
from fastapi import HTTPException
import tempfile
from pathlib import Path

model_lock = Lock()


@lru_cache(maxsize=1)
def get_model():
    from ultralytics import YOLO
    return YOLO("yolov8n.pt")


async def detect_people(content: bytes) -> dict[str, float | int | str]:
    """Run YOLO class 0 (person) detection on one uploaded image.

    Imports stay inside the function so the API still starts in demo mode without
    the large AI packages installed.
    """
    try:
        from ultralytics import YOLO
    except ImportError as exc:
        raise ImportError("Install ultralytics and opencv-python to enable YOLO") from exc

    def run() -> dict[str, float | int | str]:
        try:
            with Image.open(io.BytesIO(content)) as image:
                if image.format not in ("JPEG", "PNG") or image.width * image.height > 16000000:
                    raise ValueError("Unsupported image")
                image.verify()
        except (UnidentifiedImageError, OSError, ValueError, Image.DecompressionBombError) as exc:
            raise HTTPException(400, "Expected a valid JPEG/PNG up to 16 megapixels") from exc
        with tempfile.NamedTemporaryFile(suffix=".jpg", delete=False) as temp:
            temp.write(content)
            image_path = Path(temp.name)
        try:
            with model_lock:
                model = get_model()
                result = model.predict(source=str(image_path), classes=[0], conf=0.35, verbose=False)[0]
            boxes = result.boxes
            count = len(boxes) if boxes is not None else 0
            confidence = float(boxes.conf.max().item()) if count else 0.0
            return {"person_count": count, "confidence": round(confidence, 4), "model": "yolov8n"}
        finally:
            image_path.unlink(missing_ok=True)

    return await asyncio.to_thread(run)
