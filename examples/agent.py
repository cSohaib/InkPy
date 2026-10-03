# InkPy: edit these three values, copy to microSD, then Execute.
SSID = ""
PASSWORD = ""
API_KEY = ""
MODEL = "gpt-4.1-mini"

import gc
import inkpy
import json
import os
import time

RESPONSE_FILE = "/sd/.openai-response.json"  # Reserved temporary file.
INSTRUCTIONS = (
    "Reply in plain text only, without Markdown or formatting. "
    "Do not use headings, lists, code fences, or LaTeX. "
    "Keep answers short and concise, normally one to three sentences."
)


def remove_response():
    try:
        os.remove(RESPONSE_FILE)
    except OSError as error:
        if error.args[0] != 2:  # Missing file is fine.
            raise


def ask(history):
    remove_response()
    body = json.dumps({
        "model": MODEL,
        "instructions": INSTRUCTIONS,
        "input": history,
        "store": False,
        "max_output_tokens": 256,
    })
    try:
        status = inkpy.http(
            "https://api.openai.com/v1/responses", RESPONSE_FILE,
            method="POST", body=body,
            headers={"Authorization": "Bearer " + API_KEY,
                     "Content-Type": "application/json"},
        )
        del body
        with open(RESPONSE_FILE) as stream:
            response = json.load(stream)
    finally:
        remove_response()
    if status != 200:
        raise ValueError(response.get("error", {}).get("message", "HTTP " + str(status)))
    parts = []
    for item in response.get("output", []):
        if item.get("type") == "message":
            for part in item.get("content", []):
                if part.get("type") == "output_text":
                    parts.append(part["text"])
                elif part.get("type") == "refusal":
                    parts.append(part["refusal"])
    text = "\n".join(parts).strip()
    if not text:
        raise ValueError("No reply received")
    return text


def main():
    if not SSID or not API_KEY:
        print("Set SSID, PASSWORD and API_KEY in agent.py first.")
        return
    history = []
    try:
        remove_response()
        inkpy.wifi(SSID, PASSWORD)
        for attempt in range(150):
            if inkpy.wifi_status()[0]:
                break
            time.sleep(0.1)
        else:
            print("Wi-Fi connection failed.")
            return
        print("/quit to exit")
        while True:
            prompt = input("You: ").strip()
            if prompt == "/quit":
                return
            if not prompt:
                continue
            history.append({"role": "user", "content": prompt})
            try:
                reply = ask(history)
            except MemoryError:
                print("Memory full. Start a new chat.")
                return
            except Exception as error:
                history.pop()
                print("Error:", error)
            else:
                history.append({"role": "assistant", "content": reply})
                print(reply)
            gc.collect()
    finally:
        history.clear()
        inkpy.wifi_off()
        remove_response()


if __name__ == "__main__":
    main()
