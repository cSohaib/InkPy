# OpenAI examples on InkPy

Both scripts use the OpenAI Responses API. Set `SSID`, `PASSWORD`, and
`API_KEY` near the top of the script, copy it to microSD, then choose Execute
from its long-press menu. Set the correct device date/time before HTTPS requests.

The API key is for the OpenAI API account, whose billing is separate from a
ChatGPT subscription. Both examples use `store: false`.

## agent.py

`agent.py` is deliberately one-shot. It asks for one prompt, sends that prompt
without conversation history, prints the short plain-text answer, and exits.

The HTTP helper stores the API response temporarily at
`/sd/.openai-response.json` and the script removes it on exit.

## document.py

`document.py` asks for a document request such as:

```
difference between Python and MicroPython
```

It enables the Responses API `web_search` tool and requests Structured Output
with exactly two fields: `filename` and `content`. The generated filename ends
in `.md`; the content is a complete Markdown document. Normal Markdown, fenced
code blocks, tables, links, and LaTeX using `$...$` or `$$...$$` are allowed.
Mermaid and HTML are explicitly excluded.

The request uses `stream: true`. InkPy's current native `http()` helper writes
the HTTP/SSE response to `/sd/.openai-document.sse` in bounded chunks. After the
request completes, `document.py` reads that temporary stream incrementally and
writes decoded `response.output_text.delta` text into the final Markdown file
using a small buffer. The complete document is therefore never loaded into the
MicroPython heap.

This is not simultaneous network-to-final-file streaming: the current
`inkpy.http()` API does not expose incoming network chunks to Python. The
temporary SSE file is removed after processing. Existing Markdown files are not
overwritten; a numeric suffix is added when necessary.

The Structured Output schema declares `filename` before `content`. OpenAI
Structured Outputs preserve schema key ordering, allowing the script to know the
destination filename before the potentially large content starts arriving.

No live API/device test is implied by repository-side checks. Device testing is
still required.

API references:
- https://developers.openai.com/api/docs/guides/structured-outputs
- https://developers.openai.com/api/docs/guides/streaming-responses
- https://developers.openai.com/api/docs/guides/tools-web-search
