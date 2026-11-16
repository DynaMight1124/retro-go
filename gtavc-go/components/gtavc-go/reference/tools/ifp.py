import sys
import struct
def pad4(n): return (n+3)&~3
def parse_ifp(path):
    d=open(path,'rb').read()
    assert d[:4]==b'ANPK'
    o=8
    assert d[o:o+4]==b'INFO'; sz=struct.unpack('<I',d[o+4:o+8])[0]; n=struct.unpack('<i',d[o+8:o+12])[0]
    o+=8+pad4(sz)
    anims={}
    order=[]
    for _ in range(n):
        assert d[o:o+4]==b'NAME',(o,d[o:o+8])
        sz=struct.unpack('<I',d[o+4:o+8])[0]
        name=d[o+8:o+8+sz].split(b'\0')[0].decode('latin1'); o+=8+pad4(sz)
        assert d[o:o+4]==b'DGAN'; dgsz=struct.unpack('<I',d[o+4:o+8])[0]; end=o+8+dgsz; o+=8
        assert d[o:o+4]==b'INFO'; isz=struct.unpack('<I',d[o+4:o+8])[0]; nobj=struct.unpack('<i',d[o+8:o+12])[0]; o+=8+pad4(isz)
        bones=[]
        for _b in range(nobj):
            assert d[o:o+4]==b'CPAN',(name,o,d[o:o+8]); csz=struct.unpack('<I',d[o+4:o+8])[0]; cend=o+8+csz; o+=8
            assert d[o:o+4]==b'ANIM'; asz=struct.unpack('<I',d[o+4:o+8])[0]
            bname=d[o+8:o+8+28].split(b'\0')[0].decode('latin1')
            nfr,unk,flag,nxt,prv=struct.unpack('<5i',d[o+36:o+56]) if False else (None,)*5
            nfr=struct.unpack('<i',d[o+36:o+40])[0]
            o2=o+8+asz
            ftype=d[o2:o2+4].decode('latin1'); fsz=struct.unpack('<I',d[o2+4:o2+8])[0]
            fo=o2+8
            frames=[]
            stride={'KR00':20,'KRT0':32,'KRTS':44}[ftype]
            for i in range(nfr):
                b=d[fo+i*stride:fo+(i+1)*stride]
                f=struct.unpack('<%df'%(stride//4),b)
                q=f[:4]
                if ftype=='KR00': t=f[4]; tr=None
                else: tr=f[4:7]; t=f[10] if ftype=='KRTS' else f[7]
                frames.append((t,q,tr))
            bones.append((bname,ftype,frames))
            o=cend
        o=end
        anims[name]=bones; order.append(name)
    return anims,order
if __name__=='__main__':
    import sys
    a,order=parse_ifp(sys.argv[1])
    print(len(a),'animations')
    print(', '.join(order))
