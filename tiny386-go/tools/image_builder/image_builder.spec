from pathlib import Path
base = Path(SPECPATH)
a = Analysis([str(base/'launch_gui.py')], pathex=[str(base.parents[1])],
             binaries=[], datas=[(str(base/'assets'), 'tools/image_builder/assets'),
                                 (str(base/'licenses'), 'tools/image_builder/licenses')],
             hiddenimports=[], hookspath=[], runtime_hooks=[], excludes=['pyfatfs'], noarchive=False)
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, a.binaries, a.datas, [], name='Tiny386-Image-Builder',
          debug=False, strip=False, upx=False, console=False)
