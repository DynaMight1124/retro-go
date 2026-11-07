"""Optional packaged-runtime smoke test, invoked explicitly by developers."""
import json
from dataclasses import replace
from pathlib import Path
import tempfile
import tkinter as tk
from .boot import verify_assets
from .gui import App
from .service import BuildRequest, create_game, update_image, inspect_image
from .memory import MemorySettings
from .machine import MachineSettings
from .fat16 import Fat16Image


def check_runtime(report):
    report=Path(report)
    if report.exists(): raise ValueError('Runtime check report already exists')
    verify_assets()
    root=tk.Tk(); root.withdraw(); app=App(root); root.update()
    try:
        assert app.vars['size'].get()=='4'
        assert app.vars['dos_high'].get() is True
        dialog=app.machine_settings()
        assert 'fpu' not in dialog.vars
        tooltip=dialog.fields['mem_size']._tooltip
        tooltip.show(); root.update_idletasks(); assert tooltip.window is not None
        tooltip.hide(); dialog.close()
        with tempfile.TemporaryDirectory(prefix='tiny386-check-') as tmp:
            folder=Path(tmp); game=folder/'game'; game.mkdir()
            (game/'CHECK.COM').write_bytes(b'\xb8\x00\x4c\xcd\x21')
            machine=replace(MachineSettings(),mem_size='4M',controls=('up','right','down','left','space','alt','enter','escape'))
            outputs=create_game(BuildRequest(game,'CHECK.COM','',folder,'Check',4,memory=MemorySettings(True,256,True,512),settings=machine))
            assert inspect_image(outputs['image'])['settings']==machine
            before=outputs['image'].read_bytes()
            update_image(outputs['image'],{},settings=replace(machine,cpu_gen=4))
            assert inspect_image(outputs['image'])['settings'].cpu_gen==4
            assert outputs['image'].read_bytes()==before
            assert 'fpu = 0' in outputs['descriptor'].read_text(encoding='ascii')
            with Fat16Image.open_path(outputs['image']) as disk:
                assert b'DOS=HIGH' in disk.read_file('CONFIG.SYS')
                assert len(disk.read_file('HIMEMX.EXE'))>1000
                assert len(disk.read_file('JEMM386.EXE'))>1000
                assert b'XMSCAP.COM 256 EMS' in disk.read_file('AUTOEXEC.BAT')
                assert disk.read_file('GAME/CHECK.COM')==b'\xb8\x00\x4c\xcd\x21'
            update_image(outputs['image'],{},memory=MemorySettings(False,256,False,512),dos_high=False)
            assert inspect_image(outputs['image'])['memory']==MemorySettings(False,256,False,512)
        report.write_text(json.dumps({'tk':tk.TkVersion,'assets':'verified','image_roundtrip':'passed','gui':'constructed',
                                     'machine_settings':'passed','hover_help':'passed'})+'\n',encoding='utf-8')
    finally: app.close()
