# server.py
# Простой backend сервер для детекции чисел на кубиках

from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from ultralytics import YOLO
import cv2
import numpy as np
import os
from typing import List, Dict
import io

app = FastAPI(title="Dice Detection API", description="API для детекции чисел на кубиках")

# Enable CORS for local frontend/dev usage
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Загружаем модель при старте сервера
# Используем raw string, чтобы путь на Windows не "съедал" backslash.
model_path = r"C:\Users\gev\Desktop\Dungeon_Master\dicetower\src\esp32cam\cv\best_2.pt"

if not os.path.exists(model_path):
    print(f"Предупреждение: Модель '{model_path}' не найдена.")
    print("Сервер запустится, но детекция не будет работать до обучения модели.")
    model = None
else:
    print(f"Загрузка модели из {model_path}...")
    model = YOLO(model_path)
    print("Модель загружена успешно!")

@app.get("/")
async def root():
    """Информация о сервере"""
    return {
        "message": "Dice Detection API",
        "status": "running",
        "model_loaded": model is not None
    }

@app.get("/health")
async def health():
    """Проверка здоровья сервера"""
    return {
        "status": "healthy",
        "model_loaded": model is not None
    }

def _log_summary(tag: str, img, results, detections):
    """Print sizes and best detection (class/conf/box)."""
    # incoming size (decoded image)
    try:
        incoming_w, incoming_h = img.shape[1], img.shape[0]
        incoming = f"{incoming_w}x{incoming_h}"
    except Exception:
        incoming = "unknown"

    # inference size (model input)
    inference = "unknown"
    try:
        if results and len(results) > 0:
            r0 = results[0]
            if hasattr(r0, "imgsz") and r0.imgsz is not None:
                inference = f"{r0.imgsz[1]}x{r0.imgsz[0]}"
            elif hasattr(r0, "orig_shape") and r0.orig_shape is not None:
                ih, iw = r0.orig_shape[:2]
                inference = f"{iw}x{ih}"
    except Exception:
        pass

    # Best detection
    best = None
    if detections:
        best = max(detections, key=lambda d: d.get("confidence", 0.0))

    if best:
        bbox = best.get("bbox", {})
        box_str = f"x1={bbox.get('x1')},y1={bbox.get('y1')},w={bbox.get('width')},h={bbox.get('height')}"
        top = f"{best.get('class','?')}@{best.get('confidence',0)} [{box_str}]"
    else:
        top = "none"

    print(f"{tag}: incoming={incoming}, inference={inference}, count={len(detections)}, top={top}")


@app.post("/detect")
async def detect_dice(file: UploadFile = File(...)):
    """
    Детекция чисел на кубике
    
    Принимает изображение и возвращает:
    - detected_numbers: список обнаруженных чисел с координатами и уверенностью
    - count: количество обнаруженных чисел
    """
    if model is None:
        raise HTTPException(
            status_code=503,
            detail="Модель не загружена. Пожалуйста, сначала обучите модель."
        )
    
    # Проверяем тип файла
    if not file.content_type.startswith('image/'):
        raise HTTPException(
            status_code=400,
            detail="Файл должен быть изображением"
        )
    
    try:
        # Читаем изображение
        contents = await file.read()
        nparr = np.frombuffer(contents, np.uint8)
        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if img is None:
            raise HTTPException(
                status_code=400,
                detail="Не удалось декодировать изображение"
            )
        
        # Запускаем детекцию
        results = model(img)
        
        # Обрабатываем результаты
        detected_numbers = []
        
        for result in results:
            boxes = result.boxes
            for box in boxes:
                # Получаем координаты
                x1, y1, x2, y2 = map(float, box.xyxy[0].cpu().numpy())
                
                # Получаем уверенность и класс
                conf = float(box.conf[0].cpu().numpy())
                cls = int(box.cls[0].cpu().numpy())
                label = model.names[cls]
                
                # Формируем результат
                detection = {
                    "class": label,
                    "class_id": cls,
                    "confidence": round(conf, 4),
                    "bbox": {
                        "x1": round(x1, 2),
                        "y1": round(y1, 2),
                        "x2": round(x2, 2),
                        "y2": round(y2, 2),
                        "width": round(x2 - x1, 2),
                        "height": round(y2 - y1, 2)
                    }
                }
                detected_numbers.append(detection)
        
        # Сортируем по уверенности (от большей к меньшей)
        detected_numbers.sort(key=lambda x: x["confidence"], reverse=True)
        
        _log_summary("YOLO detect", img, results, detected_numbers)

        return JSONResponse(content={
            "success": True,
            "detected_numbers": detected_numbers,
            "count": len(detected_numbers),
            "image_size": {
                "width": int(img.shape[1]),
                "height": int(img.shape[0])
            }
        })
    
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=f"Ошибка при обработке изображения: {str(e)}"
        )

