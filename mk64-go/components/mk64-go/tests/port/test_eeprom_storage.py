"""Exercise save migration, recovery and bounds on real temporary files."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host

class EepromStorageTests(unittest.TestCase):
    def test_rom_path_migration_backup_and_invalid_requests(self):
        source=(ROOT/"src/port/ultra_shim.c").read_text()
        start=source.index('#ifdef TARGET_PSP\n#define EEPROM_FILE')
        end=source.index('/* Controller Pak (ghost data)',start)
        source=source[start:end]
        prefix=r"""
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef uint8_t u8;typedef int32_t s32;typedef int OSMesgQueue;
#define UNUSED
#define EEPROM_TYPE_4K 1
#define RG_BASE_PATH_SAVES "."
#define RG_PATH_SAVE_SRAM 1
#define RG_LOGE(...) ((void)0)
#define RG_LOGW(...) ((void)0)
#define PORT_LOG(...) ((void)0)
#define RG_PANIC(x) abort()
#define RETRO_GO 1
static struct {const char *romPath;} app={"test-rom.z64"};
static unsigned commits;
static void rg_storage_commit(void) {++commits;}
static void *rg_system_get_app(void) {return &app;}
/* Actual app field access needs a concrete return type. */
"""
        prefix=prefix.replace('static struct {const char *romPath;} app=', 'typedef struct {const char *romPath;} App;static App app=').replace('static void *rg_system_get_app','static App *rg_system_get_app')
        prefix+=r"""
static const char *rg_dirname(const char *path) {(void)path;return ".";}
static bool rg_storage_exists(const char *path) {FILE *f=fopen(path,"rb");if(!f)return false;fclose(f);return true;}
static bool rg_storage_mkdir(const char *path) {(void)path;return true;}
static char *rg_emu_get_path(int kind,const char *rom) {
 assert(kind==RG_PATH_SAVE_SRAM);assert(!strcmp(rom,"test-rom.z64"));
 return _strdup("test-rom.z64.sram");
}
"""
        harness=prefix+source+r"""
static void write_bytes(const char *name,unsigned count,u8 byte) {
 FILE *f=fopen(name,"wb");assert(f);for(unsigned i=0;i<count;++i)fputc(byte,f);assert(!fclose(f));
}
int main(void) {
 u8 buf[512]={0};
 assert(osEepromLongRead(NULL,0,buf,512)==0);
 for(int i=0;i<512;++i)assert(buf[i]==0x37);
 assert(!strcmp(EEPROM_FILE,"test-rom.z64.sram"));
 FILE *f=fopen(EEPROM_FILE,"rb");assert(f);fclose(f); /* migrated */
 buf[0]=0xA5;assert(osEepromLongWrite(NULL,0,buf,8)==0);
 assert(commits>0);
 u8 first=sEeprom[0];
 assert(osEepromLongWrite(NULL,0,buf,-1)==-1 && sEeprom[0]==first);
 assert(osEepromLongWrite(NULL,64,buf,8)==-1);
 assert(osEepromLongRead(NULL,0,NULL,8)==-1);
 assert(osEepromLongRead(NULL,63,buf,9)==-1);
 write_bytes(EEPROM_FILE,17,0);sEepromLoaded=0;
 assert(osEepromLongRead(NULL,0,buf,8)==0 && buf[0]==0xA5); /* recover */
 write_bytes(EEPROM_FILE,513,0);
 assert(!eeprom_read_file(EEPROM_FILE)); /* reject oversized images too */
 write_bytes(EEPROM_BACKUP_FILE,513,0);sEepromLoaded=0;
 assert(osEepromLongRead(NULL,0,buf,8)==0 && buf[0]==0); /* no stale legacy rollback */
 char *primary=sEepromFile,*backup=sEepromBackupFile;
 sEepromFile="missing/primary";sEepromBackupFile="missing/backup";
 buf[0]=0x55;assert(osEepromLongWrite(NULL,0,buf,8)==-1);
 assert(!mk64_eeprom_flush() && sEepromDirty); /* failed write remains retryable */
 sEepromFile=primary;sEepromBackupFile=backup;
 assert(mk64_eeprom_flush() && !sEepromDirty);
 sEepromLoaded=0;assert(osEepromLongRead(NULL,0,buf,8)==0 && buf[0]==0x55);
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory);(directory/"mk64").mkdir()
            (directory/"mk64/eeprom.bin").write_bytes(bytes([0x37])*512)
            exe=compile_host(harness,directory)
            result=subprocess.run([str(exe)],cwd=directory,capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual((directory/"mk64/eeprom.bin").read_bytes(),bytes([0x37])*512)

if __name__=="__main__":unittest.main()
