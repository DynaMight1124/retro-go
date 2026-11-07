"""Game validation and recoverable publication, independent of the GUI."""
from dataclasses import asdict, dataclass
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import tempfile
from .fat16 import Fat16Image, dos_path, geometry
from .boot import ASSETS, install_boot, startup_files, verify_assets
from .memory import MemorySettings, PROFILE, inspect_memory, update_memory
from .machine import MachineSettings, descriptor_settings, update_descriptor

BOOT_REQUIRED={'KERNEL.SYS','COMMAND.COM','CONFIG.SYS','AUTOEXEC.BAT','RUN.BAT'}
RESERVED=BOOT_REQUIRED|{'HIMEMX.EXE','JEMM386.EXE','XMSCAP.COM',PROFILE}


@dataclass(frozen=True)
class BuildRequest:
    game_dir: Path
    startup: str
    arguments: str
    output_dir: Path
    name: str
    size_mib: int
    dos_high: bool = True
    memory: MemorySettings | None = None
    settings: MachineSettings = MachineSettings()


def validate_relative_names(names):
    seen=set()
    for name in names:
        normalized=dos_path(name)
        if normalized in seen: raise ValueError(f'DOS filename collision: {name}')
        seen.add(normalized)


def inspect_game(request):
    if not isinstance(request.settings,MachineSettings): raise ValueError('Invalid machine settings')
    request.settings.validate()
    if type(request.dos_high) is not bool: raise ValueError('DOS high setting must be a boolean')
    if request.memory is not None:
        if not isinstance(request.memory,MemorySettings): raise ValueError('Invalid memory profile')
        request.memory.validate()
    geometry(request.size_mib)
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}',request.name):
        raise ValueError('Image name must use 1–64 letters, digits, underscores or hyphens')
    root=Path(request.game_dir)
    if root.is_symlink() or not root.is_dir(): raise ValueError('Select a real game directory')
    root=root.resolve(); files=[]; names=[]; directories=[]
    for p in sorted(root.rglob('*')):
        if p.is_symlink() or not p.resolve().is_relative_to(root): raise ValueError(f'Symlinks are not supported: {p}')
        relative=p.relative_to(root).as_posix(); names.append(relative)
        if p.is_file(): files.append((dos_path(relative),p))
        elif p.is_dir(): directories.append(dos_path(relative))
        else: raise ValueError(f'Not a regular file: {p}')
    validate_relative_names(names)
    candidates=sorted(n for n,_ in files if n.endswith(('.EXE','.COM','.BAT')))
    if request.startup:
        selected=dos_path(request.startup)
        if selected not in candidates: raise ValueError('Select a startup executable present in the game folder')
        startup_files('GAME/'+selected,request.arguments)
    elif not candidates: raise ValueError('No EXE, COM or BAT startup files found')
    g=geometry(request.size_mib); cluster=g['spc']*512
    required=sum(((p.stat().st_size+cluster-1)//cluster)*cluster for _,p in files)
    # Include a conservative directory-entry allowance, boot assets and replacement slack.
    boot_names=['KERNEL.SYS','COMMAND.COM']+(request.memory.assets(request.dos_high) if request.memory is not None else (['HIMEMX.EXE'] if request.dos_high else []))
    required+=(len(names)+16)*cluster+sum((ASSETS/n).stat().st_size for n in boot_names)
    available=g['clusters']*cluster
    if required>available: raise ValueError(f'Game needs approximately {required:,} bytes; choose a larger disk')
    return dict(files=files,directories=directories,candidates=candidates,required_bytes=required,
                actual_bytes=g['total']*512,free_estimate=available-required)


def _manifest(request,files):
    boot=json.loads((ASSETS/'manifest.json').read_text(encoding='utf-8'))
    return dict(format='tiny386-game-image-v1',size_mib=request.size_mib,
                actual_bytes=geometry(request.size_mib)['total']*512,
                startup='GAME/'+dos_path(request.startup),arguments=request.arguments,
                dos_high=request.dos_high,himemx=boot['himemx'] if (request.memory.assets(request.dos_high) if request.memory is not None else request.dos_high) else None,
                memory=asdict(request.memory) if request.memory is not None else None,
                machine=asdict(request.settings),
                jemm=boot.get('jemm') if request.memory is not None and request.memory.ems else None,
                files={p.relative_to(request.game_dir.resolve()).as_posix():'GAME/'+n for n,p in files},
                boot_assets={k:v['sha256'] for k,v in boot['files'].items()})


def _publish(staged,overwrite=False,backup=False):
    """Rollback synchronous publication failures; retain recovery files on process death.

    This is not a multi-file atomic filesystem transaction. A recovery directory
    containing prior outputs remains if the process is killed between renames.
    """
    if not overwrite and any(dst.exists() for _,dst in staged): raise ValueError('Output already exists; choose overwrite or another name')
    if any(dst.is_symlink() or (dst.exists() and not dst.is_file()) for _,dst in staged): raise ValueError('Output must be a regular file')
    recovery=Path(tempfile.mkdtemp(prefix='.tiny386-recovery-',dir=staged[0][1].parent))
    prior={}; published=[]; recovered=False
    try:
        for _,dst in staged:
            if dst.exists():
                saved=recovery/dst.name; shutil.copy2(dst,saved); prior[dst]=saved
                if backup:
                    backup_path=dst.with_suffix(dst.suffix+'.bak')
                    if backup_path.is_symlink(): raise ValueError('Backup path must not be a symlink')
                    shutil.copy2(dst,backup_path)
        for src,dst in staged:
            os.replace(src,dst); published.append(dst)
        recovered=True
    except BaseException:
        # copyfile avoids relying on the failed replace operation during recovery.
        for dst in published:
            if dst in prior: shutil.copy2(prior[dst],dst)
            else: dst.unlink(missing_ok=True)
        recovered=True
        raise
    finally:
        # If rollback itself fails, the exception skips here only after entering
        # finally: keep recovery material unless all outputs are restored.
        if recovered: shutil.rmtree(recovery)


def create_game(request,progress=lambda message:None,*,overwrite=False):
    if not request.startup: raise ValueError('Choose a startup executable')
    info=inspect_game(request); verify_assets()
    out=Path(request.output_dir)
    if not out.is_dir(): raise ValueError('Select an existing output folder')
    if out.resolve().is_relative_to(request.game_dir.resolve()):
        raise ValueError('Choose an output folder outside the source game folder')
    destinations={key:out/(request.name+ext) for key,ext in [('image','.img'),('descriptor','.ini'),('manifest','.json')]}
    if not overwrite and any(p.exists() for p in destinations.values()): raise ValueError('Output already exists; choose overwrite or another name')
    progress('Creating FAT16 disk…')
    with tempfile.TemporaryDirectory(prefix='.tiny386-stage-',dir=out) as tmp:
        tmp=Path(tmp)
        with Fat16Image.create(request.size_mib) as disk:
            install_boot(disk,'GAME/'+dos_path(request.startup),request.arguments,request.dos_high,request.memory)
            for directory in info['directories']: disk.mkdir('GAME/'+directory)
            for n,p in info['files']:
                progress('Copying '+n); disk.write_file('GAME/'+n,p.read_bytes())
            disk._validate(); disk.write_to(tmp/'image')
        descriptor=update_descriptor('[pc]\nhda = '+destinations['image'].name+'\n',request.settings)
        (tmp/'descriptor').write_text(descriptor,encoding='ascii')
        (tmp/'manifest').write_text(json.dumps(_manifest(request,info['files']),indent=2)+'\n',encoding='utf-8')
        progress('Validating and saving outputs…')
        _publish([(tmp/k,v) for k,v in destinations.items()],overwrite,backup=overwrite)
    return destinations


def inspect_image(path):
    path=Path(path)
    settings=None; settings_error=''
    try: settings,_=descriptor_settings(path)
    except ValueError as e: settings_error=str(e)
    with Fat16Image.open_path(path) as disk:
        files=disk.list_files()
        owned=all(n in files for n in BOOT_REQUIRED) and b'Tiny386' in disk.data[:446]
        memory=None; high=None; memory_error=''
        if owned:
            try: memory,high,_,_=inspect_memory(disk)
            except ValueError as e: memory_error=str(e)
        # MBR currently carries the identifying message; no companion manifest is required.
        return dict(files=files,free_bytes=disk.free_bytes,actual_bytes=len(disk.data),writable=owned,
                    memory=memory,dos_high=high,memory_error=memory_error,memory_editable=owned and not memory_error,
                    memory_legacy=owned and PROFILE not in files,
                    settings=settings,settings_error=settings_error,settings_editable=owned and not settings_error,
                    startup_text=disk.read_file('RUN.BAT').decode('ascii',errors='replace') if 'RUN.BAT' in files else '')


def extract_file(image,dos_name,destination):
    destination=Path(destination)
    if destination.is_symlink(): raise ValueError('Extraction destination must not be a symlink')
    with Fat16Image.open_path(image) as disk: data=disk.read_file(dos_name)
    destination.write_bytes(data)


def update_image(image,additions,startup=None,arguments='',progress=lambda message:None,*,memory=None,dos_high=None,settings=None):
    image=Path(image)
    if not inspect_image(image)['writable']: raise ValueError('Only tool-created images can be updated')
    descriptor=None
    if settings is not None:
        if not isinstance(settings,MachineSettings): raise ValueError('Invalid machine settings')
        _,text=descriptor_settings(image)
        descriptor=update_descriptor(text,settings)
    if memory is not None:
        if not isinstance(memory,MemorySettings): raise ValueError('Invalid memory profile')
        memory.validate(); verify_assets()
    elif dos_high is not None: raise ValueError('DOS high edits require memory settings')
    validate_relative_names(additions)
    for name,path in additions.items():
        if dos_path(name) in RESERVED: raise ValueError('Use startup settings to change system files')
        path=Path(path)
        if path.is_symlink() or not path.is_file(): raise ValueError('Select regular source files')
    with tempfile.TemporaryDirectory(prefix='.tiny386-stage-',dir=image.parent) as tmp:
        staged=Path(tmp)/image.name
        to_publish=[]
        if settings is None or additions or startup is not None or memory is not None:
            with Fat16Image.open_path(image) as disk:
                if memory is not None:
                    if dos_high is None: _,dos_high,_,_=inspect_memory(disk)
                    update_memory(disk,memory,dos_high,ASSETS)
                for name,path in additions.items():
                    progress('Updating '+name); disk.write_file(name,Path(path).read_bytes())
                if startup is not None:
                    selected=dos_path(startup)
                    if selected not in disk.list_files(): raise ValueError('Startup file is not present in the image')
                    disk.write_file('RUN.BAT',startup_files(selected,arguments)['RUN.BAT'])
                disk._validate(); disk.write_to(staged)
            to_publish.append((staged,image))
        if descriptor is not None:
            target=Path(tmp)/'descriptor'; target.write_bytes(descriptor.encode('utf-8'))
            to_publish.append((target,image.with_suffix('.ini')))
        metadata=image.with_suffix('.json')
        if metadata.is_file():
            content=json.loads(metadata.read_text(encoding='utf-8'))
            if content.get('format')=='tiny386-game-image-v1':
                if startup is not None: content.update(startup=selected,arguments=arguments)
                if settings is not None: content['machine']=asdict(settings)
                if memory is not None:
                    boot=json.loads((ASSETS/'manifest.json').read_text(encoding='utf-8'))
                    content.update(memory=asdict(memory),dos_high=dos_high,
                                   himemx=boot['himemx'] if memory.assets(dos_high) else None,
                                   jemm=boot['jemm'] if memory.ems else None,boot_assets={k:v['sha256'] for k,v in boot['files'].items()})
                content.setdefault('updates',[]).extend(dos_path(n) for n in additions)
                target=Path(tmp)/metadata.name; target.write_text(json.dumps(content,indent=2)+'\n',encoding='utf-8')
                to_publish.append((target,metadata))
        _publish(to_publish,overwrite=True,backup=True)
