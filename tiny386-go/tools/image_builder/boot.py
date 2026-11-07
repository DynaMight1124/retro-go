"""Verified FreeDOS boot assets and C:-specific startup files."""
import hashlib
import json
from pathlib import Path
from .fat16 import dos_path
from .memory import block, profile_bytes, PROFILE, HANDLER

ASSETS=Path(__file__).resolve().parent/'assets'


def verify_assets():
    manifest=json.loads((ASSETS/'manifest.json').read_text(encoding='utf-8'))
    for name,entry in manifest['files'].items():
        data=(ASSETS/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=entry['sha256']:
            raise ValueError(f'Boot asset checksum failed: {name}')


def startup_files(startup,arguments,dos_high=True,memory=None):
    startup=dos_path(startup)
    if any(c in startup for c in '&|<>%^()'):
        raise ValueError('Startup path cannot contain DOS shell operators; rename it in the source folder')
    if not startup.endswith(('.EXE','.COM','.BAT')):
        raise ValueError('Startup must be an EXE, COM or BAT file')
    if any(ord(c)<32 or ord(c)>126 or c in '&|<>%^' for c in arguments):
        raise ValueError('Arguments must be printable ASCII without DOS shell operators or variables')
    command='C:\\'+startup.replace('/','\\')
    if startup.endswith('.BAT'): command='CALL '+command
    if arguments: command+=' '+arguments
    directory=startup.rpartition('/')[0].replace('/','\\')
    lines=['@ECHO OFF','C:', 'CD \\'+directory,command,
           'IF ERRORLEVEL 1 ECHO Game returned a nonzero status.',
           'ECHO Game finished. Type RUN to start again.']
    files={
        'CONFIG.SYS':(['DEVICE=C:\\HIMEMX.EXE','DOS=HIGH,NOUMB'] if dos_high else ['DOS=LOW,NOUMB'])+['FILES=30','BUFFERS=20','LASTDRIVE=C','SHELL=C:\\COMMAND.COM C:\\ /E:1024 /P'],
        'AUTOEXEC.BAT':['@ECHO OFF','SET COMSPEC=C:\\COMMAND.COM','SET PATH=C:\\','CALL C:\\RUN.BAT'],
        'RUN.BAT':lines,
    }
    if memory is not None:
        memory.validate()
        files['CONFIG.SYS']=block(memory.config(dos_high))+files['CONFIG.SYS'][2 if dos_high else 1:]
        files['AUTOEXEC.BAT']=['@ECHO OFF',*block(memory.autoexec(dos_high)),*files['AUTOEXEC.BAT'][1:],*HANDLER]
    result={name:('\r\n'.join(content)+'\r\n').encode('ascii') for name,content in files.items()}
    if memory is not None: result[PROFILE]=profile_bytes(memory,dos_high)
    return result


def install_boot(image,startup,arguments,dos_high=True,memory=None):
    files=startup_files(startup,arguments,dos_high,memory)
    verify_assets()
    boot=(ASSETS/'fat16.bin').read_bytes(); mbr=(ASSETS/'mbr.bin').read_bytes()
    if len(boot)!=512 or len(mbr)!=512 or boot[510:]!=b'\x55\xaa' or mbr[510:]!=b'\x55\xaa':
        raise ValueError('Invalid boot-sector assets')
    image.data[:446]=mbr[:446]
    at=image.boot
    image.data[at:at+3]=boot[:3]
    image.data[at+62:at+512]=boot[62:]
    extra=memory.assets(dos_high) if memory is not None else (['HIMEMX.EXE'] if dos_high else [])
    for name in ['KERNEL.SYS','COMMAND.COM']+extra:
        image.write_file(name,(ASSETS/name).read_bytes())
    for name,data in files.items(): image.write_file(name,data)
