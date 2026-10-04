"""Host-only bitmap preview. Monospaced codepoint layout, no text shaping.
Usage: preview.py CACHE_DIR PAGE_NUMBER OUTPUT.png [FONT_DIRECTORY]
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from cache import metadata, page

root,number,output=sys.argv[1:4]
fonts=Path(sys.argv[4] if len(sys.argv)>4 else '/usr/share/fonts/truetype/dejavu')
meta=metadata(root); data=page(root,int(number))
im=Image.new('1',(meta['width'],meta['height']),1); draw=ImageDraw.Draw(im)
loaded={}
for run in data['runs']:
    if run.get('rule'):
        draw.rectangle((run['x'],run['y'],run['x']+run['width']-1,run['y']+run['height']-1),fill=0)
        continue
    if 'bitmap' in run:
        # Cache uses black=1; Pillow's 1-bit image uses white=1.
        from PIL import ImageChops
        tile=Image.frombytes('1',(run['width'],run['height']),run['bitmap'])
        im.paste(ImageChops.invert(tile),(run['x'],run['y']))
        continue
    level=(run['style']>>8)&7
    size=meta['font_pixels']+(2*(7-level) if level else 0)
    bold=bool(run['style']&1 or level); italic=bool(run['style']&2)
    name='DejaVuSansMono'+('-BoldOblique' if bold and italic else '-Bold' if bold else '-Oblique' if italic else '')+'.ttf'
    key=(name,size)
    if key not in loaded:
        loaded[key]=ImageFont.truetype(str(fonts/name),size)
    x=run['x']
    for char in run['text']:
        draw.text((x,run['y']),char,font=loaded[key],fill=0,anchor='lt')
        x+=run['cell']
im.save(output)
