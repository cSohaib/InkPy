"""Check default paragraph justification and expanded math on the host."""
from pathlib import Path
import os
import subprocess
import tempfile
from cache import metadata, page

repo = Path(__file__).resolve().parents[2]
exe = repo / 'prototypes/reader/build/math/reader-math'
env = dict(os.environ, INK_MATH_RESOURCES=str(repo / 'prototypes/math/.deps/MicroTeX/res'))

def layout(source, output, width=480, height=800):
    subprocess.run([str(exe), str(source), str(output), str(width), str(height), '73'],
                   env=env, check=True, capture_output=True, text=True)
    meta = metadata(output)
    runs = [run for n in range(1, meta['pages'] + 1) for run in page(output, n)['runs']]
    return meta, runs

with tempfile.TemporaryDirectory(prefix='inkpy-justify-') as temp:
    root = Path(temp)
    source = root / 'prose.md'
    prose = 'alpha beta gamma delta epsilon ' * 20 + 'end.'
    source.write_text(prose + '\n')
    for width, height in [(480, 800), (800, 480)]:
        output = root / str(width)
        meta, runs = layout(source, output, width, height)
        rows = []
        for n in range(1, meta['pages'] + 1):
            by_y = {}
            for run in page(output, n)['runs']:
                by_y.setdefault(run['y'], []).append(run)
            rows.extend(by_y.values())
        for row in rows[:-1]:
            last = row[-1]
            assert last['x'] + len(last['text']) * last['cell'] == width - 8
        last = rows[-1][-1]
        assert last['x'] + len(last['text']) * last['cell'] < width - 8
        restored = ' '.join(''.join(r['text'] for r in row).strip() for row in rows)
        assert restored == prose

    source.write_text('# Short heading\n\nshort final line\n\nfirst hard break  \nsecond hard break\n\n```\n  code    spaces\n```\n')
    _, runs = layout(source, root / 'natural')
    assert all(run['x'] == 8 for run in runs)
    assert any(run.get('text') == '  code    spaces' for run in runs)

    source.write_text(('Mixed café **bold** and `code` with $x+1$ inside prose. ' * 12) + '\n')
    meta, _ = layout(source, root / 'mixed')
    assert meta['formulas'] == 12 and meta['math_fallbacks'] == 0

    source = repo / 'fixtures/markdown-math-expanded.md'
    expected = sum(line.startswith('$') for line in source.read_text().splitlines())
    for width, height in [(480, 800), (800, 480)]:
        meta, _ = layout(source, root / ('math-' + str(width)), width, height)
        assert meta['formulas'] == expected == 64 and meta['math_fallbacks'] == 0

print('PASS: justified wraps/final lines in portrait and landscape; headings/code/hard breaks unchanged; mixed styles/math bounds; 64 expanded formulas without fallback')
