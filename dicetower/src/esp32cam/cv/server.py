# server.py
# Простой backend сервер для детекции чисел на кубиках

from fastapi import FastAPI, File, UploadFile, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from ultralytics import YOLO
import cv2
import numpy as np
import os
from typing import List, Dict, Set
import io
import asyncio
import json

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

# ============================================
# Roll Broadcast API (for Player Screen)
# ============================================
from pydantic import BaseModel
from typing import Optional
import time

class RollData(BaseModel):
    id: Optional[str] = None
    status: str  # 'rolling' or 'result'
    mode: str = 'normal'  # 'normal', 'advantage', 'disadvantage'
    playerName: Optional[str] = None
    label: Optional[str] = None
    dice: Optional[list] = None  # [low, high] for adv/dis
    value: Optional[int] = None  # single die value
    chosenValue: Optional[int] = None
    modifier: int = 0
    total: Optional[int] = None
    isNat1: bool = False
    isNat20: bool = False
    difficultyClass: Optional[int] = None  # Difficulty Class value (1-30)

# In-memory storage for current roll
current_roll: Optional[dict] = None
roll_timestamp: float = 0

# In-memory storage for player screen mode
player_screen_mode: str = 'dice'  # 'dice', 'browser', or 'map'
player_screen_browser_url: str = ''

# ============================================
# WebSocket Connection Manager
# ============================================
class ConnectionManager:
    def __init__(self):
        self.active_connections: Set[WebSocket] = set()
    
    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.add(websocket)
        print(f"[WS] Client connected. Total: {len(self.active_connections)}")
    
    def disconnect(self, websocket: WebSocket):
        self.active_connections.discard(websocket)
        print(f"[WS] Client disconnected. Total: {len(self.active_connections)}")
    
    async def broadcast(self, message: dict):
        """Send message to all connected clients"""
        if not self.active_connections:
            return
        data = json.dumps(message)
        disconnected = set()
        for connection in self.active_connections:
            try:
                await connection.send_text(data)
            except Exception:
                disconnected.add(connection)
        # Clean up disconnected
        for conn in disconnected:
            self.active_connections.discard(conn)

ws_manager = ConnectionManager()

@app.websocket("/ws/roll")
async def websocket_roll(websocket: WebSocket):
    """WebSocket endpoint for real-time roll updates and mode changes"""
    await ws_manager.connect(websocket)
    # Send current state immediately on connect
    if current_roll:
        await websocket.send_text(json.dumps({"type": "roll", "data": current_roll}))
    else:
        await websocket.send_text(json.dumps({"type": "clear"}))
    # Send current player screen mode
    await websocket.send_text(json.dumps({
        "type": "mode",
        "data": {
            "mode": player_screen_mode,
            "browserUrl": player_screen_browser_url
        }
    }))
    try:
        while True:
            # Keep connection alive, receive any messages (heartbeat)
            data = await websocket.receive_text()
            # Could handle ping/pong here if needed
    except WebSocketDisconnect:
        ws_manager.disconnect(websocket)

@app.get("/roll")
async def get_roll():
    """Get current roll state for player screen (fallback for non-WS)"""
    global current_roll, roll_timestamp
    # Auto-expire after 60 seconds
    if current_roll and time.time() - roll_timestamp > 60:
        current_roll = None
    return {"roll": current_roll, "timestamp": roll_timestamp}

@app.post("/roll")
async def set_roll(data: RollData):
    """Set current roll state (called by DM) and broadcast via WebSocket"""
    global current_roll, roll_timestamp
    current_roll = data.dict()
    roll_timestamp = time.time()
    print(f"[Roll] Set: {data.status} - {data.playerName} - {data.label}")
    # Broadcast to all WebSocket clients
    await ws_manager.broadcast({"type": "roll", "data": current_roll})
    return {"ok": True}

@app.delete("/roll")
async def clear_roll():
    """Clear current roll state and broadcast via WebSocket"""
    global current_roll, roll_timestamp
    current_roll = None
    roll_timestamp = time.time()
    print("[Roll] Cleared")
    # Broadcast clear to all WebSocket clients
    await ws_manager.broadcast({"type": "clear"})
    return {"ok": True}

@app.post("/player-screen-mode")
async def set_player_screen_mode(data: dict):
    """Set player screen display mode and broadcast via WebSocket"""
    global player_screen_mode, player_screen_browser_url
    mode = data.get("mode", "dice")
    browser_url = data.get("browserUrl", "")
    
    if mode not in ["dice", "browser", "map"]:
        raise HTTPException(status_code=400, detail="Invalid mode. Must be 'dice', 'browser', or 'map'")
    
    player_screen_mode = mode
    player_screen_browser_url = browser_url
    print(f"[PlayerScreen] Mode set: {mode}, browserUrl: {browser_url}")
    
    # Broadcast mode change to all WebSocket clients
    await ws_manager.broadcast({
        "type": "mode",
        "data": {
            "mode": player_screen_mode,
            "browserUrl": player_screen_browser_url
        }
    })
    return {"ok": True, "mode": player_screen_mode, "browserUrl": player_screen_browser_url}

@app.post("/youtube-playback")
async def youtube_playback(data: dict):
    """Handle YouTube playback commands (play/pause/seek) and broadcast via WebSocket"""
    command = data.get("command")
    position = data.get("position")
    
    if command == "seek":
        if position is None or not isinstance(position, (int, float)):
            raise HTTPException(status_code=400, detail="Position required for seek command")
        print(f"[YouTube] Seek command: {position:.2f}s")
        # Broadcast seek command to all WebSocket clients
        await ws_manager.broadcast({
            "type": "youtube_playback",
            "data": {"command": "seek", "position": position}
        })
        return {"ok": True, "command": "seek", "position": position}
    elif command in ["play", "pause"]:
        print(f"[YouTube] Playback command: {command}")
        # Broadcast playback command to all WebSocket clients
        await ws_manager.broadcast({
            "type": "youtube_playback",
            "data": {"command": command}
        })
        return {"ok": True, "command": command}
    else:
        raise HTTPException(status_code=400, detail="Invalid command. Must be 'play', 'pause', or 'seek'")

@app.get("/player-screen-mode")
async def get_player_screen_mode():
    """Get current player screen display mode"""
    global player_screen_mode, player_screen_browser_url
    return {
        "mode": player_screen_mode,
        "browserUrl": player_screen_browser_url
    }

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

