"""Validated Tiny386 descriptor settings; unrelated INI text stays intact."""
import configparser
from dataclasses import dataclass
import re

BUTTONS=('up','right','down','left','a','b','start','select')
DEFAULT_KEYS=('up','right','down','left','ctrl','alt','enter','escape')
# Matches t386_keycode() in main/t386_input.c. Shifted punctuation and chords
# cannot be sent by a single controller mapping.
KEYS=('none','up','right','down','left','ctrl','alt','shift','enter','escape',
      'space','tab','backspace','comma',*(f'f{i}' for i in range(1,13)),
      'home','end','pageup','pagedown','insert','delete',
      *'abcdefghijklmnopqrstuvwxyz',*'1234567890',*"-=[];'`\\,./")
RAM_PRESETS=('1M','2M','4M','8M','16M','32M','64M')
VGA_PRESETS=('256K','512K','1024K','2048K')


def size_bytes(value):
    if not isinstance(value,str) or not re.fullmatch(r'\+?[0-9]+[KM]?',value):
        raise ValueError('Use a whole number followed by uppercase M or K (for example 4M)')
    suffix=value[-1]
    return int(value[:-1] if suffix in 'KM' else value)*(1048576 if suffix=='M' else 1024 if suffix=='K' else 1)


@dataclass(frozen=True)
class MachineSettings:
    mem_size: str = '2M'
    vga_mem_size: str = '256K'
    fill_cmos: bool = True
    vga_force_8dm: bool = True
    cpu_gen: int = 3
    controls: tuple[str,...] = DEFAULT_KEYS

    def validate(self):
        if not 1048576<=size_bytes(self.mem_size)<=64*1048576:
            raise ValueError('Guest RAM must be between 1M and 64M; available device PSRAM may limit it further')
        vga=size_bytes(self.vga_mem_size)
        if not 262144<=vga<=2097152 or vga%65536:
            raise ValueError('VGA memory must be 256K–2048K, in 64K steps')
        if type(self.cpu_gen) is not int or self.cpu_gen not in (3,4,5):
            raise ValueError('Choose CPU generation 3 (386), 4 (486) or 5 (Pentium)')
        if type(self.fill_cmos) is not bool or type(self.vga_force_8dm) is not bool:
            raise ValueError('CMOS and 8-dot text settings must be boolean')
        if not isinstance(self.controls,tuple) or len(self.controls)!=len(BUTTONS) or any(k not in KEYS for k in self.controls):
            raise ValueError('Choose one supported keyboard key per controller button')
        return self

    def fields(self):
        self.validate()
        return {'pc':dict(mem_size=self.mem_size,vga_mem_size=self.vga_mem_size,
                          fill_cmos=str(int(self.fill_cmos)),vga_force_8dm=str(int(self.vga_force_8dm))),
                'cpu':dict(gen=str(self.cpu_gen),fpu='0'),
                'controls':dict(zip(BUTTONS,self.controls))}


def _ini_value(raw):
    raw=raw.strip()
    if not raw.startswith('"'):
        # inih trims the value before looking for comments. A leading ';' is
        # therefore a literal key, unlike ConfigParser's inline-comment rule.
        return re.split(r'\s+;',raw,maxsplit=1)[0].rstrip()
    escapes={'a':'\a','b':'\b','f':'\f','n':'\n','r':'\r','t':'\t','v':'\v','0':'\0',
             "'":"'",'"':'"','\\':'\\'}
    result=[]; index=1
    while index<len(raw):
        char=raw[index]; index+=1
        if char=='"':
            tail=raw[index:].lstrip()
            if tail and not tail.startswith(';'): raise ValueError('Unexpected text after quoted INI value')
            return ''.join(result)
        if char=='\\':
            if index==len(raw) or raw[index] not in escapes: raise ValueError('Invalid INI escape')
            char=escapes[raw[index]]; index+=1
        result.append(char)
    raise ValueError('Unterminated quoted INI value')


def read_settings(text):
    parser=configparser.ConfigParser(interpolation=None,empty_lines_in_values=False)
    parser.optionxform=str
    try:
        parser.read_string(text.removeprefix('\ufeff'))
        if parser.defaults(): raise ValueError('INI DEFAULT inheritance is not supported by Tiny386')
        def value(section,key,default): return _ini_value(parser.get(section,key,fallback=default))
        def flag(section,key,default):
            number=size_bytes(value(section,key,default))
            if number not in (0,1): raise ValueError(f'{key} must be 0 or 1')
            return number==1
        if size_bytes(value('cpu','fpu','0'))!=0: raise ValueError('This port only supports fpu = 0')
        return MachineSettings(value('pc','mem_size','2M'),value('pc','vga_mem_size','256K'),
                               flag('pc','fill_cmos','0'),flag('pc','vga_force_8dm','1'),
                               int(value('cpu','gen','3')),
                               tuple(value('controls',button,key).lower() for button,key in zip(BUTTONS,DEFAULT_KEYS))).validate()
    except (configparser.Error,ValueError) as e:
        raise ValueError('Cannot edit companion .ini: '+str(e)) from None


def update_descriptor(text,settings):
    """Replace supported fields without reserializing comments/other sections."""
    read_settings(text)  # Refuse ambiguous/unsupported input before changing it.
    bom='\ufeff' if text.startswith('\ufeff') else ''
    text=text.removeprefix('\ufeff')
    pending={section:dict(fields) for section,fields in settings.fields().items()}
    newline='\r\n' if '\r\n' in text else '\n'
    output=[]; section=None; seen=set()
    def missing():
        if section in pending and pending[section]:
            if output and not output[-1].endswith(('\n','\r')): output[-1]+=newline
            output.extend(f'{key} = {value}{newline}' for key,value in pending[section].items())
            pending[section].clear()
    for line in text.splitlines(keepends=True):
        header=re.match(r'^\s*\[([^]]+)\]',line)
        if header:
            missing(); section=header[1]; seen.add(section)
        match=re.match(r'^(\s*)([^\s=:#;]+)(\s*[=:]\s*)(.*?)(\r?\n)?$',line)
        if match and section in pending and match[2] in pending[section]:
            key=match[2]; tail=match[4]
            comment=re.search(r'\s+;.*$',tail)
            suffix=comment[0] if comment else ''
            line=match[1]+key+match[3]+pending[section].pop(key)+suffix+(match[5] or '')
        output.append(line)
    missing()
    for name,fields in pending.items():
        if name in seen: continue
        if output and not output[-1].endswith(('\n','\r')): output[-1]+=newline
        output.append(newline+'['+name+']'+newline)
        output.extend(f'{key} = {value}{newline}' for key,value in fields.items())
    result=bom+''.join(output)
    if read_settings(result)!=settings: raise ValueError('Descriptor update did not round-trip correctly')
    return result


def descriptor_settings(image):
    path=image.with_suffix('.ini')
    if path.is_symlink() or not path.is_file(): raise ValueError('Keep the matching .ini beside the image to edit machine settings')
    try: text=path.read_bytes().decode('utf-8')
    except (OSError,UnicodeError) as e: raise ValueError('Cannot read companion .ini: '+str(e)) from None
    return read_settings(text),text