@app.post("/detect/best")
async def detect_best_dice(file: UploadFile = File(...)):
    """
    Детекция с возвратом только самого уверенного результата
    
    Возвращает только число с наибольшей уверенностью
    """
    if model is None:
        raise HTTPException(
            status_code=503,
            detail="Модель не загружена. Пожалуйста, сначала обучите модель."
        )
    
    if not file.content_type.startswith('image/'):
        raise HTTPException(
            status_code=400,
            detail="Файл должен быть изображением"
        )
    
    try:
        # Читаем изображение
        contents = await file.read()
        nparr = np.frombuffer(contents, np.uint8)
        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if img is None:
            raise HTTPException(
                status_code=400,
                detail="Не удалось декодировать изображение"
            )
        
        # Запускаем детекцию
        results = model(img)
        
        # Находим самый уверенный результат
        best_detection = None
        best_confidence = 0.0
        
        for result in results:
            boxes = result.boxes
            for box in boxes:
                conf = float(box.conf[0].cpu().numpy())
                if conf > best_confidence:
                    best_confidence = conf
                    cls = int(box.cls[0].cpu().numpy())
                    label = model.names[cls]
                    x1, y1, x2, y2 = map(float, box.xyxy[0].cpu().numpy())
                    
                    best_detection = {
                        "class": label,
                        "class_id": cls,
                        "confidence": round(conf, 4),
                        "bbox": {
                            "x1": round(x1, 2),
                            "y1": round(y1, 2),
                            "x2": round(x2, 2),
                            "y2": round(y2, 2),
                            "width": round(x2 - x1, 2),
                            "height": round(y2 - y1, 2)
                        }
                    }
        
        if best_detection is None:
            _log_summary("YOLO detect_best", img, results, [])
            return JSONResponse(content={
                "success": True,
                "detected": False,
                "message": "Числа не обнаружены"
            })

        _log_summary("YOLO detect_best", img, results, [best_detection])

        return JSONResponse(content={
            "success": True,
            "detected": True,
            "result": best_detection
        })
    
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=f"Ошибка при обработке изображения: {str(e)}"
        )

if __name__ == "__main__":
    import uvicorn
    import socket
    host = os.environ.get("SERVER_HOST", "0.0.0.0")
    port = int(os.environ.get("SERVER_PORT", "8003"))

    # Попробуем угадать локальный IP, чтобы удобнее было подключаться с ESP
    def guess_local_ip():
        ip = "localhost"
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            s.close()
        except Exception:
            pass
        return ip

    local_ip = guess_local_ip()

    print("\n" + "="*60)
    print("Запуск Dice Detection API сервера")
    print("="*60)
    print(f"Модель: {model_path}")
    print(f"Статус модели: {'Загружена' if model is not None else 'Не найдена'}")
    print("\nСервер слушает на:")
    print(f"  http://{host}:{port}")
    if host == "0.0.0.0":
        print(f"  http://{local_ip}:{port}  (LAN)")
    print("Документация API:")
    print(f"  http://{host}:{port}/docs")
    if host == "0.0.0.0":
        print(f"  http://{local_ip}:{port}/docs")
    print("="*60 + "\n")
    
    uvicorn.run(app, host=host, port=port)

