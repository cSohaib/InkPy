# OpenAI chat on InkPy

Edit `agent.py`: set your Wi-Fi `SSID`, `PASSWORD` and OpenAI `API_KEY`.
Copy it to microSD and choose Execute from its long-press menu. Use the current
Stage 31 firmware; no firmware update or extra Python packages are needed.
Set the correct device date/time for HTTPS. On Stage 34 or later, use
`inkpy.set_time(year, month, day, hour, minute, second)` in Console. The API key uses your OpenAI API
account, which has separate billing from a ChatGPT subscription.

Enter prompts with the keyboard. `/quit` exits; Home → Close also stops the
script. Every request sends all successful user/assistant turns in this run,
plus the new prompt. Failed requests remove that prompt so you can try again.
The instructions request short plain-text answers; output is capped at 256 tokens.
The default model is `gpt-4.1-mini` (editable at the top of the script).

History exists only in Python memory and is never saved or restored. The HTTP
helper uses the reserved SD file `/sd/.openai-response.json`, removed after each
request and on ordinary exit. Hard Close or a reset can interrupt cleanup and
leave the last response there; the next run deletes it. Do not put your own file
at this reserved path. `store: false` disables Responses API application storage;
it is not a promise about all provider retention. Credentials stay in your script.
Long conversations can exhaust the small Python heap; start a fresh chat then.

No live API/device test has been performed for this example. Host mocked tests
check payload history, raw Responses JSON parsing, error recovery and file cleanup.
API reference: https://developers.openai.com/api/docs/guides/conversation-state
and https://developers.openai.com/api/docs/guides/text.
