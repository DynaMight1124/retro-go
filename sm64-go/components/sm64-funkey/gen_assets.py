import subprocess
import os

def gen():
    # gen mario_anims.c
    with open('assets/mario_anims.c', 'wb') as f:
        subprocess.run(['python', 'tools/mario_anims_converter.py'], stdout=f, check=True)
    
    # gen demo_data.c
    with open('assets/demo_data.c', 'wb') as f:
        subprocess.run(['python', 'tools/demo_data_converter.py', 'assets/demo_data.json', '-D', 'VERSION_US'], stdout=f, check=True)

if __name__ == '__main__':
    gen()
