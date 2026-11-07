"""Explicit DOS memory limits and conservative updates of managed startup blocks."""
from dataclasses import asdict, dataclass
import json
import re

SIZES=(128,256,512,768,1024)
BEGIN='REM T386 MEMORY BEGIN'
END='REM T386 MEMORY END'
PROFILE='T386MEM.CFG'
HANDLER=['GOTO T386MEMEND',':T386MEMFAIL',
         'ECHO Memory settings failed. Reboot or edit this image.',':T386MEMEND']


@dataclass(frozen=True)
class MemorySettings:
    xms: bool = True
    xms_kib: int = 1024
    ems: bool = False
    ems_kib: int = 1024

    def validate(self):
        if type(self.xms) is not bool or type(self.ems) is not bool:
            raise ValueError('Memory enable settings must be boolean')
        for size in (self.xms_kib,self.ems_kib):
            if type(size) is not int or size not in SIZES:
                raise ValueError('Choose 128, 256, 512, 768 or 1024 KiB')
        return self

    def assets(self,dos_high):
        self.validate()
        return (['HIMEMX.EXE','XMSCAP.COM'] if self.xms or self.ems or dos_high else [])+(['JEMM386.EXE'] if self.ems else [])

    def config(self,dos_high):
        lines=[]
        if self.xms or self.ems or dos_high: lines.append('DEVICE=C:\\HIMEMX.EXE')
        if self.ems:
            # Bundled VGA ROM ends below D0000; the 128KiB BIOS starts at E0000.
            # SeaBIOS reserves C0000-FFFFF as one E820 range, hiding this free frame.
            lines.append(f'DEVICE=C:\\JEMM386.EXE MIN={self.ems_kib} MAX={self.ems_kib} I=D000-DFFF FRAME=D000 NODYN NOVCPI NORAM')
        lines.append('DOS='+('HIGH' if dos_high else 'LOW')+',NOUMB')
        return lines

    def autoexec(self,dos_high):
        if not (self.xms or self.ems or dos_high): return []
        return [f'C:\\XMSCAP.COM {self.xms_kib if self.xms else 0}'+(' EMS' if self.ems else ''),
                'IF ERRORLEVEL 1 GOTO T386MEMFAIL']


def block(lines):
    return [BEGIN,*lines,END]


def profile_bytes(memory,dos_high):
    return (json.dumps(dict(version=1,dos_high=dos_high,**asdict(memory)),indent=2)+'\n').encode('ascii')


def _lines(data):
    try: text=data.decode('ascii')
    except UnicodeDecodeError: raise ValueError('Startup file is not supported ASCII text') from None
    if '\x00' in text: raise ValueError('Startup file contains binary data')
    return text.splitlines()


def _bounds(lines):
    starts=[i for i,l in enumerate(lines) if l==BEGIN]
    ends=[i for i,l in enumerate(lines) if l==END]
    if not starts and not ends: return None
    if len(starts)!=1 or len(ends)!=1 or starts[0]>=ends[0]:
        raise ValueError('Memory settings block was modified; restore its backup before editing')
    return starts[0],ends[0]


def inspect_memory(disk):
    files=disk.list_files(); config=_lines(disk.read_file('CONFIG.SYS')); auto=_lines(disk.read_file('AUTOEXEC.BAT'))
    bounds=_bounds(config); abounds=_bounds(auto)
    if PROFILE in files:
        try:
            p=json.loads(disk.read_file(PROFILE))
            if not isinstance(p,dict): raise ValueError()
            high=p.pop('dos_high'); version=p.pop('version')
            if type(high) is not bool or type(version) is not int or version!=1: raise ValueError()
            memory=MemorySettings(**p).validate()
        except (ValueError,TypeError,KeyError): raise ValueError('Invalid memory profile inside image') from None
        if not bounds or not abounds: raise ValueError('Memory profile has missing startup blocks')
        if config[bounds[0]+1:bounds[1]]!=memory.config(high) or auto[abounds[0]+1:abounds[1]]!=memory.autoexec(high):
            raise ValueError('Memory startup commands were modified; restore or reconcile them before editing')
        labels=[l.strip().upper() for l in auto if l.strip().upper() in (':T386MEMFAIL',':T386MEMEND')]
        tail=list(auto)
        while tail and (not tail[-1].strip() or re.match(r'\s*REM(?:\s|$)',tail[-1],re.I)): tail.pop()
        if labels!=[':T386MEMFAIL',':T386MEMEND'] or tail[-len(HANDLER):]!=HANDLER:
            raise ValueError('Memory failure handler was modified; restore its backup before editing')
    else:
        if bounds or abounds: raise ValueError('Memory blocks have no matching profile')
        if any(l.strip().upper() in (':T386MEMFAIL',':T386MEMEND') for l in auto):
            raise ValueError('Custom startup labels conflict with memory settings')
        # Upgrade only our known legacy directives; never replace third-party managers.
        high='DOS=HIGH,NOUMB' in config
        expected=['DEVICE=C:\\HIMEMX.EXE','DOS=HIGH,NOUMB'] if high else ['DOS=LOW,NOUMB']
        if any(config.count(l)!=1 for l in expected): raise ValueError('Unrecognised legacy memory setup')
        memory=MemorySettings(xms=high)
    outside=config if not bounds else config[:bounds[0]]+config[bounds[1]+1:]
    for line in outside:
        if re.match(r'\s*REM(?:\s|$)',line,re.I): continue
        if not bounds and line in expected: continue
        if re.match(r'\s*DOS\s*=',line,re.I) or re.search(r'(HIMEM|JEMM|EMM386|QEMM|XMGR)',line,re.I):
            raise ValueError('Additional memory-manager commands need manual review')
    outside_auto=auto if not abounds else auto[:abounds[0]]+auto[abounds[1]+1:]
    for line in outside_auto:
        if re.match(r'\s*REM(?:\s|$)',line,re.I): continue
        if re.search(r'(HIMEM|JEMM|EMM386|QEMM|XMGR|XMSCAP)',line,re.I):
            raise ValueError('Additional AUTOEXEC memory-manager commands need manual review')
    return memory,high,bounds,abounds


def update_memory(disk,memory,dos_high,assets):
    memory.validate()
    if type(dos_high) is not bool: raise ValueError('DOS high setting must be boolean')
    old,old_high,bounds,abounds=inspect_memory(disk)
    config=_lines(disk.read_file('CONFIG.SYS')); auto=_lines(disk.read_file('AUTOEXEC.BAT'))
    if bounds:
        config=config[:bounds[0]]+block(memory.config(dos_high))+config[bounds[1]+1:]
        auto=auto[:abounds[0]]+block(memory.autoexec(dos_high))+auto[abounds[1]+1:]
    else:
        known=['DEVICE=C:\\HIMEMX.EXE','DOS=HIGH,NOUMB','DOS=LOW,NOUMB']
        config=block(memory.config(dos_high))+[l for l in config if l not in known]
        # Run limiter before any auto-started game, while preserving custom commands.
        auto=block(memory.autoexec(dos_high))+auto
    # Failure must land at the DOS prompt rather than start a game with wrong limits.
    if not bounds: auto+=HANDLER
    for name in memory.assets(dos_high): disk.write_file(name,(assets/name).read_bytes())
    disk.write_file('CONFIG.SYS',('\r\n'.join(config)+'\r\n').encode('ascii'))
    disk.write_file('AUTOEXEC.BAT',('\r\n'.join(auto)+'\r\n').encode('ascii'))
    disk.write_file(PROFILE,profile_bytes(memory,dos_high))
