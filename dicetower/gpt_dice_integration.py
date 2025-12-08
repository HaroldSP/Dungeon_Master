"""
Integration helpers for using ChatGPT (OpenAI) to recognize the value
on a D20 dice face from an image.

This code is separated from the main server so that you can easily
enable/disable or experiment with prompts and models without touching
the core HTTP glue.

Requirements:
- `pip install openai` (and ensure it's added to dice_detection_server_requirements.txt)
- Set environment variable: OPENAI_API_KEY=sk-...

Security note:
- Do NOT hardcode your API key in this file. Use environment variables.
"""

import os
from typing import Dict, Any
import base64

try:
    from openai import OpenAI  # New-style OpenAI client
except ImportError:
    OpenAI = None  # Will be checked at runtime

# Load environment variables from .env if python-dotenv is available.
# This lets you keep OPENAI_API_KEY in a local .env file.
try:
    from dotenv import load_dotenv

    load_dotenv()
except Exception:
    pass


def detect_with_chatgpt(image_bytes: bytes) -> Dict[str, Any]:
    """
    Use a vision-capable ChatGPT model to guess the top value of a D20 die.

    Returns a dict with the same shape as the regular detector:
    {
      "detected": bool,
      "value": int,
      "confidence": float,
      "x": 0, "y": 0, "w": 0, "h": 0
    }
    """
    api_key = os.environ.get("OPENAI_API_KEY") or os.environ.get("OPENAI_API_KEY_DICE")

    if OpenAI is None or not api_key:
        # Library or API key not available
        return {
            "detected": False,
            "value": 0,
            "confidence": 0.0,
            "x": 0,
            "y": 0,
            "w": 0,
            "h": 0,
            "error": "OpenAI client not installed or API key not set",
        }

    client = OpenAI(api_key=api_key)

    # Detailed prompt for a single standard D20 (icosahedron) using face-adjacency consistency checks
    # Also known as dual-graph adjacency reasoning on an icosahedron
    system_prompt = (
        "You are an expert assistant that analyzes images of a single standard 20-sided die (D20) to identify the TOP face value.\n"
        "- PRIMARY GOAL: Report the number on the face pointing straight up (the rolled result).\n"
        "- Orientation: The die may be rotated/tilted; digits can appear at arbitrary angles.\n"
        "- Typically you see the top face plus about three neighboring faces.\n"
        "- The top face is most centered and faces the camera. Use POSITION/ORIENTATION, not readability.\n"
        "\n"
        "OPPOSITE-FACE PAIRS (sum to 21): 1↔20, 2↔19, 3↔18, 4↔17, 5↔16, 6↔15, 7↔14, 8↔13, 9↔12, 10↔11.\n"
        "If your predicted top appears together with its opposite as a neighbor, your guess is wrong.\n"
        "\n"
        "EXACT NEIGHBOR TABLE (TOPFACE(left,right,down)):\n"
        "    1(7,19,13)\n"
        "    2(12,18,20)\n"
        "    3(17,16,19)\n"
        "    4(18,11,14)\n"
        "    5(18,15,13)\n"
        "    6(9,16,14)\n"
        "    7(15,17,1)\n"
        "    8(16,10,20)\n"
        "    9(6,11,19)\n"
        "    10(17,12,8)\n"
        "    11(9,4,13)\n"
        "    12(10,15,2)\n"
        "    13(11,5,1)\n"
        "    14(4,6,20)\n"
        "    15(5,12,7)\n"
        "    16(6,3,8)\n"
        "    17(10,3,7)\n"
        "    18(5,4,2)\n"
        "    19(3,9,1)\n"
        "    20(2,14,8)\n"
        "Note: left/right/down labels match this table when the top number is in its usual horizontal (upright) reading. Use these as adjacency checks even when rotated.\n"
        "\n"
        "TRICKY PAIRS (face-adjacency / dual-graph reasoning):\n"
        "- 6 vs 9 algorithm:\n"
        "  1) List all visible numbers.\n"
        "  2) If a neighbor is also 6 or 9, ignore that ambiguous neighbor; use other neighbors.\n"
        "  3) Cross-check remaining neighbors: 9 expects (6,11,19); 6 expects (9,16,14).\n"
        "  4) If still close, use visual cues: the small dot is nearer the rounded part for 6, nearer the tail for 9.\n"
        "  5) Final answer must be 6 or 9 (never a neighbor).\n"
        "- 1 vs 7 algorithm:\n"
        "  1) List all visible numbers.\n"
        "  2) If a neighbor is 1 or 7, ignore that ambiguous neighbor; use other neighbors.\n"
        "  3) Cross-check remaining neighbors: 1 expects (7,19,13); 7 expects (15,17,1).\n"
        "  4) If still close, use visual cues: 7 usually has a horizontal top stroke; 1 is a vertical line.\n"
        "  5) Final answer must be 1 or 7 (never a neighbor).\n"
        "\n"
        "VERIFICATION STEPS:\n"
        "  1) List every visible number you can read in the image.\n"
        "  2) From that set, pick the face that is most central/facing the camera as the top candidate.\n"
        "  3) Identify NEIGHBORS: the three faces immediately adjacent around that top (left/right/down).\n"
        "  4) Validate against the exact neighbor table: neighbors must match your candidate; if 0–1 match, reject and try another candidate from the visible set.\n"
        "  5) Check opposites: top + its opposite cannot both appear as neighbors.\n"
        "  6) Apply the special 6/9 or 1/7 algorithm if relevant.\n"
        "  7) Final answer must be 1–20 and must be the top face, not a neighbor.\n"
        "\n"
        "Confidence rule: if below ~60% sure, set detected=false and value=0.\n"
        "You must answer ONLY with JSON.\n"
    )
    user_prompt = (
        "Analyze this image of a single standard D20.\n"
        "\n"
        "PROCESS:\n"
        "1) Identify every visible number in the image.\n"
        "2) Choose the top face by position/orientation: most centered and facing the camera (not by readability).\n"
        "3) Identify neighbors: the three faces immediately adjacent around the chosen top (left/right/down).\n"
        "4) Use the exact neighbor table to validate; if neighbors don't match, pick another candidate from the visible set.\n"
        "5) Apply special handling for 6/9 or 1/7 if needed.\n"
        "6) Also provide the second most likely top face.\n"
        "\n"
        "RULES:\n"
        "- Neighbors must match the table entry for the candidate top.\n"
        "- Opposite faces (sum to 21) cannot both be neighbors.\n"
        "- For 6/9 or 1/7, ignore ambiguous neighbors in that pair and use the others to match the table.\n"
        "- Final answer must be 1–20 and must be the top face, never a neighbor.\n"
        "\n"
        "Respond ONLY with JSON:\n"
        "{\n"
        '  \"detected\": true or false,\n'
        '  \"value\": integer from 1 to 20 (0 if not sure),\n'
        '  \"confidence\": number between 0 and 1,\n'
        '  \"neighbors\": array of integers (e.g. [7, 19, 13]),\n'
        '  \"second_most_likely\": integer (0 if not applicable)\n'
        "}\n"
    )

    # We send one "message" with text + image, using the new Responses API.
    # Adjust model name if you have access to a different vision-capable model.
    try:
        # Encode image bytes as base64 data URL for the Responses API
        b64_image = base64.b64encode(image_bytes).decode("ascii")
        image_data_url = f"data:image/jpeg;base64,{b64_image}"

        result = client.responses.create(
            model="gpt-4.1-mini",  # or gpt-4o-mini, gpt-4.1, etc.
            input=[
                {
                    "role": "system",
                    "content": [{"type": "input_text", "text": system_prompt}],
                },
                {
                    "role": "user",
                    "content": [
                        {"type": "input_text", "text": user_prompt},
                        {
                            "type": "input_image",
                            "image_url": image_data_url,
                        },
                    ],
                },
            ],
            max_output_tokens=200,  # Increased for bounding box coordinates
        )

        # Extract the text output (JSON string)
        text = ""
        try:
            text = result.output_text
        except Exception:
            # Fallback for older client versions
            if result.output and result.output[0].content:
                text = result.output[0].content[0].text

        import json

        # Clean up the text - remove markdown code blocks if present
        if text:
            text = text.strip()
            # Remove markdown code blocks if ChatGPT wrapped the JSON
            if text.startswith("```"):
                lines = text.split("\n")
                text = "\n".join(lines[1:-1]) if len(lines) > 2 else text
            elif text.startswith("```json"):
                lines = text.split("\n")
                text = "\n".join(lines[1:-1]) if len(lines) > 2 else text

        if not text or not text.strip():
            return {
                "detected": False,
                "value": 0,
                "confidence": 0.0,
                "neighbors": [],
                "second_most_likely": 0,
                "error": "Empty response from ChatGPT"
            }

        try:
            data = json.loads(text)
        except json.JSONDecodeError as e:
            return {
                "detected": False,
                "value": 0,
                "confidence": 0.0,
                "neighbors": [],
                "second_most_likely": 0,
                "error": f"JSON parse error: {str(e)}. Response was: {text[:200]}"
            }
        detected = bool(data.get("detected", False))
        value = int(data.get("value", 0))
        confidence = float(data.get("confidence", 0.0))
        
        # Extract debug info
        neighbors = data.get("neighbors", [])
        if isinstance(neighbors, list):
            neighbors = [int(n) for n in neighbors if isinstance(n, (int, float)) and 1 <= int(n) <= 20]
        else:
            neighbors = []
        second_most_likely = int(data.get("second_most_likely", 0))
        if not (1 <= second_most_likely <= 20):
            second_most_likely = 0

        # Clamp to safe ranges
        if not (1 <= value <= 20):
            value = 0
            detected = False

        result = {
            "detected": detected,
            "value": value,
            "confidence": confidence,
            "x": 0,
            "y": 0,
            "w": 0,
            "h": 0,
        }
        
        # Add debug info for logging
        if neighbors:
            result["neighbors"] = neighbors
        if second_most_likely > 0:
            result["second_most_likely"] = second_most_likely
        
        return result

    except Exception as e:
        # On any error, return a non-detection with the error message for debugging
        return {
            "detected": False,
            "value": 0,
            "confidence": 0.0,
            "x": 0,
            "y": 0,
            "w": 0,
            "h": 0,
            "error": str(e),
        }


