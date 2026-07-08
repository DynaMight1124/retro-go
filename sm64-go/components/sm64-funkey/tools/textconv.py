import sys
import os

def parse_charmap(charmap_file):
    charmap = {}
    with open(charmap_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('//'): continue
            if ' = ' in line:
                key_part, val_part = line.rsplit(' = ', 1)
                if key_part.startswith("'") and key_part.endswith("'"):
                    key = key_part[1:-1]
                    # basic escape handling for the charmap key itself
                    key = key.replace("\\'", "'").replace("\\\\", "\\").replace("\\n", "\n")
                    
                    bytes_list = []
                    for v in val_part.split():
                        v = v.rstrip(',')
                        if v.startswith('0x'):
                            bytes_list.append(int(v, 16))
                    charmap[key] = bytes_list
    # Sort keys by length descending to match longest possible sequence first
    sorted_keys = sorted(charmap.keys(), key=lambda x: len(x), reverse=True)
    return charmap, sorted_keys

def convert_string(s, charmap, sorted_keys, uncompressed=False):
    # Handle common escapes in the input string
    s = s.replace("\\n", "\n").replace('\\"', '"').replace("\\\\", "\\")
    
    out = []
    i = 0
    while i < len(s):
        match_found = False
        for k in sorted_keys:
            if uncompressed and len(k) > 1:
                continue
            if s.startswith(k, i):
                out.extend(charmap[k])
                i += len(k)
                match_found = True
                break
        if not match_found:
            # Fallback for unmapped characters
            # print(f"Warning: Unmapped char at index {i}: {repr(s[i])}", file=sys.stderr)
            out.append(0x9E) # Space fallback
            i += 1
    out.append(0xFF) # Terminator
    return out

def process_file(charmap_file, in_file, out_file):
    charmap, sorted_keys = parse_charmap(charmap_file)
    with open(in_file, 'r', encoding='utf-8') as f:
        content = f.read()

    output = []
    pos = 0
    while pos < len(content):
        # Look for _( or __(
        if content.startswith('__(', pos):
            uncompressed = True
            pos += 3
        elif content.startswith('_(', pos):
            uncompressed = False
            pos += 2
        else:
            output.append(content[pos])
            pos += 1
            continue
            
        # Inside _( ... )
        full_string = ""
        while True:
            # skip whitespace
            while pos < len(content) and content[pos].isspace():
                pos += 1
            if pos >= len(content): break
            
            if content[pos] == '"':
                # start of a quoted string
                pos += 1
                start = pos
                while pos < len(content):
                    if content[pos] == '"' and content[pos-1] != '\\':
                        break
                    pos += 1
                full_string += content[start:pos]
                pos += 1 # skip closing "
            elif content[pos] == ')':
                pos += 1
                break
            else:
                # print(f"Unexpected char in _(): {content[pos]}", file=sys.stderr)
                pos += 1
        
        bytes_list = convert_string(full_string, charmap, sorted_keys, uncompressed)
        output.append("".join([f"0x{b:02X}, " for b in bytes_list]).strip()[:-1])

    with open(out_file, 'w', encoding='utf-8') as f:
        f.write("".join(output))

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print("Usage: textconv.py charmap in_file out_file")
        sys.exit(1)
    process_file(sys.argv[1], sys.argv[2], sys.argv[3])
