#!/usr/bin/env python3
# smb1.py: samba v1 server
# Usage: smb1.py <listen port> <share_path>

import os
import sys
import subprocess
import tempfile
import atexit
import shutil
import getpass

LISTEN_IP = "127.0.0.1"
LISTEN_PORT = int(sys.argv[1])
SHARE_NAME = "share"
SHARE_PATH = sys.argv[2]

if not os.path.exists(SHARE_PATH):
    sys.exit(1)

tmp_dir = tempfile.mkdtemp(prefix="smb-")
log_dir = os.path.join(tmp_dir, "log")
conf_path = os.path.join(tmp_dir, "smb.conf")
os.makedirs(log_dir, exist_ok=True)

def cleanup():
    shutil.rmtree(tmp_dir, ignore_errors=True)

atexit.register(cleanup)

smb_conf_content = f"""
[global]
rpc start on demand helpers = no
interfaces = {LISTEN_IP}
bind interfaces only = yes
smb ports = {LISTEN_PORT}
private dir = {tmp_dir}
state directory = {tmp_dir}
cache directory = {tmp_dir}
lock directory = {tmp_dir}
pid directory = {tmp_dir}
rpc_server:epmapper = disabled
ncalrpc dir = {tmp_dir}
usershare max shares = 0
logging = file
log level = 0
max log size = 50
load printers = no
printing = bsd
disable spoolss = yes
server min protocol = NT1
server max protocol = NT1
lanman auth = yes
ntlm auth = yes
raw NTLMv2 auth = no
map to guest = bad user
[{SHARE_NAME}]
path = {SHARE_PATH}
read only = no
guest ok = yes
writable = yes
browseable = yes
force user = {getpass.getuser()}
"""

with open(conf_path, "w", encoding="utf-8") as f:
    f.write(smb_conf_content)

try:
    exit(subprocess.call(["/usr/sbin/smbd", "-F", "-l", log_dir, "-s", conf_path]))
except:
    pass
