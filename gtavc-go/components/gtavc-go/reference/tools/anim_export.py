#!/usr/bin/env python3
"""ped.ifp -> anim.bin: per frame world rotations for the 9 rig parts + pelvis, plus pelvis offset.
Layout (big endian): u16 nClips ; per clip: u8 nameLen,name ; u16 durationMs ; u16 nFrames ; u8 loop ; s16 travel ;
  nFrames * ( 10 matrices * 9 s16 (2.14) , 3 s16 root offset (1/64 m: right, up, fwd) )"""
import os, sys, struct, io, contextlib
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
with contextlib.redirect_stdout(io.StringIO()):
    from ifp import parse_ifp
IFP = os.environ.get('PEDIFP', '/home/claude/work/pk/ped.ifp')
A, ORDER = parse_ifp(IFP)
PAR = {'Root': None, 'Pelvis': 'Root', 'Spine': 'Pelvis', 'Spine1': 'Spine', 'Neck': 'Spine1', 'Head': 'Neck', 'Jaw': 'Head',
       'Bip01 L Clavicle': 'Spine1', 'L UpperArm': 'Bip01 L Clavicle', 'L Forearm': 'L UpperArm', 'L Hand': 'L Forearm', 'L Finger': 'L Hand',
       'Bip01 R Clavicle': 'Spine1', 'R UpperArm': 'Bip01 R Clavicle', 'R Forearm': 'R UpperArm', 'R Hand': 'R Forearm', 'R Finger': 'R Hand',
       'L Thigh': 'Pelvis', 'L Calf': 'L Thigh', 'L Foot': 'L Calf', 'R Thigh': 'Pelvis', 'R Calf': 'R Thigh', 'R Foot': 'R Calf'}
S = np.array([[1, 0, 0], [0, 0, 1], [0, 1, 0]], float)
FPS = 20
CLIPS = [  # (name, loop)
 ('IDLE_stance', 1), ('woman_idlestance', 1), ('WALK_player', 1), ('WALK_civi', 1), ('WOMAN_walknorm', 1), ('walk_back', 1),
 ('run_player', 1), ('run_civi', 1), ('woman_run', 1), ('sprint_civi', 1), ('sprint_panic', 1), ('run_back', 1),
 ('FIGHTIDLE', 1), ('FIGHTppunch', 0), ('FIGHTpunch', 0), ('FIGHTjab', 0), ('FIGHTkick', 0),
 ('HIT_front', 0), ('HIT_back', 0), ('HIT_L', 0), ('HIT_R', 0), ('KO_shot_front', 0), ('KO_skid_back', 0), ('KO_spin_L', 0), ('FALL_front', 0), ('FALL_back', 0),
 ('getup', 0), ('getup_front', 0), ('cower', 1), ('handscower', 1), ('turn_180', 0),
 ('CAR_open_LHS', 0), ('CAR_getin_LHS', 0), ('CAR_getout_LHS', 0), ('CAR_closedoor_LHS', 0), ('CAR_jackedLHS', 0), ('CAR_Qjack', 0), ('CAR_Qjacked', 0),
 ('CAR_pullout_LHS', 0), ('CAR_sit', 1), ('Drive_L', 1), ('SEAT_idle', 1), ('JUMP_launch', 0), ('JUMP_glide', 0), ('JUMP_land', 0),
]
def qmat(q):
    x, y, z, w = q; n = np.sqrt(x*x+y*y+z*z+w*w); x, y, z, w = x/n, y/n, z/n, w/n
    return np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)], [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)], [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]]).T   # stored conjugated
def nlerp(q0, q1, t):
    q0 = np.array(q0); q1 = np.array(q1)
    if q0 @ q1 < 0: q1 = -q1
    q = q0*(1-t)+q1*t; return q/np.linalg.norm(q)
def sample(fr, t, key=1):
    if t <= fr[0][0]: return np.array(fr[0][key])
    if t >= fr[-1][0]: return np.array(fr[-1][key])
    for i in range(len(fr)-1):
        if fr[i][0] <= t <= fr[i+1][0]:
            u = (t-fr[i][0])/max(1e-6, fr[i+1][0]-fr[i][0])
            if key == 1: return nlerp(fr[i][1], fr[i+1][1], u)
            return np.array(fr[i][key])*(1-u)+np.array(fr[i+1][key])*u
