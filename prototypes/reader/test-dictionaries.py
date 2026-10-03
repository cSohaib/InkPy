"""Generated original test data: no third-party dictionary downloads required."""
from pathlib import Path
import gzip, struct, subprocess, sys, tempfile, zlib

def dictzip(data):
    compressor=zlib.compressobj(wbits=-15)
    parts=[]
    for start in range(0,len(data),32768):
        parts.append(compressor.compress(data[start:start+32768])+compressor.flush(zlib.Z_FULL_FLUSH))
    end=compressor.flush(zlib.Z_FINISH)
    table=struct.pack('<HHH',1,32768,len(parts))+b''.join(struct.pack('<H',len(p)) for p in parts)
    extra=b'RA'+struct.pack('<H',len(table))+table
    return b'\x1f\x8b\x08\x04'+b'\0'*6+struct.pack('<H',len(extra))+extra+b''.join(parts)+end+struct.pack('<II',zlib.crc32(data),len(data))

def create(root,name,entries,types='m',wide=False,compressed=False,synonyms=()):
    folder=root/'dictionaries'/name;folder.mkdir(parents=True)
    data=bytearray();index=bytearray()
    for word,definition in entries:
        index+=word.encode()+b'\0'+struct.pack('>QI' if wide else '>II',len(data),len(definition))
        data+=definition
    info=f"StarDict's dict ifo file\nversion=3.0.0\nbookname={name}\nwordcount={len(entries)}\nidxfilesize={len(index)}\n"
    if types:info+=f'sametypesequence={types}\n'
    if wide:info+='idxoffsetbits=64\n'
    if synonyms:
        info+=f'synwordcount={len(synonyms)}\n'
        (folder/(name+'.syn')).write_bytes(b''.join(w.encode()+b'\0'+struct.pack('>I',i) for w,i in synonyms))
    (folder/(name+'.ifo')).write_text(info)
    (folder/(name+('.idx.gz' if compressed else '.idx'))).write_bytes(gzip.compress(index,mtime=0) if compressed else index)
    (folder/(name+('.dict.dz' if compressed else '.dict'))).write_bytes(dictzip(data) if compressed else data)

def fixtures(root):
    create(root,'plain',[('Apple',b'a red fruit'),('apple',b'second meaning'),('café',b'coffee'),('z'*40,b'wrapped word definition')],synonyms=[('pomme',0),('pomme',1)])
    create(root,'zip',[('large',b'large definition '*8000),('markup','<p>fish &amp; chips</p><div>caf&#233;</div>'.encode())],types='h',compressed=True)
    create(root,'typed',[('empty',b'\0'),('word',b'phonetic\0meaning')],types='tm')
    create(root,'mixed',[('word',b'tfirst\0P'+struct.pack('>I',3)+b'123'+b'mlast\0')],types='')
    create(root,'wide',[('word',b'm64 bit offset\0')],types='',wide=True)
    create(root,'invalid-text',[('word',b'\xff')])
    create(root,'bad-index',[('z',b'last'),('a',b'first')])
    create(root,'bad-gzip',[('word',b'definition')],compressed=True)
    p=root/'dictionaries/bad-gzip/bad-gzip.dict.dz';data=bytearray(p.read_bytes());data[-8]^=1;p.write_bytes(data)
    create(root,'bad-size',[('word',b'definition')]);p=root/'dictionaries/bad-size/bad-size.dict';p.write_bytes(b'')
    create(root,'xdxf',[('word',b'<k>word</k><def>XML meaning</def>')],types='x')
    create(root,'binary-last',[('word',b'plain text\0'+b'123')],types='mP')

if __name__=='__main__':
    with tempfile.TemporaryDirectory(prefix='inkpy-dictionary-test-') as directory:
        root=Path(directory);fixtures(root)
        subprocess.run([sys.argv[1],str(root)],check=True)
        if len(sys.argv)>2:subprocess.run([sys.argv[2],str(root)],check=True)
