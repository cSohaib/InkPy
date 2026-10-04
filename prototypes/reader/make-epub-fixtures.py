"""Generate small reproducible EPUB 2/3 books; no downloaded book or image assets."""
from pathlib import Path
from PIL import Image, ImageDraw
import io, sys, zipfile
root=Path(sys.argv[1]);root.mkdir(parents=True,exist_ok=True)
assets={}
for name,mode,size,fmt in [('cover.jpg','RGB',(1000,700),'JPEG'),('inline.png','RGBA',(32,32),'PNG'),('palette.png','P',(160,80),'PNG'),('mono.png','1',(140,70),'PNG')]:
    im=Image.new(mode,size,(255,255,255) if mode=='RGB' else (255,255,255,0) if mode=='RGBA' else 1)
    if mode=='P':im.putpalette([255,255,255,0,0,0]+[0]*762)
    draw=ImageDraw.Draw(im);draw.rectangle((5,5,size[0]-6,size[1]-6),outline=0 if mode!='RGBA' else (0,0,0,255),width=3)
    draw.line((0,0,size[0]-1,size[1]-1),fill=0 if mode!='RGBA' else (0,0,0,255),width=3)
    b=io.BytesIO();im.save(b,fmt);assets[name]=b.getvalue();(root/name).write_bytes(b.getvalue())
chapter1='''<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Unused title</title><style>p {font-size:99px}</style></head><body><h1>First chapter</h1><p>Inline image <img src="../images/inline.png" alt="icon"/> after image.</p><p>Formula $\\bar{x}=\\frac{\\sum x_i}{n}$.</p><h2>Not a chapter</h2><p>Ordinary section text.</p><h3 id="part">Real TOC section</h3><p><strong>Bold</strong> and <em>italic</em>, café &amp; text.</p><table><tr><th>Name</th><th>Value</th></tr><tr><td>A</td><td>$\\mu$</td></tr></table><img src="../images/palette.png"/><img src="../images/mono.png"/><img src="../images/not-found.png" alt="missing"/></body></html>'''
chapter2='''<html xmlns="http://www.w3.org/1999/xhtml"><body><h1>Second chapter</h1><p>Landscape and cover image.</p><svg xmlns="http://www.w3.org/2000/svg"><image xlink:href="../images/cover.jpg"/></svg><p>End of book.</p></body></html>'''
nav='''<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops"><body><nav epub:type="toc"><ol><li><a href="text/one.xhtml">Chapter One</a><ol><li><a href="text/one.xhtml#part">TOC Section</a></li></ol></li><li><a href="text/two.xhtml">Chapter Two</a></li></ol></nav><nav epub:type="page-list"><a href="text/one.xhtml">Ignored page</a></nav></body></html>'''
ncx='''<ncx><navMap><navPoint><navLabel><text>Chapter One</text></navLabel><content src="text/one.xhtml"/><navPoint><navLabel><text>TOC Section</text></navLabel><content src="text/one.xhtml#part"/></navPoint></navPoint><navPoint><navLabel><text>Chapter Two</text></navLabel><content src="text/two.xhtml"/></navPoint></navMap></ncx>'''
for version in [2,3]:
    for compressed in [False,True]:
        opf=f'''<package version="{version}.0"><metadata/><manifest><item id="one" href="text/one.xhtml" media-type="application/xhtml+xml"/><item id="two" href="text/two.xhtml" media-type="application/xhtml+xml"/><item id="nav" href="{'nav.xhtml' if version==3 else 'toc.ncx'}" media-type="{'application/xhtml+xml' if version==3 else 'application/x-dtbncx+xml'}" {'properties="nav"' if version==3 else ''}/></manifest><spine toc="nav"><itemref idref="one"/><itemref idref="two"/></spine></package>'''
        name=f'epub{version}-{"deflate" if compressed else "store"}.epub'
        with zipfile.ZipFile(root/name,'w',compression=zipfile.ZIP_DEFLATED if compressed else zipfile.ZIP_STORED) as z:
            z.writestr('mimetype','application/epub+zip',compress_type=zipfile.ZIP_STORED)
            z.writestr('META-INF/container.xml','<container><rootfiles><rootfile full-path="OPS/book.opf"/></rootfiles></container>')
            z.writestr('OPS/book.opf',opf);z.writestr('OPS/nav.xhtml' if version==3 else 'OPS/toc.ncx',nav if version==3 else ncx)
            z.writestr('OPS/text/one.xhtml',chapter1);z.writestr('OPS/text/two.xhtml',chapter2)
            for n,data in assets.items():z.writestr('OPS/images/'+n,data)
(root/'images.md').write_text('# Images\n\nAn inline ![icon](inline.png) image.\n\n![cover](cover.jpg)\n\n![palette](palette.png)\n\n![mono](mono.png)\n\n![missing](absent.png)\n')
print('Generated EPUB 2/3 stored/deflate, PNG/JPEG and Markdown image fixtures')

# Stage38 compatibility regressions: malformed nav falls back to NCX; aliased
# namespaces and large publisher attributes; image names with spaces/parentheses.
base=root/'epub3-deflate.epub'
with zipfile.ZipFile(base) as z: original={n:z.read(n) for n in z.namelist()}
for variant in ['nav-ncx-fallback','namespace-images','no-toc']:
    entries=dict(original)
    if variant=='nav-ncx-fallback':
        entries['OPS/nav.xhtml']=b'<html><nav'
        entries['OPS/toc.ncx']=ncx.encode()
        entries['OPS/book.opf']=entries['OPS/book.opf'].replace(b'</manifest>',b'<item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/></manifest>')
    elif variant=='namespace-images':
        entries['OPS/nav.xhtml']=entries['OPS/nav.xhtml'].replace(b'epub:type',b'e:type')
        entries['OPS/text/one.xhtml']=entries['OPS/text/one.xhtml'].replace(b'inline.png',b'inline%20%28small%29.png').replace(b'<p>Ordinary',b'<p id="oversized" style="'+b'x'*5000+b'">Ordinary')
        entries['OPS/images/inline (small).png']=entries.pop('OPS/images/inline.png')
    else:
        entries['OPS/book.opf']=entries['OPS/book.opf'].replace(b'properties="nav"',b'')
    with zipfile.ZipFile(root/(variant+'.epub'),'w',compression=zipfile.ZIP_DEFLATED) as z:
        for name,data in entries.items():z.writestr(name,data)

entries=dict(original)
entries['OPS/text/one.xhtml']=entries['OPS/text/one.xhtml'].replace(b'<h3 id="part">',
    (b'<p>'+b'Heavy chapter text. '*100+b'</p>')*100+b'<h3 id="part">')
with zipfile.ZipFile(root/'heavy.epub','w',compression=zipfile.ZIP_DEFLATED) as z:
    for name,data in entries.items():z.writestr(name,data)
