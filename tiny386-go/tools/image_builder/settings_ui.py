"""Machine settings dialog and lightweight, delayed hover help."""
import tkinter as tk
from tkinter import ttk, messagebox
import weakref
from .machine import MachineSettings, BUTTONS, KEYS, RAM_PRESETS, VGA_PRESETS
from .gui_model import machine_from_form


class Tooltip:
    def __init__(self,widget,text):
        # A widget owns its tooltip. Avoid the reverse strong reference, which
        # could defer Tk cleanup until cyclic GC runs on an image worker thread.
        self.widget=weakref.proxy(widget); self.text=text; self.timer=None; self.window=None
        widget.bind('<Enter>',self.schedule,add='+')
        for event in ('<Leave>','<ButtonPress>','<Destroy>'):
            widget.bind(event,self.hide,add='+')

    def schedule(self,event=None):
        self.hide(); self.timer=self.widget.after(550,self.show)

    def show(self):
        self.timer=None
        if not self.widget.winfo_exists(): return
        tip=tk.Toplevel(self.widget); self.window=tip
        tip.overrideredirect(True)
        ttk.Label(tip,text=self.text,wraplength=410,padding=9,relief='solid',borderwidth=1).pack()
        tip.update_idletasks()
        x=min(self.widget.winfo_rootx()+12,tip.winfo_screenwidth()-tip.winfo_reqwidth()-8)
        y=self.widget.winfo_rooty()+self.widget.winfo_height()+6
        if y+tip.winfo_reqheight()>tip.winfo_screenheight():
            y=self.widget.winfo_rooty()-tip.winfo_reqheight()-6
        tip.geometry(f'+{max(0,x)}+{max(0,y)}')

    def hide(self,event=None):
        if self.timer is not None:
            self.widget.after_cancel(self.timer); self.timer=None
        if self.window is not None:
            self.window.destroy(); self.window=None


def help_for(widget,text):
    widget._tooltip=Tooltip(widget,text)
    return widget


def set_machine_vars(values,settings):
    for key in ('mem_size','vga_mem_size','fill_cmos','vga_force_8dm','cpu_gen'):
        values[key].set(getattr(settings,key))
    for button,key in zip(BUTTONS,settings.controls): values['key_'+button].set(key)


def add_machine_vars(parent,values,settings=MachineSettings()):
    for key in ('mem_size','vga_mem_size','cpu_gen'):
        values[key]=tk.StringVar(parent)
    for key in ('fill_cmos','vga_force_8dm'): values[key]=tk.BooleanVar(parent)
    for button in BUTTONS: values['key_'+button]=tk.StringVar(parent)
    set_machine_vars(values,settings)


