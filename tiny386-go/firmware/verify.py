"""Verify bundled source/binary provenance and VGA option ROM structure."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parent
manifest = json.loads((root / 'manifest.json').read_text())
for name, expected in manifest['files'].items():
    data = (root / name).read_bytes()
    if len(data) != expected['size'] or hashlib.sha256(data).hexdigest() != expected['sha256']:
        raise SystemExit('Firmware integrity failure: ' + name)
bios = (root / 'bios.bin').read_bytes()
vga = (root / 'vgabios.bin').read_bytes()
assert len(bios) == 131072 and bios[-16] == 0xea, 'Invalid system BIOS reset vector'
assert vga[:2] == b'\x55\xaa' and vga[2] * 512 == len(vga), 'Invalid VGA ROM header/size'
assert sum(vga) % 256 == 0, 'Invalid VGA ROM checksum'
print('Bundled BIOS/source hashes, reset vector and VGA ROM checksum: PASS')
