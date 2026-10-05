# InkPy: edit these four values, copy to microSD, then Execute.
SSID = ""
PASSWORD = ""
API_KEY = ""
MODEL = "gpt-4.1-mini"

import inkpy
import json
import os
import time

RESPONSE_FILE = "/sd/.openai-response.json"
INSTRUCTIONS = (
    "Reply in plain text only, without Markdown or formatting. "
    "Keep the answer short and concise."
)


def remove_response():
    try:
        os.remove(RESPONSE_FILE)
    except OSError as error:
        if error.args[0] != 2:
            raise


def ask(prompt):
    remove_response()
    body = json.dumps({
        "model": MODEL,
        "instructions": INSTRUCTIONS,
        "input": prompt,
        "store": False,
        "max_output_tokens": 256,
    })
    try:
        status = inkpy.http(
            "https://api.openai.com/v1/responses",
            RESPONSE_FILE,
            method="POST",
            body=body,
            headers={
                "Authorization": "Bearer " + API_KEY,
                "Content-Type": "application/json",
            },
        )
        with open(RESPONSE_FILE) as stream:
            response = json.load(stream)
    finally:
        remove_response()

    if status != 200:
        raise ValueError(
            response.get("error", {}).get("message", "HTTP " + str(status))
        )

    for item in response.get("output", []):
        if item.get("type") == "message":
            for part in item.get("content", []):
                if part.get("type") == "output_text":
                    return part["text"].strip()
                if part.get("type") == "refusal":
                    return part["refusal"].strip()
    raise ValueError("No reply received")


def main():
    if not SSID or not API_KEY:
        print("Set SSID, PASSWORD and API_KEY in agent.py first.")
        return

    try:
        inkpy.wifi(SSID, PASSWORD)
        for _ in range(150):
            if inkpy.wifi_status()[0]:
                break
            time.sleep(0.1)
        else:
            print("Wi-Fi connection failed.")
            return

        prompt = input("You: ").strip()
        if prompt:
            print(ask(prompt))
    except Exception as error:
        print("Error:", error)
    finally:
        inkpy.wifi_off()
        remove_response()


if __name__ == "__main__":
    main()
