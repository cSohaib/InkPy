"""Compile the actual transfer functions against a recording SPI stub."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = (root / 'main/display.c').read_text()
functions = src[src.index('static esp_err_t send_transfer('):src.index('static esp_err_t bytes(')]
functions += src[src.index('static esp_err_t plane('):src.index('esp_err_t ink_display_update(')]
prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
typedef int esp_err_t;
enum { ESP_OK=0, PIN_EPD_CS, PIN_EPD_DC, INK_SSD1677, INK_UC8179, INK_UC8279 };
enum { PANEL_STRIDE=100, PANEL_BYTES=48000 };
static int spi,panel,cs,calls,fail_at;
static uint8_t transfer[4000],received[60000];
static size_t used;
typedef struct { size_t length; const void *tx_buffer; } spi_transaction_t;
static esp_err_t spi_device_polling_transmit(int device,const spi_transaction_t *t) {
    (void)device; assert(t->tx_buffer==transfer&&t->length%8==0);
    size_t n=t->length/8; assert(n<=sizeof(transfer));
    if(++calls==fail_at) return -1;
    assert(used+n<=sizeof(received)); memcpy(received+used,t->tx_buffer,n); used+=n;
    return 0;
}
static void gpio_set_level(int pin,int value) { if(pin==PIN_EPD_CS) cs=value; }
static int command(uint8_t c,const uint8_t *data,size_t n) { (void)c;(void)data;assert(!n);return 0; }
#define TRY(expr) do { int e=(expr); if(e) return e; } while(0)
'''
suffix = r'''
int main(void) {
    uint8_t frame[PANEL_BYTES];
    for(unsigned i=0;i<sizeof(frame);i++) frame[i]=(uint8_t)(i*37+i/113);
    int panels[]={INK_SSD1677,INK_UC8179,INK_UC8279};
    for(unsigned p=0;p<3;p++) for(unsigned white=0;white<2;white++) {
        panel=panels[p]; used=0;calls=0;fail_at=0;
        assert(!plane(0x13,white?NULL:frame)&&cs==1);
        unsigned rows=panel==INK_SSD1677?480:600;
        assert(used==rows*100&&calls==(int)(rows/40));
        for(unsigned y=0;y<rows;y++) for(unsigned x=0;x<100;x++) {
            uint8_t expected=255;
            if(!white) {
                if(panel==INK_SSD1677) expected=frame[y*100+x];
                else if(panel==INK_UC8179&&y<480) expected=frame[(479-y)*100+x];
                else if(panel==INK_UC8279&&y>=120) expected=frame[(y-120)*100+x];
            }
            assert(received[y*100+x]==expected);
        }
    }
    used=0;calls=0;fail_at=3;
    assert(plane(0x13,frame)==-1&&cs==1&&calls==3&&used==8000);
    puts("PASS: actual display transfer byte order/padding for all panels, 12/15 batches, failure cleanup");
}
'''
with tempfile.TemporaryDirectory(prefix='inkpy-spi-test-') as directory:
    source = Path(directory) / 'probe.c'
    binary = Path(directory) / 'probe'
    source.write_text(prefix + functions + suffix)
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
