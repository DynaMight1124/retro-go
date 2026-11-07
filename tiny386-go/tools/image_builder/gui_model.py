"""Headless form validation and worker events."""
from pathlib import Path
from queue import Queue
from threading import Thread
from .service import BuildRequest
from .fat16 import geometry
from .memory import MemorySettings
from .machine import MachineSettings, BUTTONS


def parse_size(preset,custom):
    try: size=int(custom if preset=='Custom' else preset)
    except ValueError: raise ValueError('Enter a whole-number disk size in MiB') from None
    geometry(size)
    return size


def request_from_form(values):
    for key in ('game_dir','output_dir','name'):
        if not values[key].strip(): raise ValueError(f'Please fill in {key.replace("_"," ")}')
    memory=memory_from_form(values) if 'xms' in values else None
    settings=machine_from_form(values) if 'mem_size' in values else MachineSettings()
    return BuildRequest(Path(values['game_dir']),values['startup'],values['arguments'],
                        Path(values['output_dir']),values['name'],parse_size(values['size'],values['custom']),values.get('dos_high',True),memory,settings)


def machine_from_form(values):
    try: gen=int(values['cpu_gen'])
    except (ValueError,TypeError,KeyError): raise ValueError('Choose CPU generation 3, 4 or 5') from None
    return MachineSettings(values['mem_size'].strip(),values['vga_mem_size'].strip(),
                           values['fill_cmos'],values['vga_force_8dm'],gen,
                           tuple(values['key_'+button] for button in BUTTONS)).validate()


def memory_from_form(values):
    try: memory=MemorySettings(values['xms'],int(values['xms_kib']),values['ems'],int(values['ems_kib']))
    except (ValueError,KeyError,TypeError): raise ValueError('Choose a valid memory size in KiB') from None
    return memory.validate()


class Worker:
    def __init__(self): self.events=Queue(); self.thread=None
    @property
    def busy(self): return self.thread is not None and self.thread.is_alive()
    def start(self,action):
        if self.busy: raise ValueError('An operation is already running')
        def run():
            try: self.events.put(('done',action(lambda msg:self.events.put(('progress',msg)))))
            except Exception as e: self.events.put(('error',str(e)))
        self.thread=Thread(target=run,daemon=False); self.thread.start()
