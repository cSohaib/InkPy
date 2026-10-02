"""Read one indexed page/chapter at a time; no whole-book in-memory index."""
import json
import struct
from pathlib import Path

PAGE = struct.Struct('<QQQII')
RUN = struct.Struct('<HHHHIQ')
CHAPTER = struct.Struct('<QIIHH192s')

def metadata(root):
    data = json.loads((Path(root) / 'manifest.json').read_text())
    if data.get('version') not in (1,2) or not data.get('complete'):
        raise ValueError('incomplete/unsupported cache')
    return data

def page(root, number):
    root = Path(root)
    meta = metadata(root)
    if not 1 <= number <= meta['pages']:
        raise IndexError('page out of range')
    with (root / 'pages.bin').open('rb') as f:
        f.seek((number-1)*PAGE.size)
        begin, end, source, chapter_id, reserved = PAGE.unpack(f.read(PAGE.size))
    if reserved or not 0 <= begin <= end <= (root/'draw.bin').stat().st_size:
        raise ValueError('invalid page index')
    runs = []  # bounded to one page, never the whole document
    with (root/'draw.bin').open('rb') as f:
        f.seek(begin)
        while f.tell() < end:
            if end-f.tell() < RUN.size:
                raise ValueError('truncated run header')
            x,y,cell,style,size,anchor = RUN.unpack(f.read(RUN.size))
            if not 0 < size <= 48002 or f.tell()+size > end or len(runs) >= 8192:
                raise ValueError('invalid run size/count')
            payload=f.read(size)
            if style == 32768:
                height=struct.unpack('<H',payload[:2])[0]
                stride=(cell+7)//8
                if not cell or not height or len(payload)!=2+stride*height or x+cell>meta['width']-16 or y+height>meta['height']-16:
                    raise ValueError('bitmap outside page')
                runs.append(dict(x=x,y=y,width=cell,height=height,style=style,source=anchor,bitmap=payload[2:]))
            else:
                if size>512: raise ValueError('invalid text size')
                text=payload.decode('utf-8',errors='strict')
                if cell == 0 or x < 16 or x+len(text)*cell > meta['width']-16 or y < 16 or y >= meta['height']-16:
                    raise ValueError('run outside page')
                runs.append(dict(x=x,y=y,cell=cell,style=style,source=anchor,text=text))
    return dict(number=number,source=source,chapter=chapter_id,runs=runs)

def chapter(root, number):
    root = Path(root); meta = metadata(root)
    if number == 0:
        return dict(id=0,title='No chapter',page=1,source=0,truncated=False)
    if not 1 <= number <= meta['chapters']:
        raise IndexError('chapter out of range')
    with (root/'chapters.bin').open('rb') as f:
        f.seek((number-1)*CHAPTER.size)
        source,p,ident,n,truncated,title = CHAPTER.unpack(f.read(CHAPTER.size))
    if n >= 192 or ident != number or not 1 <= p <= meta['pages']:
        raise ValueError('invalid chapter record')
    return dict(id=ident,title=title[:n].decode('utf-8') or 'Untitled',page=p,source=source,truncated=bool(truncated))

def page_at_source(root, offset):
    """Reflow restore: find the last page whose anchor is <= the old position."""
    root = Path(root); count=metadata(root)['pages']
    if not count:
        return 0
    lo,hi=0,count
    with (root/'pages.bin').open('rb') as f:
        while lo<hi:
            mid=(lo+hi)//2; f.seek(mid*PAGE.size)
            anchor=PAGE.unpack(f.read(PAGE.size))[2]
            if anchor<=offset: lo=mid+1
            else: hi=mid
    return max(1,lo)
