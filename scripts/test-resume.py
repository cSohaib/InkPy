"""Exercise actual checkpoint I/O and editor draft reopen without hardware."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='inkpy-resume-') as tmp:
    directory = Path(tmp)
    source = (root/'main/resume.c').read_text().replace('"/sd/.inkpy-resume', '"'+tmp+'/.inkpy-resume')
    (directory/'resume.c').write_text(source)
    (directory/'test.c').write_text(r'''
#include "resume.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
static ink_resume before,after;
static uint8_t frame[48000],loaded[48000];
int main(int argc,char **argv) {
    assert(argc==2);char path[544],saved[544],next[544];
    snprintf(path,sizeof(path),"%s/draft.md",argv[1]);
    snprintf(saved,sizeof(saved),"%s/.inkpy-resume",argv[1]);
    snprintf(next,sizeof(next),"%s/.inkpy-resume-next",argv[1]);
    FILE *f=fopen(path,"wb");assert(f);fputs("original\n",f);fclose(f);
    ink_editor editor;assert(!ink_editor_open(&editor,path));
    assert(!ink_editor_key(&editor,'X'));assert(!ink_editor_key(&editor,'Y'));
    editor.menu=true;editor.keyboard.shift=true;
    before.editor=editor;before.editor.work=NULL;before.editing=true;
    before.power.night=true;before.power.brightness=70;
    before.reading=true;before.landscape=true;before.page=14;before.document=3;
    strcpy(before.browser.folder,argv[1]);strcpy(before.console.input,"unfinished");
    for(unsigned i=0;i<sizeof(frame);i++)frame[i]=(uint8_t)(i*37+i/113);
    assert(!fflush(editor.work)&&!fsync(fileno(editor.work)));
    fclose(editor.work);editor.work=NULL;
    assert(!ink_resume_save(&before,frame));
    assert(!ink_resume_load(&after,loaded));
    assert(!memcmp(&before,&after,sizeof(before))&&!memcmp(frame,loaded,sizeof(frame)));
    assert(access(saved,F_OK)&&access(next,F_OK));
    assert(!ink_editor_resume(&after.editor));
    assert(after.editor.gap_start==2&&after.editor.menu&&after.editor.keyboard.shift);
    assert(!strcmp(after.editor.lines[0],"XYoriginal"));
    f=fopen(path,"rb");assert(f&&fgetc(f)=='o');fclose(f); /* not autosaved */
    after.editor.menu=false;assert(!ink_editor_key(&after.editor,'Z'));
    assert(!ink_editor_save(&after.editor));
    f=fopen(path,"rb");assert(f);char text[32]={0};fread(text,1,31,f);fclose(f);
    assert(!strcmp(text,"XYZoriginal\n"));
    /* Reject a truncated or corrupted save and consume it. */
    assert(!ink_resume_save(&before,frame));assert(!truncate(saved,20));
    assert(ink_resume_load(&after,loaded)&&access(saved,F_OK));
    assert(!ink_resume_save(&before,frame));f=fopen(saved,"r+b");assert(f);
    assert(!fseek(f,200,SEEK_SET));fputc(42,f);fclose(f);
    assert(ink_resume_load(&after,loaded));
    /* Complete next file recovers an interrupted FatFs rename. */
    assert(!ink_resume_save(&before,frame));assert(!rename(saved,next));
    assert(!ink_resume_load(&after,loaded)&&!memcmp(frame,loaded,sizeof(frame)));
    unlink(path);unlink(before.editor.temporary);
    puts("PASS: checkpoint/frame round-trip, draft cursor/menu/edit/save, original unchanged, corrupt/truncated rejection, rename recovery");
}
''')
    binary = directory/'test'
    subprocess.run(['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-D_FILE_OFFSET_BITS=64',
                    '-Wall','-Wextra','-Werror','-I'+str(root/'main'),
                    '-I'+str(root/'components/ink_browser'),'-I'+str(root/'components/ink_python'),
                    str(directory/'test.c'),str(directory/'resume.c'),
                    str(root/'components/ink_browser/ink_editor.c'),
                    str(root/'components/ink_browser/ink_keyboard.c'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),tmp],check=True)