class MachineDialog:
    def __init__(self,parent,settings,on_apply,editing=False):
        self.window=tk.Toplevel(parent); self.window.title('Machine settings')
        self.window.transient(parent); self.window.resizable(False,False)
        self.vars={}; self.fields={}; self.on_apply=on_apply
        add_machine_vars(self.window,self.vars,settings)
        outer=ttk.Frame(self.window,padding=16); outer.pack(fill='both',expand=True)
        ttk.Label(outer,text='Hover over a setting for help. Defaults suit most early DOS games.').pack(anchor='w',pady=(0,10))
        machine=ttk.LabelFrame(outer,text='Emulated PC',padding=10); machine.pack(fill='x')
        for row,(key,title,choices,help_text) in enumerate([
            ('mem_size','Guest RAM',RAM_PRESETS,
             'Total emulated PC RAM, separate from disk capacity. Use M for MiB or K for KiB. Range: 1M–64M, subject to free device PSRAM. Original ESP32: usually 2M with 4 MiB usable PSRAM; S3: up to 4M with 8 MiB PSRAM; P4: up to 16M with 32 MiB PSRAM (estimate; 4M tested). A larger choice can fail to boot if RAM is insufficient.'),
            ('vga_mem_size','VGA memory',VGA_PRESETS,
             'Emulated video RAM, allocated in addition to guest RAM. 256K is the default for standard DOS VGA games. Larger values may help software needing more video memory, but consume more device PSRAM. Range: 256K–2048K, in 64K steps; this does not set screen resolution.'),
            ('cpu_gen','CPU generation',('3','4','5'),
             '3 = 386 (default), 4 = 486, 5 = Pentium. Selects CPU identity/features for compatibility, not clock speed. A newer generation does not guarantee better performance. FPU emulation is unavailable; the INI always uses fpu = 0.')]):
            label=ttk.Label(machine,text=title); label.grid(row=row,column=0,sticky='w',pady=5)
            widget=ttk.Combobox(machine,textvariable=self.vars[key],values=choices,width=18,
                                state='readonly' if key=='cpu_gen' else 'normal')
            widget.grid(row=row,column=1,sticky='w',padx=12,pady=5)
            self.fields[key]=widget; help_for(widget,help_text); help_for(label,help_text)
        for row,(key,title,help_text) in enumerate([
            ('fill_cmos','Fill CMOS disk information',
             'Populate emulated BIOS CMOS with the attached hard-disk geometry. Enabled by default for the bundled BIOS. Disable only for a known compatibility reason.'),
            ('vga_force_8dm','Force 8-dot text characters',
             'Use 8-pixel-wide DOS text characters instead of 9, keeping 80-column text 640 pixels wide. Enabled by default. This affects text width, not the game graphics resolution or Retro-Go scaling.')],start=3):
            widget=ttk.Checkbutton(machine,text=title,variable=self.vars[key]); widget.grid(row=row,column=0,columnspan=2,sticky='w',pady=5)
            self.fields[key]=widget; help_for(widget,help_text)
        ttk.Label(outer,text='2M guest: roughly 1 MiB combined XMS/EMS; split it when both are enabled.\n4M+ guest: up to 1 MiB each. DOS/drivers may leave less free.',wraplength=600).pack(anchor='w',pady=10)
        controls=ttk.LabelFrame(outer,text='Controller → keyboard',padding=10); controls.pack(fill='x')
        for index,button in enumerate(BUTTONS):
            row=index%4; col=(index//4)*2
            label=ttk.Label(controls,text=button.upper()); label.grid(row=row,column=col,sticky='w',padx=(0,10),pady=5)
            widget=ttk.Combobox(controls,textvariable=self.vars['key_'+button],values=KEYS,width=17,state='readonly')
            widget.grid(row=row,column=col+1,sticky='w',padx=(0,24),pady=5)
            self.fields['key_'+button]=widget
            description=f'Keyboard key held while the {button.upper()} controller button is held. Choose none to disable it. One key per button; key combinations are unsupported. These mappings are saved per game.'
            help_for(widget,description); help_for(label,description)
        ttk.Label(outer,text='Menu / Option remain Retro-Go controls. Changes apply at the next game launch.',wraplength=600).pack(anchor='w',pady=(10,0))
        if editing:
            ttk.Label(outer,text='Apply backs up and updates the companion .ini; game files and saves stay intact.').pack(anchor='w',pady=(6,0))
        actions=ttk.Frame(outer); actions.pack(fill='x',pady=(14,0))
        ttk.Button(actions,text='Cancel',command=self.close).pack(side='right')
        ttk.Button(actions,text='Apply',command=self.apply).pack(side='right',padx=8)
        self.window.protocol('WM_DELETE_WINDOW',self.close)
        self.window.bind('<Escape>',lambda event:self.close())
        self.window.grab_set()

    def apply(self):
        try:
            settings=machine_from_form({key:value.get() for key,value in self.vars.items()})
            self.on_apply(settings)
        except Exception as e:
            messagebox.showerror('Cannot apply machine settings',str(e),parent=self.window)
            return
        self.close()

    def close(self):
        self.window.grab_release(); self.window.destroy()
