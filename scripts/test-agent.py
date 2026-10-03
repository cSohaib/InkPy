"""Offline integration check; no credentials, network, or device required."""
import builtins
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import types

root = Path(__file__).resolve().parents[1]
requests = []
responses = [
    (200, {"output": [{"type": "reasoning"}, {"type": "message", "content": [
        {"type": "output_text", "text": "Hello."}]}]}),
    (429, {"error": {"message": "Try later"}}),
    (200, {"output": [{"type": "message", "content": [
        {"type": "output_text", "text": "I remember."}]}]}),
]
stopped = []

def http(url, destination, **kwargs):
    assert url == "https://api.openai.com/v1/responses"
    assert kwargs["method"] == "POST"
    assert kwargs["headers"]["Authorization"] == "Bearer test-key"
    requests.append(json.loads(kwargs["body"]))
    status, response = responses.pop(0)
    Path(destination).write_text(json.dumps(response))
    return status

sys.modules["inkpy"] = types.SimpleNamespace(
    wifi=lambda ssid, password: None, wifi_status=lambda: (True, "127.0.0.1"),
    wifi_off=lambda: stopped.append(True), http=http)
spec = importlib.util.spec_from_file_location("agent", root / "examples/agent.py")
agent = importlib.util.module_from_spec(spec)
spec.loader.exec_module(agent)
agent.SSID, agent.API_KEY = "test", "test-key"
with tempfile.TemporaryDirectory() as directory:
    agent.RESPONSE_FILE = str(Path(directory) / "response.json")
    Path(agent.RESPONSE_FILE).write_text("old interrupted response")
    prompts = iter(["hello", "failed prompt", "remember?", "/quit"])
    old_input = builtins.input
    output = io.StringIO()
    try:
        builtins.input = lambda prompt: next(prompts)
        with contextlib.redirect_stdout(output):
            agent.main()
    finally:
        builtins.input = old_input
    assert not Path(agent.RESPONSE_FILE).exists()
assert requests[0]["input"] == [{"role": "user", "content": "hello"}]
assert requests[2]["input"] == [
    {"role": "user", "content": "hello"},
    {"role": "assistant", "content": "Hello."},
    {"role": "user", "content": "remember?"},
]
assert all(r["store"] is False and r["max_output_tokens"] == 256 for r in requests)
assert "without Markdown" in requests[0]["instructions"]
assert "Try later" in output.getvalue() and "I remember." in output.getvalue()
assert stopped == [True]
print("agent: full history, failure rollback, Responses parsing, cleanup PASS")
