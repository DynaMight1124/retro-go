"""Tkinter desktop interface for creating and maintaining DOS game disks."""
from pathlib import Path
from queue import Empty
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, simpledialog
from .gui_model import Worker, request_from_form, memory_from_form, machine_from_form
from .memory import SIZES
from .settings_ui import MachineDialog, add_machine_vars, set_machine_vars, help_for
from .service import inspect_game, create_game, inspect_image, update_image, extract_file


class App:
    def __init__(self,root):
        self.root=root; self.worker=Worker(); self.pending=False; self.callback=None; self.controls=[]
        root.title('Tiny386 DOS Image Builder'); root.geometry('900x790'); root.minsize(800,740)
        root.protocol('WM_DELETE_WINDOW',self.close)
        style=ttk.Style(root)
        if 'vista' in style.theme_names(): style.theme_use('vista')
        outer=ttk.Frame(root,padding=18); outer.pack(fill='both',expand=True)
        ttk.Label(outer,text='DOS games for Retro-Go',font=('TkDefaultFont',18,'bold')).pack(anchor='w')
        ttk.Label(outer,text='Create a bootable FreeDOS disk, then copy its .img and .ini files to your SD card.').pack(anchor='w',pady=(5,14))
        tabs=ttk.Notebook(outer); tabs.pack(fill='both',expand=True)
        create=ttk.Frame(tabs,padding=16); edit=ttk.Frame(tabs,padding=16)
        tabs.add(create,text='Create game image'); tabs.add(edit,text='Open / update image')
        self.vars={k:tk.StringVar(value=v) for k,v in dict(game_dir='',startup='',arguments='',output_dir='',name='Game',size='4',custom='').items()}
        self.vars['dos_high']=tk.BooleanVar(value=True)
        self.add_memory_vars(self.vars)
        add_machine_vars(root,self.vars)
        create.columnconfigure(1,weight=1)
        self.field(create,0,'Game folder','game_dir',lambda:self.pick_folder('game_dir'))
        ttk.Label(create,text='Startup file').grid(row=1,column=0,sticky='w',pady=7)
        self.startup=ttk.Combobox(create,textvariable=self.vars['startup'],state='readonly'); self.startup.grid(row=1,column=1,sticky='ew',padx=10)
        self.controls.append((self.startup,'readonly'))
        self.button(create,'Scan folder',self.scan).grid(row=1,column=2)
        self.field(create,2,'Arguments (optional)','arguments')
        ttk.Label(create,text='Disk capacity (MiB)').grid(row=3,column=0,sticky='w',pady=7)
        sizes=ttk.Frame(create); sizes.grid(row=3,column=1,sticky='ew',padx=10)
        size=ttk.Combobox(sizes,textvariable=self.vars['size'],values=['4','8','16','32','64','Custom'],width=10,state='readonly'); size.pack(side='left'); self.controls.append((size,'readonly'))
        ttk.Label(sizes,text='Custom:').pack(side='left',padx=(12,5))
        custom=ttk.Entry(sizes,textvariable=self.vars['custom'],width=10); custom.pack(side='left'); self.controls.append((custom,'normal'))
        ttk.Label(create,text='Disk space only; does not increase emulated RAM.').grid(row=4,column=1,sticky='w',padx=10)
        self.field(create,5,'Output folder','output_dir',lambda:self.pick_folder('output_dir'))
        self.field(create,6,'Image name','name')
        self.memory_panel(create,self.vars).grid(row=7,column=0,columnspan=3,sticky='ew',pady=7)
        actions=ttk.Frame(create); actions.grid(row=8,column=0,columnspan=3,sticky='e',pady=15)
        self.button(actions,'Machine settings…',self.machine_settings).pack(side='left',padx=6)
        self.button(actions,'Check space',self.check).pack(side='left',padx=6)
        self.button(actions,'Create image',self.create).pack(side='left')
        self.summary=tk.StringVar(value='Choose a game folder and scan for startup files. Sizes: 4–2048 MiB.')
        ttk.Label(create,textvariable=self.summary,wraplength=660).grid(row=9,column=0,columnspan=3,sticky='w')
        edit.columnconfigure(0,weight=1); edit.rowconfigure(2,weight=1)
        bar=ttk.Frame(edit); bar.grid(row=0,column=0,sticky='ew')
        self.button(bar,'Open image…',self.open_image).pack(side='left')
        self.image_path=None; self.image_info=None
        self.image_label=tk.StringVar(value='Open an image created by this tool.')
        ttk.Label(edit,textvariable=self.image_label,wraplength=660).grid(row=1,column=0,sticky='w',pady=10)
        frame=ttk.Frame(edit); frame.grid(row=2,column=0,sticky='nsew')
        self.files=tk.Listbox(frame,exportselection=False); self.files.pack(side='left',fill='both',expand=True)
        scroll=ttk.Scrollbar(frame,command=self.files.yview); scroll.pack(side='right',fill='y'); self.files.configure(yscrollcommand=scroll.set)
        self.edit_vars={'dos_high':tk.BooleanVar(value=True)}; self.add_memory_vars(self.edit_vars)
        add_machine_vars(root,self.edit_vars)
        self.memory_panel(edit,self.edit_vars).grid(row=3,column=0,sticky='ew',pady=(10,0))
        actions=ttk.Frame(edit); actions.grid(row=4,column=0,sticky='w',pady=12)
        for title,action in [('Extract selected…',self.extract),('Add / replace file…',self.add),('Change startup…',self.change_startup)]:
            self.button(actions,title,action).pack(side='left',padx=(0,8))
        self.button(actions,'Apply memory settings',self.change_memory).pack(side='left')
        self.button(bar,'Machine settings…',lambda:self.machine_settings(edit=True)).pack(side='left',padx=8)
        self.status=tk.StringVar(value='Ready')
        ttk.Label(outer,textvariable=self.status,wraplength=760).pack(anchor='w',pady=(12,5))
        self.progress=ttk.Progressbar(outer,mode='indeterminate'); self.progress.pack(fill='x')
        self.timer=root.after(80,self.poll)

    def add_memory_vars(self,values):
        values.update(xms=tk.BooleanVar(value=True),ems=tk.BooleanVar(value=False),
                      xms_kib=tk.StringVar(value='1024'),ems_kib=tk.StringVar(value='1024'))

    def memory_panel(self,parent,values):
        panel=ttk.LabelFrame(parent,text='DOS memory',padding=8)
        for col,(key,title) in enumerate([('xms','XMS'),('ems','EMS')]):
            group=ttk.Frame(panel); group.grid(row=0,column=col,sticky='w',padx=(0,22))
            check=ttk.Checkbutton(group,text=title,variable=values[key]); check.pack(side='left')
            size=ttk.Combobox(group,textvariable=values[key+'_kib'],values=[str(n) for n in SIZES],width=7,state='readonly'); size.pack(side='left',padx=6)
            ttk.Label(group,text='KiB limit').pack(side='left')
            self.controls.extend([(check,'normal'),(size,'readonly')])
            description=('Extended memory for games. The selected KiB value caps free XMS; DOS and drivers also use guest RAM.' if key=='xms' else
                         'Expanded memory for games that require EMS. Off by default. Uses JEMM386 and consumes part of guest RAM; it can affect compatibility or performance.')
            help_for(check,description); help_for(size,description)
        high=ttk.Checkbutton(panel,text='Load DOS high',variable=values['dos_high']); high.grid(row=1,column=0,columnspan=2,sticky='w',pady=(5,0)); self.controls.append((high,'normal'))
        help_for(high,'Loads DOS into the high memory area to free conventional RAM for games. Requires an XMS driver, even if free XMS is disabled. Recommended on; disable for a known compatibility problem.')
        guide=('2 MB guest: plan around 1 MB combined XMS/EMS; split it if both are enabled. DOS/drivers may leave less.\n'
               '4 MB+ guest: up to 1 MB each. Change guest RAM in Machine settings; disk size does not change RAM.\n'
               'Rough guest-RAM guide: ESP32 2 MB; S3 up to 4 MB with 8 MB PSRAM; P4 up to 16 MB with 32 MB PSRAM (4 MB tested).\n'
               'Limits depend on device/free PSRAM. EMS needs an XMS driver even when XMS memory is disabled.')
        ttk.Label(panel,text=guide,wraplength=670).grid(row=2,column=0,columnspan=2,sticky='w',pady=(5,0))
        return panel

    def button(self,parent,text,command):
        button=ttk.Button(parent,text=text,command=lambda:self.guard(command)); self.controls.append((button,'normal')); return button
    def field(self,parent,row,label,key,browse=None):
        ttk.Label(parent,text=label).grid(row=row,column=0,sticky='w',pady=7)
        entry=ttk.Entry(parent,textvariable=self.vars[key]); entry.grid(row=row,column=1,sticky='ew',padx=10); self.controls.append((entry,'normal'))
        if browse: self.button(parent,'Browse…',browse).grid(row=row,column=2)
    def guard(self,action):
        if self.pending: return
        try: action()
        except Exception as e: messagebox.showerror('Cannot complete operation',str(e),parent=self.root)
    def pick_folder(self,key):
        path=filedialog.askdirectory(parent=self.root)
        if path:
            self.vars[key].set(path)
            if key=='game_dir': self.vars['startup'].set(''); self.startup['values']=(); self.scan()
    def request(self): return request_from_form({k:v.get() for k,v in self.vars.items()})
    def start(self,action,callback):
        self.worker.start(action)
        self.pending=True; self.callback=callback
        for control,_ in self.controls: control.configure(state='disabled')
        self.progress.start(12)
    def poll(self):
        try:
            while True:
                event,data=self.worker.events.get_nowait()
                if event=='progress': self.status.set(data)
                else:
                    # Terminal event is queued just before thread return. Wait for
                    # that return before callbacks can start a follow-up worker.
                    self.worker.thread.join()
                    self.pending=False; self.progress.stop()
                    callback=self.callback; self.callback=None
                    for control,state in self.controls: control.configure(state=state)
                    if event=='error':
                        self.status.set('Operation failed; see details.'); messagebox.showerror('Operation failed',data,parent=self.root)
                    else:
                        self.status.set('Ready')
                        try: callback(data)
                        except Exception as e: messagebox.showerror('Operation result',str(e),parent=self.root)
        except Empty: pass
        self.timer=self.root.after(80,self.poll)
    def scan(self):
        # Scanning does not require output-folder selection.
        values={k:v.get() for k,v in self.vars.items()}; values['startup']=''; values['output_dir']=values['output_dir'] or '.'
        req=request_from_form(values)
        def done(info):
            self.startup['values']=info['candidates']; self.vars['startup'].set(info['candidates'][0]); self.show_space(info)
        self.start(lambda progress:inspect_game(req),done)
    def show_space(self,info):
        self.summary.set(f"Estimated content: {info['required_bytes']/1048576:.2f} MiB. Actual disk: {info['actual_bytes']/1048576:.2f} MiB. Estimated free: {info['free_estimate']/1048576:.2f} MiB.")
    def check(self):
        req=self.request(); self.start(lambda progress:inspect_game(req),self.show_space)
    def create(self):
        req=self.request(); out=req.output_dir
        exists=any((out/(req.name+ext)).exists() for ext in ('.img','.ini','.json'))
        if exists and not messagebox.askyesno('Replace outputs?', 'Existing outputs will be backed up and replaced. Continue?',parent=self.root): return
        def done(result):
            self.status.set('Image created successfully.')
            messagebox.showinfo('Game image ready',f"Copy these files together to the Retro-Go game folder:\n\n{result['image']}\n{result['descriptor']}\n\nKeep the .json manifest on your computer.",parent=self.root)
        self.start(lambda progress:create_game(req,progress,overwrite=exists),done)
    def open_image(self):
        path=filedialog.askopenfilename(parent=self.root,filetypes=[('Disk images','*.img'),('All files','*')])
        if path: self.load_image(Path(path))
    def load_image(self,path):
        def done(info):
            self.image_path=path; self.image_info=info; self.files.delete(0,'end')
            for name in info['files']: self.files.insert('end',name)
            self.image_label.set(f"{path.name} — {info['free_bytes']/1048576:.2f} MiB free"+('' if info['writable'] else ' (read only)'))
            if info['memory'] is not None:
                memory=info['memory']
                for key in ('xms','ems','xms_kib','ems_kib'): self.edit_vars[key].set(getattr(memory,key))
                self.edit_vars['dos_high'].set(info['dos_high'])
            if info['settings'] is not None: set_machine_vars(self.edit_vars,info['settings'])
            if info['settings_error']: self.image_label.set(self.image_label.get()+' — Machine settings unavailable: '+info['settings_error'])
            if info['memory_error']: self.image_label.set(self.image_label.get()+' — Memory editing unavailable: '+info['memory_error'])
            elif info['memory_legacy']: self.image_label.set(self.image_label.get()+' — Legacy memory setup; selected limits apply when saved.')
        self.start(lambda progress:inspect_image(path),done)
    def selected(self):
        selection=self.files.curselection()
        if self.image_path is None or not selection: raise ValueError('Open an image and select a file first')
        return self.files.get(selection[0])
    def extract(self):
        name=self.selected(); path=filedialog.asksaveasfilename(parent=self.root,initialfile=Path(name).name)
        if path:
            image=self.image_path; self.start(lambda progress:extract_file(image,name,Path(path)),lambda result:self.status.set('File extracted.'))
    def writable(self):
        if self.image_path is None or not self.image_info['writable']: raise ValueError('Open a tool-created writable image first')
    def add(self):
        self.writable(); path=filedialog.askopenfilename(parent=self.root)
        if not path: return
        name=simpledialog.askstring('Destination','DOS path inside image (for example GAME/LEVEL.DAT):',initialvalue='GAME/'+Path(path).name.upper(),parent=self.root)
        if not name: return
        if not messagebox.askyesno('Update image','Add or replace this file? The existing image will be backed up.',parent=self.root): return
        image=self.image_path
        self.start(lambda progress:update_image(image,{name:Path(path)},progress=progress),lambda result:self.load_image(image))
    def change_startup(self):
        self.writable(); selected=self.selected()
        if not selected.endswith(('.EXE','.COM','.BAT')): raise ValueError('Select an EXE, COM or BAT file')
        args=simpledialog.askstring('Startup arguments','Optional arguments:',initialvalue='',parent=self.root)
        if args is None: return
        if not messagebox.askyesno('Change startup','Start this file automatically? The image will be backed up.',parent=self.root): return
        image=self.image_path
        self.start(lambda progress:update_image(image,{},selected,args,progress),lambda result:self.load_image(image))
    def change_memory(self):
        self.writable()
        if not self.image_info['memory_editable']: raise ValueError(self.image_info['memory_error'] or 'Memory editing is unavailable for this image')
        values={key:value.get() for key,value in self.edit_vars.items()}
        memory=memory_from_form(values); image=self.image_path
        if not messagebox.askyesno('Change memory settings','Apply these DOS memory settings? Game files and saves will be preserved, and the image will be backed up. The current descriptor RAM size is unchanged.',parent=self.root): return
        self.start(lambda progress:update_image(image,{},progress=progress,memory=memory,dos_high=values['dos_high']),lambda result:self.load_image(image))
    def machine_settings(self,edit=False):
        if self.pending: return
        values=self.edit_vars if edit else self.vars
        if edit:
            self.writable()
            if not self.image_info['settings_editable']: raise ValueError(self.image_info['settings_error'])
        settings=machine_from_form({key:value.get() for key,value in values.items()})
        def apply(selected):
            if edit:
                image=self.image_path
                self.start(lambda progress:update_image(image,{},progress=progress,settings=selected),lambda result:self.load_image(image))
            else: set_machine_vars(self.vars,selected)
        return MachineDialog(self.root,settings,apply,editing=edit)
    def close(self):
        if self.pending:
            messagebox.showinfo('Operation in progress','Please wait for the current operation to finish before closing.',parent=self.root); return
        self.root.after_cancel(self.timer)
        self.root.destroy()


def main():
    root=tk.Tk(); App(root); root.mainloop()