def world(anim, t):
    bones = {b[0]: b for b in A[anim]}; out = {}
    def get(n):
        if n in out: return out[n]
        if n not in bones: out[n] = get(PAR[n]) if PAR[n] else np.eye(3); return out[n]
        R = qmat(sample(bones[n][2], t)); p = PAR[n]
        if p: R = get(p) @ R
        out[n] = R; return R
    for n in PAR: get(n)
    return out
def pelvis_tr(anim, t):
    for b in A[anim]:
        if b[0] == 'Root' and b[2][0][2] is not None: return sample(b[2], t, 2)
    return np.zeros(3)
IDLE = world('IDLE_stance', 0.0); IDLE_TR = pelvis_tr('IDLE_stance', 0.0)
def mini(a, b):
    a = a/np.linalg.norm(a); b = b/np.linalg.norm(b); v = np.cross(a, b); c = a@b; s = np.linalg.norm(v)
    if s < 1e-8: return np.eye(3)
    K = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]]); return np.eye(3)+K+K@K*((1-c)/(s*s))
REST = {'Pelvis': IDLE['Pelvis'], 'Spine1': IDLE['Spine1']}
for s in 'LR':
    th = IDLE[f'{s} Thigh']; REST[f'{s} Thigh'] = th; REST[f'{s} Calf'] = th
    ua = IDLE[f'{s} UpperArm']; rest = mini(ua[:, 0], np.array([-1.0 if s == 'L' else 1.0, 0, 0])) @ ua
    REST[f'{s} UpperArm'] = rest; REST[f'{s} Forearm'] = rest
# part order of PedModel: torso, armA, foreA, armB, foreB, thighA, shinA, thighB, shinB ; A = engine +x = character's RIGHT ; plus pelvis
ORDER_BONES = ['Spine1', 'R UpperArm', 'R Forearm', 'L UpperArm', 'L Forearm', 'R Thigh', 'R Calf', 'L Thigh', 'L Calf', 'Pelvis']
def frame_data(anim, t, keep_fwd):
    start = pelvis_tr(anim, 0.0)
    W = world(anim, t); out = []
    for b in ORDER_BONES:
        D = S @ (W[b] @ REST[b].T) @ S
        out += [int(round(np.clip(v, -1.99, 1.99)*16384)) for v in D.flatten()]
    cur = pelvis_tr(anim, t)
    d = cur - IDLE_TR                            # GTA axes: x right, y fwd, z up
    if keep_fwd: d[0] = cur[0] - start[0]; d[1] = cur[1] - start[1]      # one-shots: horizontal motion relative to the clip start
    off = np.array([d[0], d[2], d[1]])*64.0     # engine: right, up, fwd
    if not keep_fwd: off[2] = 0
    out += [int(round(np.clip(v, -32000, 32000))) for v in off]
    return out
def export(path):
    out = bytearray(); clips = [c for c in CLIPS if c[0] in A]
    out += struct.pack('>H', len(clips)); report = []
    for name, loop in clips:
        dur = max(b[2][-1][0] for b in A[name]); nf = max(2, int(round(dur*FPS))+1)
        keep_fwd = not loop                      # locomotion: speed comes from the game, keep bob only
        fr = [frame_data(name, min(dur, i/FPS), keep_fwd) for i in range(nf)]
        tr0 = pelvis_tr(name, 0.0); tr1 = pelvis_tr(name, dur)
        travel = int(round(abs(tr1[1]-tr0[1])*64)) if loop else 0
        out += struct.pack('>B', len(name)) + name.encode() + struct.pack('>HHBh', int(dur*1000), nf, loop, travel)
        for f in fr: out += struct.pack('>93h', *f)
        report.append((name, dur, nf, fr[0][90:93], fr[-1][90:93]))
    open(path, 'wb').write(out)
    return report
if __name__ == '__main__':
    rep = export(os.path.join(HERE, '..', 'res', 'anim.bin'))
    for n, d, nf, a, b in rep:
        if False: print(f'{n:18s} {d:.2f}s {nf:3d}f  root start(r,u,f)={a} end={b}')
    print(len(rep), 'clips,', os.path.getsize(os.path.join(HERE, '..', 'res', 'anim.bin')), 'bytes')
