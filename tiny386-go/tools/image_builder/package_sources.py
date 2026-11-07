"""Package only release deliverables, with corresponding source and notices."""
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

SOURCE_FILES=(
    '__init__.py','__main__.py','launch_gui.py','gui.py','gui_model.py',
    'settings_ui.py','service.py','fat16.py','boot.py','memory.py','machine.py',
    'diagnostics.py','package_sources.py','build_windows.ps1',
    'image_builder.spec','requirements-package.txt','README.md','DEVELOPMENT.md',
)
LICENSE_FILES=('Python-LICENSE.txt','Python-third-party.html','TclTk-LICENSE.txt','PyInstaller-COPYING.txt')
RELEASE_FILES=('Tiny386-Image-Builder.exe','README.md','COPYING','image-builder-source.zip')


def create_release(base,dist):
    base=Path(base);dist=Path(dist)
    exe=dist/RELEASE_FILES[0]
    if not exe.is_file():raise FileNotFoundError('Build the Windows executable before packaging')
    copying=base.parents[2]/'COPYING'
    # Include pinned boot/source/licence inputs; ignore stray local assets or caches.
    manifest=json.loads((base/'assets/manifest.json').read_text(encoding='utf-8'))
    assets=sorted(set(manifest['files'])|{'manifest.json','README.md','LICENSE.MBR.txt'})
    shutil.copy2(base/'README.md',dist/'README.md')
    shutil.copy2(copying,dist/'COPYING')
    with zipfile.ZipFile(dist/'image-builder-source.zip','w',zipfile.ZIP_DEFLATED) as z:
        z.write(copying,'COPYING')
        for name in SOURCE_FILES:z.write(base/name,'tools/image_builder/'+name)
        for name in assets:z.write(base/'assets'/name,'tools/image_builder/assets/'+name)
        for name in LICENSE_FILES:z.write(base/'licenses'/name,'tools/image_builder/licenses/'+name)
    release_files=(*RELEASE_FILES,*(f'licenses/{name}' for name in LICENSE_FILES))
    for name in LICENSE_FILES:
        target=dist/'licenses'/name;target.parent.mkdir(exist_ok=True)
        shutil.copy2(base/'licenses'/name,target)
    checksums=''.join(hashlib.sha256((dist/name).read_bytes()).hexdigest()+'  '+name+'\n' for name in release_files)
    (dist/'SHA256SUMS.txt').write_text(checksums,encoding='ascii')
    release=dist/'Tiny386-Image-Builder-Windows.zip'
    with zipfile.ZipFile(release,'w',zipfile.ZIP_DEFLATED) as z:
        for name in (*release_files,'SHA256SUMS.txt'):z.write(dist/name,name)
    return release


def main():
    base=Path(__file__).resolve().parent
    print(create_release(base,base/'dist'))


if __name__=='__main__':main()
