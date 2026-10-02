"""Basic script/REPL check after run.sh builds the embedding."""
import subprocess
from pathlib import Path
here=Path(__file__).resolve().parent
root=here.parent.parent
binary=here/'build/inkpy-python'
demo=root/'fixtures/python-demo.py'
script=subprocess.run([str(binary),str(demo)],capture_output=True,text=True)
assert script.returncode==0,script.stdout+script.stderr
for expected in ('[0, 1, 4, 9, 16, 25]','sqrt: 9.0','1208925819614629174706176','caught division by zero'):
    assert expected in script.stdout,script.stdout
repl=subprocess.run([str(binary),'-i',str(demo)],input=(root/'fixtures/python-console.txt').read_text(),capture_output=True,text=True)
assert repl.returncode==0,repl.stdout+repl.stderr
for expected in ('>>> 42','0\n1\n2','>>> 4.0','ZeroDivisionError','still alive'):
    assert expected in repl.stdout,repl.stdout
(here/'build/session.txt').write_text(repl.stdout)
print('OK: MicroPython script, math/bigint, persistent globals, multiline REPL, exception recovery')
