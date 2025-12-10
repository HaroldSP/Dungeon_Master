#!/usr/bin/env python3
"""
ChatGPT-only Dice Detection Server.
- /health     -> simple health check
- /detect     -> placeholder (model detection disabled)
- /gpt_detect -> uses gpt_dice_integration.detect_with_chatgpt on posted JPEG
"""

from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

try:
    from gpt_dice_integration import detect_with_chatgpt
except Exception:
    detect_with_chatgpt = None


@app.route("/health", methods=["GET"])
def health():
    return jsonify({"status": "ok"}), 200


@app.route("/detect", methods=["POST"])
def detect():
    # Model-based detection removed (no local model)
    return jsonify({"error": "model detection disabled"}), 501


@app.route("/gpt_detect", methods=["POST"])
def gpt_detect():
    if detect_with_chatgpt is None:
        return jsonify({"detected": False, "value": 0, "confidence": 0.0, "error": "gpt_dice_integration not available"}), 500

    image_bytes = request.data or b""
    if not image_bytes:
        return jsonify({"error": "Empty image"}), 400

    try:
        print(
            f"GPT DETECT: from {request.remote_addr}, "
            f"content_type={request.content_type}, "
            f"bytes={len(image_bytes)}"
        )
    except Exception:
        pass

    result = detect_with_chatgpt(image_bytes)

    try:
        print("GPT DETECT result:", result)
    except Exception:
        pass

    return jsonify(result), 200


if __name__ == "__main__":
    print("Starting ChatGPT-only Dice Detection Server...")
    app.run(host="0.0.0.0", port=5000, debug=True)

