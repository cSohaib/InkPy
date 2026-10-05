"""Small publication checks; not a complete secret/security audit."""
import ast
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
tracked = subprocess.check_output(["git", "ls-files", "-z"], cwd=root).split(b"\0")
tokens = re.compile(rb"(?:sk-(?:proj-)?[A-Za-z0-9_-]{30,}|gh[pousr]_[A-Za-z0-9]{30,}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----)")
errors = []
for entry in tracked:
    if not entry:
        continue
    path = root / entry.decode()
    data = path.read_bytes()
    if tokens.search(data):
        errors.append(str(path.relative_to(root)) + ": possible credential")
    if path.suffix == ".py":
        try:
            ast.parse(data, filename=str(path))
        except SyntaxError as exc:
            errors.append(str(exc))
# Check maintained public pages, not historical prototype reports.
for name in ("README.md", "THIRD_PARTY.md", "SECURITY.md", "docs/README.md", "docs/BUILD.md"):
    page = root / name
    for target in re.findall(r"\]\(([^)]+)\)", page.read_text()):
        if "://" not in target and not target.startswith("#"):
            if not (page.parent / target.split("#")[0]).exists():
                errors.append(name + ": missing link " + target)
if errors:
    raise SystemExit("\n".join(errors))
print("Public documentation links, Python syntax and credential-pattern scan passed")
