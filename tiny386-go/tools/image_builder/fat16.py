"""Bounded FAT16 implementation for single-partition Tiny386 game disks.

Disk storage is temporary-file-backed, so custom capacities do not require a
matching Python heap allocation. Never opens physical devices.
"""
import mmap
import os
import stat
import re
import shutil
import struct
import tempfile
from pathlib import PurePosixPath

SECTOR = 512
HEADS, SPT, START = 16, 63, 63
ROOT_ENTRIES = 512
VALID = re.compile(r"^[A-Z0-9!#$%&'()@^_`{}~\-]{1,8}(\.[A-Z0-9!#$%&'()@^_`{}~\-]{1,3})?$")


def dos_path(path):
    path = str(path).replace('\\', '/')
    parts = path.split('/')
    if not parts or any(not VALID.fullmatch(p.upper()) for p in parts):
        raise ValueError(f'Use relative DOS 8.3 names: {path}')
    return '/'.join(p.upper() for p in parts)


def geometry(size_mib):
    if type(size_mib) is not int or not 4 <= size_mib <= 2048:
        raise ValueError('Disk size must be an integer from 4 to 2048 MiB')
    total = min(size_mib*2048, 2047*2048) // (HEADS*SPT) * (HEADS*SPT)
    sectors = total-START
    for spc in (1, 2, 4, 8, 16, 32, 64):
        # Fixed point may oscillate by one sector: choose the conservative size.
        fat = ((sectors//spc+2)*2+511)//512
        clusters = (sectors-1-2*fat-ROOT_ENTRIES*32//512)//spc
        if 4085 <= clusters < 65525:
            return dict(total=total, sectors=sectors, spc=spc, fat=fat,
                        clusters=clusters, start=START, heads=HEADS, spt=SPT)
    raise ValueError('Capacity cannot be represented as supported FAT16')


def chs(lba):
    cylinder, rem = divmod(lba, HEADS*SPT)
    head, sector = divmod(rem, SPT)
    if cylinder > 1023: return b'\xfe\xff\xff'
    return bytes((head, (sector+1) | ((cylinder>>2)&0xc0), cylinder&255))


class Fat16Image:
    def __init__(self, file):
        self.file = file
        self.data = mmap.mmap(file.fileno(), 0)

    def __enter__(self): return self
    def __exit__(self, *args): self.close()
    def close(self):
        self.data.close()
        self.file.close()

    @classmethod
    def create(cls, size_mib):
        g = geometry(size_mib)
        f = tempfile.TemporaryFile(); f.truncate(g['total']*512)
        obj = cls(f)
        d = obj.data
        d[446:462] = bytes((0x80,))+chs(START)+bytes((0x06,))+chs(g['total']-1)+struct.pack('<II', START, g['sectors'])
        d[510:512] = b'\x55\xaa'
        b = bytearray(512)
        b[:11] = b'\xeb\x3c\x90T386GO  '
        struct.pack_into('<HBHBHHBHHHII', b, 11, 512, g['spc'], 1, 2,
                         ROOT_ENTRIES, g['sectors'] if g['sectors']<65536 else 0,
                         0xf8, g['fat'], SPT, HEADS, START,
                         g['sectors'] if g['sectors']>=65536 else 0)
        b[36:62] = bytes((0x80,0,0x29))+struct.pack('<I', 0x38660001)+b'TINY386GAME'+b'FAT16   '
        b[510:512] = b'\x55\xaa'; d[START*512:(START+1)*512] = b
        for n in (0,1):
            offset = (START+1+n*g['fat'])*512
            d[offset:offset+4] = b'\xf8\xff\xff\xff'
        obj._parse()
        return obj

    @classmethod
    def open(cls, data):
        f = tempfile.TemporaryFile(); f.write(data); f.flush()
        return cls._checked(f)

    @classmethod
    def open_path(cls, path):
        if not stat.S_ISREG(os.stat(path).st_mode): raise ValueError('Image must be a regular file')
        f = tempfile.TemporaryFile()
        try:
            with open(path, 'rb') as src:
                if not stat.S_ISREG(os.fstat(src.fileno()).st_mode): raise ValueError('Image must be a regular file')
                shutil.copyfileobj(src, f, 1024*1024)
            f.flush()
            return cls._checked(f)
        except BaseException:
            f.close(); raise

    @classmethod
    def _checked(cls, f):
        obj = None
        try:
            if f.seek(0,2)<512: raise ValueError('Truncated image')
            obj = cls(f); obj._parse(); obj._validate()
            return obj
        except BaseException:
            if obj: obj.close()
            else: f.close()
            raise

    def _parse(self):
        d = self.data
        if d[510:512]!=b'\x55\xaa': raise ValueError('Missing MBR signature')
        entry = d[446:462]
        if entry[0]!=0x80 or entry[4]!=6 or any(d[462:510]):
            raise ValueError('Only one active FAT16 partition is supported')
        start, count = struct.unpack_from('<II', entry, 8)
        if start!=START or (start+count)*512!=len(d): raise ValueError('Partition bounds do not match image')
        self.boot = start*512
        b = d[self.boot:self.boot+512]
        ss, self.spc, reserved, fats, roots, small, media, self.fat_sectors, spt, heads, hidden, large = struct.unpack_from('<HBHBHHBHHHII', b, 11)
        if (ss,reserved,fats,roots,media,spt,heads,hidden)!=(512,1,2,ROOT_ENTRIES,0xf8,SPT,HEADS,START) or b[510:512]!=b'\x55\xaa':
            raise ValueError('Unsupported FAT16 disk layout')
        if self.spc not in (1,2,4,8,16,32,64) or (small or large)!=count or (small and large):
            raise ValueError('Invalid BPB capacity')
        self.fat = self.boot+512
        self.root = self.fat+2*self.fat_sectors*512
        self.base = self.root+ROOT_ENTRIES*32
        self.cluster_bytes = self.spc*512
        self.clusters = (len(d)-self.base)//self.cluster_bytes
        if not 4085<=self.clusters<65525 or (self.clusters+2)*2>self.fat_sectors*512:
            raise ValueError('Invalid FAT16 cluster count')
        n = self.fat_sectors*512
        if d[self.fat:self.fat+n]!=d[self.fat+n:self.fat+2*n] or d[self.fat:self.fat+4]!=b'\xf8\xff\xff\xff':
            raise ValueError('Invalid or mismatched FAT copies')

    def _get(self, cluster): return struct.unpack_from('<H', self.data, self.fat+2*cluster)[0]
    def _set(self, cluster, value):
        for offset in (self.fat,self.fat+self.fat_sectors*512):
            struct.pack_into('<H', self.data, offset+2*cluster, value)
    def _offset(self, cluster): return self.base+(cluster-2)*self.cluster_bytes
    def _chain(self, cluster):
        result=[]; seen=set()
        while cluster:
            if cluster in seen or not 2<=cluster<self.clusters+2:
                raise ValueError('Corrupt FAT chain')
            seen.add(cluster); result.append(cluster); cluster=self._get(cluster)
            if cluster>=0xfff8: return result
        if result: raise ValueError('Unterminated FAT chain')
        return result

    def _slots(self, cluster):
        if cluster==0: return range(self.root,self.base,32)
        return [o for c in self._chain(cluster) for o in range(self._offset(c),self._offset(c)+self.cluster_bytes,32)]
    def _entries(self, cluster):
        for offset in self._slots(cluster):
            e=self.data[offset:offset+32]
            if e[0]==0: break
            if e[0]==229: continue
            if e[11]==15: raise ValueError('Long filenames are not supported for writable images')
            if e[11]&8: continue
            base=e[:8].decode('ascii').rstrip(); ext=e[8:11].decode('ascii').rstrip()
            name=base+('.'+ext if ext else '')
            if name in ('.','..'): continue
            dos_path(name)
            yield name,offset,e[11],struct.unpack_from('<H',e,26)[0],struct.unpack_from('<I',e,28)[0]

    def _validate(self):
        used=set(); directories=set()
        def walk(cluster,depth):
            if depth>32 or cluster in directories: raise ValueError('Directory cycle or excessive nesting')
            directories.add(cluster); names=set()
            for name,off,attr,c,size in self._entries(cluster):
                if name in names: raise ValueError('Duplicate DOS filename')
                names.add(name); chain=self._chain(c)
                if used.intersection(chain): raise ValueError('Cross-linked FAT chains')
                used.update(chain)
                if attr&16:
                    if not c: raise ValueError('Invalid directory cluster')
                    walk(c,depth+1)
                elif size>len(chain)*self.cluster_bytes: raise ValueError('File exceeds its cluster chain')
        walk(0,0)
        for c in range(2,self.clusters+2):
            value=self._get(c)
            if value and c not in used and value!=0xfff7: raise ValueError('Lost allocated cluster')

    def list_files(self):
        result=[]
        def walk(cluster,prefix):
            for name,_,attr,c,_ in self._entries(cluster):
                path=prefix+name
                if attr&16: walk(c,path+'/')
                else: result.append(path)
        walk(0,''); return sorted(result)

    def _find(self, path):
        parts=dos_path(path).split('/'); cluster=0
        for i,part in enumerate(parts):
            found=next((e for e in self._entries(cluster) if e[0]==part),None)
            if not found: raise ValueError(f'File not found: {path}')
            if i<len(parts)-1:
                if not found[2]&16: raise ValueError('Parent is not a directory')
                cluster=found[3]
        return found

    def read_file(self,path):
        _,_,attr,c,size=self._find(path)
        if attr&16: raise ValueError('Cannot read directory as file')
        return b''.join(self.data[self._offset(n):self._offset(n)+self.cluster_bytes] for n in self._chain(c))[:size]

    @property
    def free_bytes(self): return sum(self._get(c)==0 for c in range(2,self.clusters+2))*self.cluster_bytes

    def _allocate(self,count):
        selected=[]
        for c in range(2,self.clusters+2):
            if self._get(c)==0:
                selected.append(c)
                if len(selected)==count: break
        if len(selected)!=count: raise ValueError('Disk is full')
        for i,c in enumerate(selected):
            self._set(c, selected[i+1] if i+1<count else 0xffff)
            self.data[self._offset(c):self._offset(c)+self.cluster_bytes]=bytes(self.cluster_bytes)
        return selected

    def _entry_slot(self,parent):
        for off in self._slots(parent):
            if self.data[off] in (0,229): return off
        if parent==0: raise ValueError('Root directory is full')
        chain=self._chain(parent); extra=self._allocate(1)[0]; self._set(chain[-1],extra)
        return self._offset(extra)

    def _entry(self,off,name,attr,cluster,size):
        base,_,ext=name.partition('.')
        e=bytearray(32); e[:11]=(name.ljust(11) if name in ('.','..') else base.ljust(8)+ext.ljust(3)).encode('ascii'); e[11]=attr
        struct.pack_into('<HH',e,22,0,0x21) # 1980-01-01
        struct.pack_into('<HI',e,26,cluster,size); self.data[off:off+32]=e

    def mkdir(self,path):
        parts=dos_path(path).split("/"); parent=0
        for part in parts:
            found=next((e for e in self._entries(parent) if e[0]==part),None)
            if found:
                if not found[2]&16: raise ValueError('Parent is not a directory')
                parent=found[3]; continue
            off=self._entry_slot(parent); c=self._allocate(1)[0]
            self._entry(off,part,16,c,0)
            at=self._offset(c); self._entry(at,'.',16,c,0); self._entry(at+32,'..',16,parent,0)
            parent=c
        return parent

    def write_file(self,path,data):
        parts=dos_path(path).split("/")
        parent=self.mkdir("/".join(parts[:-1])) if len(parts)>1 else 0
        found=next((e for e in self._entries(parent) if e[0]==parts[-1]),None)
        if found and found[2]&16: raise ValueError('Cannot replace a directory')
        count=(len(data)+self.cluster_bytes-1)//self.cluster_bytes
        old=self._chain(found[3]) if found else []
        if count>len(old)+self.free_bytes//self.cluster_bytes: raise ValueError('Disk is full')
        off=found[1] if found else self._entry_slot(parent)
        extra=self._allocate(count-len(old)) if count>len(old) else []
        chain=old[:count]+extra
        for i,c in enumerate(chain): self._set(c,chain[i+1] if i+1<len(chain) else 0xffff)
        for i,c in enumerate(chain):
            chunk=data[i*self.cluster_bytes:(i+1)*self.cluster_bytes]
            at=self._offset(c); self.data[at:at+len(chunk)]=chunk
        self._entry(off,parts[-1],32,chain[0] if chain else 0,len(data))
        for c in old[count:]: self._set(c,0)

    def to_bytes(self): return self.data[:]
    def write_to(self,path):
        self.data.flush(); self.file.seek(0)
        with open(path,'wb') as dst: shutil.copyfileobj(self.file,dst,1024*1024)
