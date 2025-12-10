# server.py
# Простой backend сервер для детекции чисел на кубиках

from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.responses import JSONResponse
from ultralytics import YOLO
import cv2
import numpy as np
import os
from typing import List, Dict
import io

app = FastAPI(title="Dice Detection API", description="API для детекции чисел на кубиках")

# Загружаем модель при старте сервера
model_path = 'runs/detect/dice_detection_run/weights/best.pt'

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
            return JSONResponse(content={
                "success": True,
                "detected": False,
                "message": "Числа не обнаружены"
            })
        
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
    print("\n" + "="*60)
    print("Запуск Dice Detection API сервера")
    print("="*60)
    print(f"Модель: {model_path}")
    print(f"Статус модели: {'Загружена' if model is not None else 'Не найдена'}")
    print("\nСервер будет доступен по адресу: http://localhost:8003")
    print("Документация API: http://localhost:8003/docs")
    print("="*60 + "\n")
    
    uvicorn.run(app, host="0.0.0.0", port=8003)

