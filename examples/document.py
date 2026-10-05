# InkPy AI Markdown document generator.
# Edit these four values, copy to microSD, then Execute.
SSID = ""
PASSWORD = ""
API_KEY = ""
MODEL = "gpt-4.1-mini"

import inkpy
import json
import os
import time

STREAM_FILE = "/sd/.openai-document.sse"
API_URL = "https://api.openai.com/v1/responses"

INSTRUCTIONS = """Create a complete, useful Markdown document answering the user's request.

The content may use normal Markdown features such as headings, paragraphs, lists,
tables, blockquotes, links, and fenced code blocks. It may use LaTeX math with
$...$ for inline math and $$...$$ for display math. Never use Mermaid diagrams.
Do not use HTML.

Choose a short descriptive filename ending in .md. The filename must contain only
a filename, never a directory path.

If web search is useful, use it. When web sources materially contribute to the
document, include a final Sources section containing ordinary Markdown links so
the saved document remains self-contained and readable outside the API response.
"""

OUTPUT_FORMAT = {
    "type": "json_schema",
    "name": "markdown_document",
    "strict": True,
    "schema": {
        "type": "object",
        "properties": {
            "filename": {
                "type": "string",
                "pattern": r"^[^/\\]+\.md$",
            },
            "content": {
                "type": "string",
            },
        },
        "required": ["filename", "content"],
        "additionalProperties": False,
    },
}


def remove_file(path):
    try:
        os.remove(path)
    except OSError as error:
        if error.args[0] != 2:
            raise


def exists(path):
    try:
        os.stat(path)
        return True
    except OSError:
        return False


def safe_filename(name):
    name = name.strip()
    for bad in '/\\:*?"<>|':
        name = name.replace(bad, "_")
    if not name.lower().endswith(".md"):
        name += ".md"
    if name == ".md":
        name = "document.md"

    stem = name[:-3]
    candidate = name
    number = 2
    while exists("/sd/" + candidate):
        candidate = stem + "-" + str(number) + ".md"
        number += 1
    return candidate


class DocumentWriter:
    START = 0
    KEY_OR_END = 1
    KEY = 2
    COLON = 3
    VALUE = 4
    STRING_VALUE = 5
    COMMA_OR_END = 6
    DONE = 7

    def __init__(self):
        self.state = self.START
        self.key = ""
        self.string = ""
        self.escape = False
        self.unicode_digits = None
        self.pending_high = None
        self.filename = None
        self.path = None
        self.output = None
        self.buffer = []

    def _flush(self):
        if self.output and self.buffer:
            self.output.write("".join(self.buffer))
            self.buffer = []

    def _emit(self, ch):
        if self.state == self.KEY:
            self.string += ch
        elif self.key == "filename":
            self.string += ch
        elif self.key == "content":
            if self.output is None:
                if self.filename is None:
                    raise ValueError("filename must come before content")
                self.output = open(self.path, "w")
            self.buffer.append(ch)
            if len(self.buffer) >= 256:
                self._flush()

    def _emit_codepoint(self, code):
        if 0xD800 <= code <= 0xDBFF:
            self.pending_high = code
            return
        if 0xDC00 <= code <= 0xDFFF and self.pending_high is not None:
            code = 0x10000 + ((self.pending_high - 0xD800) << 10) + (code - 0xDC00)
            self.pending_high = None
            self._emit(chr(code))
            return
        if self.pending_high is not None:
            self._emit("?")
            self.pending_high = None
        self._emit(chr(code))

    def _begin_string(self):
        self.escape = False
        self.unicode_digits = None
        self.pending_high = None

    def _string_char(self, ch):
        if self.unicode_digits is not None:
            self.unicode_digits += ch
            if len(self.unicode_digits) == 4:
                self._emit_codepoint(int(self.unicode_digits, 16))
                self.unicode_digits = None
            return True

        if self.escape:
            self.escape = False
            if ch == "u":
                self.unicode_digits = ""
                return True
            mapped = {
                '"': '"',
                "\\": "\\",
                "/": "/",
                "b": "\b",
                "f": "\f",
                "n": "\n",
                "r": "\r",
                "t": "\t",
            }.get(ch)
            if mapped is None:
                raise ValueError("Invalid JSON escape")
            self._emit(mapped)
            return True

        if ch == "\\":
            self.escape = True
            return True
        if ch == '"':
            return False

        if self.pending_high is not None:
            self._emit("?")
            self.pending_high = None
        self._emit(ch)
        return True

    def feed(self, text):
        for ch in text:
            if self.state == self.START:
                if ch.isspace():
                    continue
                if ch != "{":
                    raise ValueError("Invalid structured output")
                self.state = self.KEY_OR_END

            elif self.state == self.KEY_OR_END:
                if ch.isspace():
                    continue
                if ch == "}":
                    self.state = self.DONE
                elif ch == '"':
                    self.string = ""
                    self._begin_string()
                    self.state = self.KEY
                else:
                    raise ValueError("Expected JSON key")

            elif self.state == self.KEY:
                if self._string_char(ch):
                    continue
                self.key = self.string
                self.string = ""
                self.state = self.COLON

            elif self.state == self.COLON:
                if ch.isspace():
                    continue
                if ch != ":":
                    raise ValueError("Expected ':'")
                self.state = self.VALUE

            elif self.state == self.VALUE:
                if ch.isspace():
                    continue
                if ch != '"':
                    raise ValueError("Expected string value")
                self.string = ""
                self._begin_string()
                if self.key == "content":
                    if self.filename is None:
                        raise ValueError("filename must come before content")
                    self.path = "/sd/" + self.filename
                self.state = self.STRING_VALUE

            elif self.state == self.STRING_VALUE:
                if self._string_char(ch):
                    continue
                if self.key == "filename":
                    self.filename = safe_filename(self.string)
                    self.path = "/sd/" + self.filename
                elif self.key != "content":
                    raise ValueError("Unexpected field: " + self.key)
                self.string = ""
                self.state = self.COMMA_OR_END

            elif self.state == self.COMMA_OR_END:
                if ch.isspace():
                    continue
                if ch == ",":
                    self.state = self.KEY_OR_END
                elif ch == "}":
                    self.state = self.DONE
                else:
                    raise ValueError("Expected ',' or '}'")

            elif self.state == self.DONE:
                if not ch.isspace():
                    raise ValueError("Unexpected data after JSON document")

    def finish(self):
        self._flush()
        if self.output:
            self.output.close()
            self.output = None
        if self.state != self.DONE or not self.filename or not self.path:
            raise ValueError("Incomplete document response")
        if not exists(self.path):
            with open(self.path, "w"):
                pass
        return self.filename

    def abort(self):
        if self.output:
            self.output.close()
            self.output = None
        if self.path:
            remove_file(self.path)


