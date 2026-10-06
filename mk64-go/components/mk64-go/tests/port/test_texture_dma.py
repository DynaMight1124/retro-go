"""Exercise PI source selection for extracted texture descriptors."""
from pathlib import Path
import subprocess
import tempfile
import unittest

from test_rg_render import ROOT, compile_host, function


class TextureDmaTests(unittest.TestCase):
    def test_zero_offset_segment_b_is_not_read_as_low_rom_address(self):
        source = (ROOT / "src/port/ultra_shim.c").read_text()
        harness = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#define UNUSED
#define RG_LOGD(...)
#define RG_LOGI(...)
#define RG_LOGE(...)
#define OS_MESG_NOBLOCK 0
typedef int s32;
typedef int OSIoMesg;
typedef int OSMesgQueue;
typedef struct { size_t size; } mk64_rom_t;
static mk64_rom_t rom={0xc00000};
static mk64_rom_t *sPortRom=&rom;
static uint32_t read_offset;
static bool mk64_rom_read(mk64_rom_t *r,uint32_t off,void *dst,size_t size) {
  read_offset=off; return true;
}
static bool esp_ptr_byte_accessible(const void *ptr) { return false; }
static void osSendMesg(OSMesgQueue *queue,void *msg,int flags) {}
'''
        harness += function(source, "static bool texture_rom_offset(")
        harness += function(source, "s32 osPiStartDma(")
        harness += r'''
int main(void) {
  char output[8];
  assert(osPiStartDma(NULL,0,0,0x00000b00,output,8,NULL)==0);
  assert(read_offset==0x7fa3c0);
  assert(osPiStartDma(NULL,0,0,0x0b000000,output,8,NULL)==0);
  assert(read_offset==0x7fa3c0);
  assert(osPiStartDma(NULL,0,0,0xbcb40a0b,output,8,NULL)==0);
  assert(read_offset==0x7e56e4);
  assert(osPiStartDma(NULL,0,0,0x0f001080,output,8,NULL)==0);
  assert(read_offset==0x642ff0);
  assert(osPiStartDma(NULL,0,0,0x123456,output,8,NULL)==0);
  assert(read_offset==0x123456);
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
