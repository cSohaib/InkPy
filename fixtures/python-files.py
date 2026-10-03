import os, io, sys, json, time
import local_helper

assert local_helper.answer == 42
with open('text.txt', 'w') as f:
    assert f.write('Hello café\nsecond line\n') > 0
with open('text.txt') as f:
    assert f.readline() == 'Hello café\n'
    assert f.tell() > 0
    f.seek(0)
    assert len(list(f)) == 2
with open('binary.dat', 'wb') as f:
    f.write(bytes(range(256)) * 32)
with open('binary.dat', 'rb') as f:
    data = bytearray(8192)
    assert f.readinto(data) == 8192
    assert data[255] == 255 and data[256] == 0
assert os.stat('binary.dat')[6] == 8192
assert 'text.txt' in os.listdir()
assert json.loads(json.dumps({'answer': 42}))['answer'] == 42
with open('value.json', 'w') as f:
    json.dump({'answer': 42}, f)
with open('value.json') as f:
    assert json.load(f)['answer'] == 42
assert io.StringIO('abc').read() == 'abc'
time.sleep(0.01)
assert time.ticks_ms() >= 0
try:
    os.mkdir('folder')
except OSError:
    pass
with open('folder/rename.txt', 'w') as f:
    f.write('safe')
os.rename('folder/rename.txt', 'folder/moved.txt')
os.remove('folder/moved.txt')
os.rmdir('folder')
try:
    open('/outside-sd.txt', 'w')
    raise AssertionError('escaped SD')
except OSError:
    pass
try:
    open('/sd/.inkpy-reader/draw', 'w')
    raise AssertionError('firmware cache accessible')
except OSError:
    pass
print('Files/imports/Unicode/JSON passed')