def api_error():
    try:
        with open(STREAM_FILE) as stream:
            error = json.load(stream)
        return error.get("error", {}).get("message", "OpenAI request failed")
    except Exception:
        return "OpenAI request failed"


def write_document_from_stream():
    writer = DocumentWriter()
    try:
        with open(STREAM_FILE) as stream:
            for line in stream:
                if not line.startswith("data:"):
                    continue
                data = line[5:].strip()
                if not data or data == "[DONE]":
                    continue

                event = json.loads(data)
                event_type = event.get("type")

                if event_type == "response.output_text.delta":
                    writer.feed(event.get("delta", ""))
                elif event_type == "error":
                    raise ValueError(event.get("message", "OpenAI stream error"))
                elif event_type == "response.failed":
                    response = event.get("response", {})
                    error = response.get("error", {})
                    raise ValueError(error.get("message", "OpenAI response failed"))

        return writer.finish()
    except Exception:
        writer.abort()
        raise


def generate(prompt):
    remove_file(STREAM_FILE)

    body = json.dumps({
        "model": MODEL,
        "instructions": INSTRUCTIONS,
        "input": prompt,
        "tools": [{"type": "web_search"}],
        "tool_choice": "auto",
        "text": {"format": OUTPUT_FORMAT},
        "store": False,
        "stream": True,
        "max_output_tokens": 16384,
    })

    status = inkpy.http(
        API_URL,
        STREAM_FILE,
        method="POST",
        body=body,
        headers={
            "Authorization": "Bearer " + API_KEY,
            "Content-Type": "application/json",
            "Accept": "text/event-stream",
        },
    )

    if status != 200:
        raise ValueError(api_error())

    return write_document_from_stream()


def main():
    if not SSID or not API_KEY:
        print("Set SSID, PASSWORD and API_KEY in document.py first.")
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

        prompt = input("Document: ").strip()
        if prompt:
            print(generate(prompt))
    except Exception as error:
        print("Error:", error)
    finally:
        inkpy.wifi_off()
        remove_file(STREAM_FILE)


if __name__ == "__main__":
    main()
