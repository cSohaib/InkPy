"""Host acceptance: boundaries, rejection, chapters, reflow, bounded working set."""
import json
import subprocess
import tempfile
from pathlib import Path
from cache import metadata, page, chapter, page_at_source

HERE = Path(__file__).resolve().parent
BIN = HERE/'build/reader-probe'
checks = 0

def check(ok, message):
    global checks
    assert ok, message
    checks += 1

def run(root, name, data, width=480, height=800, chunk=1024, good=True):
    source=root/(name+'.md'); out=root/name
    source.write_bytes(data)
    p=subprocess.run([str(BIN),str(source),str(out),str(width),str(height),str(chunk)],capture_output=True,text=True)
    check((p.returncode==0)==good, name+': '+p.stderr)
    check(source.read_bytes()==data, 'source modified')
    if good:
        m=metadata(out)
        check(m['parser_peak_bytes']<=m['parser_budget_bytes'], 'parser budget')
        anchors=[]
        for i in range(1,m['pages']+1):
            q=page(out,i); anchors.append(q['source'])
            check(all(r['y']+m['font_pixels']+2*(7-(r['style']>>8))*(bool(r['style']>>8))<=height-16 for r in q['runs']), 'vertical bounds')
        check(anchors==sorted(anchors), 'nonmonotonic page anchors')
    else:
        check(not (out/'manifest.json').exists(),'published failed cache')
    return out

def content(out):
    return ''.join(r['text'] for i in range(1,metadata(out)['pages']+1) for r in page(out,i)['runs'])

with tempfile.TemporaryDirectory(prefix='inkpy-reader-') as tmp:
    root=Path(tmp)
    sample=('\ufeff# Book\r\n\r\n## First café\r\nText **bold** and *italic*, α β 😀 &amp; &#x3c0;.\r\n\r\n'
            '```python\r\n## Not chapter\r\n\r\nprint(1)\r\n```\r\n\r\n'
            'Second\r\n------\r\n\r\n### Not chapter either\r\n\r\n## First café\r\n'+('words flowing on pages. '*200)).encode()
    outputs=[run(root,'boundary'+str(n),sample,chunk=n) for n in (1,2,7,1024)]
    for out in outputs[1:]:
        for f in ('draw.bin','pages.bin','chapters.bin'):
            check((out/f).read_bytes()==(outputs[0]/f).read_bytes(),'I/O boundary changed cache')
    out=outputs[0]
    check([chapter(out,i)['title'] for i in range(1,4)]==['First café','Second','First café'],'H2 chapters')
    check(metadata(out)['chapters']==3,'fake chapter in code')
    check('α β 😀 & π.' in content(out),'Unicode/entity loss')
    for i in range(1,metadata(out)['pages']+1):
        check(page(out,i)==page(out,i),'random access unstable')
        check(page_at_source(out,page(out,i)['source'])>=i,'source lookup')
    landscape=run(root,'landscape',sample,800,480)
    check(metadata(landscape)['chapters']==3,'reflow lost chapters')
    for i in range(1,4):
        c=chapter(landscape,i)
        check(c['source']==chapter(out,i)['source'],'chapter source changed with reflow')
    for n,bad in enumerate((b'\x00',b'\xc0\xaf',b'\xed\xa0\x80',b'\xf4\x90\x80\x80',b'\xe2\x82',b'\x7f',b'\xff')):
        run(root,'invalid'+str(n),b'valid\n\n'*1500+bad,chunk=1,good=False)
    for n,newline in enumerate((b'\n',b'\r',b'\r\n')):
        q=run(root,'newline'+str(n),newline.join((b'## One',b'',b'Text',b'',b'## Two')))
        check(metadata(q)['chapters']==2,'newline chapter handling')
    title=run(root,'title',('## '+'é'*200+'\n').encode())
    check(chapter(title,1)['truncated'] and chapter(title,1)['title']=='é'*95,'UTF8 title truncation')
    for n,data in enumerate((b'Z'*25000+b'\n\n## End\n', b'```\n'+(b'Z'*100+b'\n')*250+b'```\n\n## End\n')):
        q=run(root,'oversize'+str(n),data,chunk=7)
        check(content(q).count('Z')==25000,'literal fallback lost text')
        check(metadata(q)['literal_blocks']==1 and metadata(q)['chapters']==1,'fallback recovery')
    empty=run(root,'empty',b'')
    check(metadata(empty)['pages']==0 and page_at_source(empty,0)==0,'empty document')
    p=subprocess.run([str(BIN),str(root/'empty.md'),str(empty)],capture_output=True)
    check(p.returncode!=0,'existing cache overwritten')
    # Separate child processes; input/output remain disk-backed. Avoid retaining books here.
    stress=[]
    unit=b'## Section\n\n'+b'A bounded paragraph with **bold** text. '*32+b'\n\n'
    for mib in (8,32):
        source=root/('stress'+str(mib)+'.md'); dest=root/('stress'+str(mib))
        with source.open('wb') as f:
            for _ in range(mib*1024*1024//len(unit)): f.write(unit)
        p=subprocess.run([str(BIN),str(source),str(dest)],capture_output=True,text=True)
        check(p.returncode==0,p.stderr)
        m=metadata(dest); stress.append(m)
        check(m['parser_peak_bytes']<=m['parser_budget_bytes'],'stress parser budget')
        for i in (1,m['pages']//2,m['pages']): page(dest,i)
    check(stress[0]['context_bytes']==stress[1]['context_bytes'],'context grows')
    check(stress[0]['parser_peak_bytes']==stress[1]['parser_peak_bytes'],'parser grows')
    check(stress[1]['peak_rss_kib']<=stress[0]['peak_rss_kib']+4096,'working set grows with book')
    report={'checks':checks,'stress':stress}
    (HERE/'build/test-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
