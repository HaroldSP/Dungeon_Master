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

    # Prompt engineered for a single standard D20 in the frame
    system_prompt = (
        "You are an assistant that looks at images of a single standard 20-sided dice (D20).\n"
        "- The task is to identify the value on the face that is pointing straight up on the die.\n"
        "- The die can be in any orientation (rotated, tilted, or upside down), so digits may appear at arbitrary angles.\n"
        "- A standard D20 layout means that you usually see the top face plus roughly three neighboring faces.\n"
        "- CRITICAL: Opposite-face pairs on a standard D20 sum to 21, approximately as follows:\n"
        "    1 ↔ 20\n"
        "    2 ↔ 19\n"
        "    3 ↔ 18\n"
        "    4 ↔ 17\n"
        "    5 ↔ 16\n"
        "    6 ↔ 15\n"
        "    7 ↔ 14\n"
        "    8 ↔ 13\n"
        "    9 ↔ 12\n"
        "    10 ↔ 11\n"
        "- IMPORTANT CONSTRAINT: If you predict that the top face is a certain number, it is IMPOSSIBLE for the "
        "neighboring faces to include its opposite number. For example:\n"
        "    - If top face is 20, the neighboring faces CANNOT include 1\n"
        "    - If top face is 15, the neighboring faces CANNOT include 6\n"
        "    - If top face is 7, the neighboring faces CANNOT include 14\n"
        "    - And so on for all opposite pairs\n"
        "Use this as a consistency check: if you see an opposite pair both visible, your top face prediction is wrong.\n"
        "- The following table (TOPFACE(left,right,down)) is an approximate example of three neighbors for each top face. "
        "The exact arrangement may differ slightly on some dice, but you can use it as a consistency check:\n"
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
        "- CRITICAL FOR NUMBERS 6 AND 9: These are very tricky because the die can be rolled upside down. "
        "Follow this step-by-step algorithm when you think the top face might be either 6 or 9:\n"
        "    STEP 1: Look at the neighboring numbers visible in the image. If one of the neighbors is also 6 or 9 "
        "(the tricky pair), IGNORE that ambiguous neighbor and use ONLY the other neighboring numbers.\n"
        "    Example: If you see neighbors (6, 11, 19), ignore the '6' (since it could be 6 or 9) and use only 11 and 19.\n"
        "    STEP 2: Cross-reference the remaining neighboring numbers (11 and 19) with the table above.\n"
        "    These numbers (11, 19) appear in the table entry for 9: 9(6,11,19), so the top face is most likely 9.\n"
        "    STEP 3: If confidence between 6 and 9 is still very close after using the table, use secondary attributes:\n"
        "        - The small single dot (·) position: closer to rounded part = 6, closer to tail = 9\n"
        "    STEP 4: CRITICAL - The final answer MUST be either 6 or 9, NEVER a neighboring number. "
        "You are determining the TOP FACE value, not the neighbors.\n"
        "- CRITICAL FOR NUMBERS 1 AND 7: These can be confused, especially when rotated. "
        "Follow this step-by-step algorithm when you think the top face might be either 1 or 7:\n"
        "    STEP 1: Look at the neighboring numbers visible in the image. If one of the neighbors is also 1 or 7 "
        "(the tricky pair), IGNORE that ambiguous neighbor and use ONLY the other neighboring numbers.\n"
        "    Example: If you see neighbors (7, 19, 13), ignore the '7' (since it could be 1 or 7) and use only 19 and 13.\n"
        "    STEP 2: Cross-reference the remaining neighboring numbers (19 and 13) with the table above.\n"
        "    These numbers (19, 13) appear in the table entry for 1: 1(7,19,13), so the top face is most likely 1.\n"
        "    STEP 3: If confidence between 1 and 7 is still very close after using the table, use secondary attributes:\n"
        "        - The upper part (top stroke) of 7 is typically longer than that of 1\n"
        "        - Number 1 is usually a simple vertical line, while 7 has a horizontal top stroke\n"
        "    STEP 4: CRITICAL - The final answer MUST be either 1 or 7, NEVER a neighboring number. "
        "You are determining the TOP FACE value, not the neighbors.\n"
        "- You will often see the top face plus three neighboring faces. One example (not the only possible orientation):\n"
        "    If 15 is the top face, the three neighboring visible faces might be:\n"
        "      5 on the left, 12 on the right, and 7 toward the bottom.\n"
        "Use these geometric and neighborhood cues when reasoning about the top value.\n"
        "You must answer ONLY with a JSON object describing the upward facing value.\n"
        "If you are not at least 60% sure, set detected:false and value:0."
    )
    user_prompt = (
        "Look at this photo of a single standard D20 dice.\n"
        "- Determine the number on the face that is facing straight up (the rolled value).\n"
        "- Take into account that the die can be rotated or upside down and that neighboring faces can help you verify the guess.\n"
        "- Remember: opposite numbers (summing to 21) CANNOT both be visible as neighbors.\n"
        "- For numbers 6 and 9: Follow the step-by-step algorithm above. If a neighbor is 6 or 9, ignore it and use "
        "the OTHER neighbors to look up in the table. The final answer must be 6 or 9, never a neighbor.\n"
        "- For numbers 1 and 7: Follow the step-by-step algorithm above. If a neighbor is 1 or 7, ignore it and use "
        "the OTHER neighbors to look up in the table. The final answer must be 1 or 7, never a neighbor.\n"
        "Respond ONLY with JSON using this schema and nothing else:\n"
        "{\n"
        '  \"detected\": true or false,\n'
        '  \"value\": integer from 1 to 20 (0 if not sure),\n'
        '  \"confidence\": number between 0 and 1\n'
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
            max_output_tokens=150,
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

        data = json.loads(text)
        detected = bool(data.get("detected", False))
        value = int(data.get("value", 0))
        confidence = float(data.get("confidence", 0.0))

        # Clamp to safe ranges
        if not (1 <= value <= 20):
            value = 0
            detected = False

        return {
            "detected": detected,
            "value": value,
            "confidence": confidence,
            "x": 0,
            "y": 0,
            "w": 0,
            "h": 0,
        }

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


