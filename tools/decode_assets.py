#!/usr/bin/env python3
"""
Decode all XQuest original binary/text assets into JSON files.
Run from the xquest-sdl root: python3 tools/decode_assets.py
"""

import struct
import json
import base64
import os
import sys

SRC = os.path.join(os.path.dirname(__file__), '../../xquest')
OUT = os.path.join(os.path.dirname(__file__), '../assets')
os.makedirs(OUT, exist_ok=True)

def src(name):
    return os.path.join(SRC, name)

def out(name):
    return os.path.join(OUT, name)

def save(name, data):
    path = out(name)
    with open(path, 'w') as f:
        json.dump(data, f, indent=2)
    size = os.path.getsize(path)
    print(f'  wrote {name} ({size} bytes)')

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def vga_to_rgb(v):
    """VGA palette value (0-63) to 0-255."""
    return round(v * 255 / 63)

def read_sprite(data, pos):
    """Read one sprite: uint16 width, uint16 height, then width*height pixels.
    The file always stores rows padded to a multiple of 4 bytes (bmwidth*4),
    but since the stored width is always already a multiple of 4 the stride
    equals the width."""
    width, height = struct.unpack_from('<HH', data, pos)
    pos += 4
    stride = ((width - 1) // 4 + 1) * 4   # ceil(width/4)*4
    pixels = list(data[pos : pos + stride * height])
    pos += stride * height
    # Trim padding columns so width==visual_width
    if stride != width and width > 0:
        rows = []
        for r in range(height):
            rows.extend(pixels[r*stride : r*stride + width])
        pixels = rows
    return {'width': width, 'height': height, 'pixels': pixels}, pos

# ---------------------------------------------------------------------------
# 1. Soft data (directly embedded in Pascal sources)
# ---------------------------------------------------------------------------

def decode_soft_data():
    print('Decoding soft data from Pascal sources...')

    # --- Palette from palette.inc ---
    # Format: array[0..769] where [0]=start_index, [1]=num_entries, then R,G,B triplets
    palette_raw = [
        0,255,
        0,0,0,2,2,2,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,
        16,16,16,18,18,18,20,20,20,22,22,22,24,24,24,26,26,26,28,28,28,
        30,30,30,32,32,32,34,34,34,36,36,36,38,38,38,40,40,40,42,42,42,
        44,44,44,46,46,46,48,48,48,50,50,50,52,52,52,54,54,54,56,56,56,
        58,58,58,60,60,60,63,63,63,
        0,0,6,0,0,14,0,0,22,0,0,30,0,0,38,0,0,46,0,0,54,0,0,63,
        6,6,63,14,14,63,22,22,63,30,30,63,38,38,63,46,46,63,54,54,63,
        6,0,0,14,0,0,22,0,0,30,0,0,38,0,0,46,0,0,54,0,0,63,0,0,
        63,6,6,63,14,14,63,22,22,63,30,30,63,38,38,63,46,46,63,54,54,
        0,6,0,0,14,0,0,22,0,0,30,0,0,38,0,0,46,0,0,54,0,0,63,0,
        6,63,6,14,63,14,22,63,22,30,63,30,38,63,38,46,63,46,54,63,54,
        6,0,6,14,0,14,22,0,22,30,0,30,38,0,38,46,0,46,54,0,54,63,0,63,
        63,6,63,63,14,63,63,22,63,63,30,63,63,38,63,63,46,63,63,54,63,
        6,6,0,14,14,0,22,22,0,30,30,0,38,38,0,46,46,0,54,54,0,63,63,0,
        63,63,6,63,63,14,63,63,22,63,63,30,63,63,38,63,63,46,63,63,54,
        0,6,6,0,14,14,0,22,22,0,30,30,0,38,38,0,46,46,0,54,54,0,63,63,
        6,63,63,14,63,63,22,63,63,30,63,63,38,63,63,46,63,63,54,63,63,
        6,6,2,14,14,6,22,22,10,30,30,14,38,38,18,46,46,22,54,54,26,
        63,63,30,63,63,34,63,63,38,63,63,42,63,63,46,63,63,50,63,63,54,63,63,58,
        2,6,6,6,14,14,10,22,22,14,30,30,18,38,38,22,46,46,26,54,54,
        30,63,63,34,63,63,38,63,63,42,63,63,46,63,63,50,63,63,54,63,63,58,63,63,
        6,2,6,14,6,14,22,10,22,30,14,30,38,18,38,46,22,46,54,26,54,
        63,30,63,63,34,63,63,38,63,63,42,63,63,46,63,63,50,63,63,54,63,63,58,63,
        57,6,0,51,12,0,45,18,0,38,25,0,32,31,0,26,37,0,19,44,0,13,50,0,
        7,56,0,0,63,0,0,57,6,0,51,12,0,45,18,0,39,24,0,32,31,0,26,37,
        0,20,43,0,14,49,0,7,56,0,0,63,
        45,45,45,23,23,23,45,0,0,23,0,0,0,0,0,0,0,0,
        35,1,1,46,22,22,10,0,0,34,15,15,63,2,2,21,0,0,63,14,14,63,30,30,
        63,10,10,57,2,2,21,7,7,50,3,3,55,26,26,58,15,15,4,0,0,35,11,11,
        63,18,18,25,0,0,63,6,6,14,0,0,63,22,22,53,16,16,35,7,7,22,10,10,
        63,32,32,48,15,15,57,10,10,14,4,4,50,9,9,43,17,17,27,4,4,43,11,11,
        43,1,1,32,6,6,63,4,4,63,28,28,47,7,7,63,20,20,63,12,12,28,6,6,
        17,0,0,63,35,35,63,16,16,29,0,0,42,6,6,63,24,24,8,0,0,57,23,23,
        29,12,12,20,3,3,63,8,8,40,15,15,58,18,18,29,2,2,12,0,0,63,26,26,
        58,4,4,32,10,10,43,20,20,6,0,0,30,10,10,47,19,19,28,10,10,
    ]
    start = palette_raw[0]
    count = palette_raw[1]
    triplets = palette_raw[2:]
    palette = []
    for i in range(count):
        r6, g6, b6 = triplets[i*3], triplets[i*3+1], triplets[i*3+2]
        palette.append({
            'index': start + i,
            'r6': r6, 'g6': g6, 'b6': b6,
            'r': vga_to_rgb(r6), 'g': vga_to_rgb(g6), 'b': vga_to_rgb(b6),
        })

    # --- Enemy kinds (from xqenter.pas) ---
    enemy_names = [
        'SuperCrystal', 'Explosion', 'Grunger', 'Zippo', 'Zinger',
        'Vince', 'Hibernator_Sleeping', 'Miner', 'Meeby', 'Retaliator',
        'Terrier', 'Doinger', 'Snipe', 'Tribbler', 'Tribble',
        'Buckshot', 'Cluster', 'Sticktight', 'Repulsor',
    ]
    # sound name constants
    SND = {1:'fire6',2:'fire5',3:'phew',4:'fire4',5:'fire',6:'boing',
           7:'squelch',8:'woohoo',9:'allright',10:'ohyeah',11:'getcrystal',
           12:'explosn',13:'explosn2',14:'explosn3',15:'retaliate',16:'ow',
           17:'countdown',18:'gatesound',19:'sxtsmash',20:'bark',21:'applause',
           22:'enemyent',23:'menuclick',24:'doh',25:'repulse',0:'none'}
    enemy_kinds = [
        # index, speed, speed2, curve, curve2, hits, firetype, score, deathsound,
        # fires, follows, curves, explodes, laysmines, shootback, zoom, maxspeed,
        # rebounds, tribbles, repulses,
        # fireprob, changedir, changecurve, follow, numframes, framespeed
        {'index':0,'name':'SuperCrystal','speed':301,'speed2':150,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':0,'death_sound':'sxtsmash',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':True,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.002,'changecurve':0.0,'follow':0.0,'numframes':5,'framespeed':70},
        {'index':1,'name':'Explosion','speed':0,'speed2':0,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':0,'death_sound':'none',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.0,'changecurve':0.0,'follow':0.0,'numframes':5,'framespeed':64},
        {'index':2,'name':'Grunger','speed':121,'speed2':60,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':200,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.006,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':40},
        {'index':3,'name':'Zippo','speed':281,'speed2':140,'curve':6000,'curve2':3000,'hits':1,'firetype':0,'score':300,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':True,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.003,'changecurve':0.02,'follow':0.0,'numframes':3,'framespeed':56},
        {'index':4,'name':'Zinger','speed':101,'speed2':50,'curve':0,'curve2':0,'hits':1,'firetype':1,'score':300,'death_sound':'explosn',
         'fires':True,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.01,'changedir':0.006,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':60},
        {'index':5,'name':'Vince','speed':201,'speed2':100,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':500,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':True,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.003,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':56},
        {'index':6,'name':'Hibernator_Sleeping','speed':0,'speed2':0,'curve':0,'curve2':0,'hits':300,'firetype':0,'score':500,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.0,'changecurve':0.0,'follow':0.0,'numframes':0,'framespeed':32767},
        {'index':7,'name':'Miner','speed':121,'speed2':60,'curve':4000,'curve2':2000,'hits':1,'firetype':0,'score':600,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':True,'explodes':False,'laysmines':True,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.008,'changedir':0.006,'changecurve':0.1,'follow':0.0,'numframes':3,'framespeed':32},
        {'index':8,'name':'Meeby','speed':81,'speed2':40,'curve':0,'curve2':0,'hits':5,'firetype':0,'score':2000,'death_sound':'explosn',
         'fires':False,'follows':True,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.006,'changecurve':0.0,'follow':0.01,'numframes':5,'framespeed':28},
        {'index':9,'name':'Retaliator','speed':121,'speed2':60,'curve':0,'curve2':0,'hits':1,'firetype':3,'score':1000,'death_sound':'none',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':True,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.006,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':64},
        {'index':10,'name':'Terrier','speed':121,'speed2':60,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':1000,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':True,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.02,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':120},
        {'index':11,'name':'Doinger','speed':121,'speed2':60,'curve':0,'curve2':0,'hits':1,'firetype':4,'score':1000,'death_sound':'explosn',
         'fires':True,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.005,'changedir':0.003,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':128},
        {'index':12,'name':'Snipe','speed':131,'speed2':65,'curve':0,'curve2':0,'hits':1,'firetype':5,'score':1250,'death_sound':'explosn',
         'fires':True,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.004,'changedir':0.004,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':26},
        {'index':13,'name':'Tribbler','speed':100,'speed2':50,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':1500,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':True,'repulses':False,
         'fireprob':0.0,'changedir':0.01,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':32},
        {'index':14,'name':'Tribble','speed':220,'speed2':110,'curve':1000,'curve2':500,'hits':1,'firetype':0,'score':500,'death_sound':'explosn',
         'fires':False,'follows':False,'curves':True,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.005,'changecurve':0.1,'follow':0.0,'numframes':3,'framespeed':48},
        {'index':15,'name':'Buckshot','speed':101,'speed2':50,'curve':0,'curve2':0,'hits':1,'firetype':2,'score':1500,'death_sound':'explosn',
         'fires':True,'follows':False,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.03,'changedir':0.006,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':36},
        {'index':16,'name':'Cluster','speed':81,'speed2':40,'curve':0,'curve2':0,'hits':1,'firetype':6,'score':5000,'death_sound':'retaliate',
         'fires':False,'follows':False,'curves':False,'explodes':True,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.02,'changecurve':0.0,'follow':0.0,'numframes':3,'framespeed':32},
        {'index':17,'name':'Sticktight','speed':101,'speed2':50,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':2000,'death_sound':'explosn',
         'fires':False,'follows':True,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':False,
         'fireprob':0.0,'changedir':0.0,'changecurve':0.0,'follow':1.0,'numframes':3,'framespeed':40},
        {'index':18,'name':'Repulsor','speed':141,'speed2':70,'curve':0,'curve2':0,'hits':1,'firetype':0,'score':7500,'death_sound':'explosn',
         'fires':False,'follows':True,'curves':False,'explodes':False,'laysmines':False,'shootback':False,'zoom':False,'maxspeed':False,'rebounds':False,'tribbles':False,'repulses':True,
         'fireprob':0.0,'changedir':0.01,'changecurve':0.0,'follow':0.01,'numframes':5,'framespeed':60},
    ]

    # --- Missile kinds (from InitialiseVariables in xqinit.pas) ---
    missile_kinds = [
        {'index':1,'mspeed':120,'sound':'fire6','rebound':False,'firedirect':False},
        {'index':2,'mspeed':150,'sound':'fire5','rebound':False,'firedirect':False},
        {'index':3,'mspeed':200,'sound':'retaliate','rebound':False,'firedirect':True},
        {'index':4,'mspeed':150,'sound':'none','rebound':True,'firedirect':False},
        {'index':5,'mspeed':150,'sound':'fire4','rebound':False,'firedirect':True},
        {'index':6,'mspeed':170,'sound':'none','rebound':False,'firedirect':False},
    ]

    # --- Levels (from xqvars.pas) ---
    levels_raw = [
        (15,0,1,0.2,15000,20,0.005,27,0,0.0,20,False),
        (16,3,1,0.2,15000,4,0.005,27,0,0.0,20,False),
        (17,4,1,0.2,15000,5,0.005,26,0,0.0,20,False),
        (18,5,1,0.2,15000,6,0.005,26,0,0.0,25,False),
        (19,6,1,0.2,15000,7,0.005,25,0,0.0,25,False),
        (20,6,1,0.2,15000,8,0.005,25,0,0.0,30,False),
        (21,7,1,0.2,15000,9,0.005,24,0,0.0,30,False),
        (22,7,1,0.2,20000,10,0.006,24,0,0.0,35,False),
        (23,8,1,0.2,20000,10,0.006,23,0,0.0,35,False),
        (24,8,1,0.2,20000,10,0.006,23,0,0.0,40,False),
        (24,9,2,0.2,20000,10,0.006,22,0,0.0,40,False),
        (25,9,2,0.2,20000,10,0.007,22,0,0.0,45,False),
        (25,10,2,0.2,40000,10,0.007,21,0,0.0,45,False),
        (26,10,2,0.2,40000,10,0.008,21,0,0.0,45,False),
        (26,10,2,0.2,40000,10,0.008,20,0,0.0,50,False),
        (27,11,2,0.2,40000,10,0.009,20,0,0.0,50,False),
        (27,11,2,0.2,40000,10,0.009,19,0,0.0,50,False),
        (28,11,2,0.2,40000,10,0.01,19,0,0.0,55,False),
        (28,12,2,0.2,40000,10,0.01,18,0,0.0,55,False),
        (29,12,2,0.3,40000,10,0.01,18,0,0.0,55,False),
        (29,12,2,0.3,70000,10,0.01,17,0,0.0,60,False),
        (30,13,2,0.3,70000,10,0.01,17,0,0.0,60,False),
        (30,13,2,0.3,70000,10,0.01,17,0,0.0,60,False),
        (31,13,2,0.3,70000,10,0.01,17,0,0.0,60,False),
        (31,13,2,0.3,70000,10,0.01,17,0,0.0,65,False),
        (32,14,2,0.3,70000,10,0.01,17,0,0.0,65,False),
        (32,14,2,0.3,70000,11,0.01,17,0,0.0,65,False),
        (33,14,2,0.3,70000,11,0.01,17,0,0.0,65,False),
        (33,14,2,0.3,70000,12,0.01,17,0,0.0,70,False),
        (34,15,2,0.3,70000,12,0.01,17,0,0.0,70,False),
        (34,15,2,0.3,70000,13,0.01,17,0,0.0,70,False),
        (35,15,2,0.3,70000,13,0.01,17,0,0.0,70,False),
        (35,15,2,0.3,100000,14,0.01,17,10,0.0,75,False),
        (36,16,2,0.3,100000,14,0.01,17,20,0.0,75,False),
        (36,16,2,0.3,100000,15,0.01,17,30,0.0,75,False),
        (37,16,2,0.3,100000,15,0.01,17,40,0.0,75,False),
        (37,16,2,0.3,100000,16,0.025,17,50,0.0,80,False),
        (38,17,2,0.3,100000,16,0.011,17,60,0.0,80,False),
        (38,17,2,0.3,100000,17,0.012,17,70,0.0,80,False),
        (39,17,2,0.3,100000,17,0.012,17,80,0.0,80,False),
        (39,17,2,0.3,100000,18,0.013,17,90,0.002,85,False),
        (40,18,2,0.3,100000,18,0.013,17,100,0.004,85,False),
        (40,18,2,0.3,100000,19,0.014,17,100,0.006,85,False),
        (40,18,2,0.3,100000,19,0.014,17,100,0.008,85,False),
        (40,18,2,0.3,100000,20,0.015,17,100,0.010,90,False),
        (40,19,2,0.3,100000,20,0.016,17,100,0.012,90,False),
        (40,19,2,0.3,100000,20,0.016,17,100,0.014,90,False),
        (40,19,2,0.3,100000,20,0.017,17,100,0.016,90,False),
        (40,19,2,0.3,100000,20,0.017,17,100,0.018,90,False),
        (40,20,2,0.3,100000,20,0.018,17,100,0.02,90,False),
    ]
    levels = []
    for i, l in enumerate(levels_raw):
        levels.append({
            'level': i+1,
            'num_crystals': l[0],
            'num_mines': l[1],
            'max_smart_bombs': l[2],
            'smart_bomb_prob': l[3],
            'new_man_score': l[4],
            'max_enemies': l[5],
            'enemy_release_rate': l[6],
            'gate_width': l[7],
            'gate_move': l[8],
            'gate_change_dir_prob': l[9],
            'par_time_sec': l[10],
            'bonus_level': l[11],
        })

    # --- Level enemy probability tables (probs[level][0..18]) ---
    probs = [
        [5,0,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [5,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [5,0,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [5,0,15,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [7,0,0,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [7,0,15,15,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [7,0,0,0,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [7,0,15,15,15,55,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [7,0,0,0,0,0,0,100,0,0,0,0,0,0,0,0,0,0,0],
        [7,0,15,15,15,15,0,50,0,0,0,0,0,0,0,0,0,0,0],
        [7,0,0,0,0,0,0,0,100,0,0,0,0,0,0,0,0,0,0],
        [7,0,10,10,10,10,0,10,60,0,0,0,0,0,0,0,0,0,0],
        [7,0,0,0,0,0,0,0,0,100,0,0,0,0,0,0,0,0,0],
        [7,0,10,10,10,10,0,10,3,60,0,0,0,0,0,0,0,0,0],
        [7,0,0,0,0,0,0,0,0,0,100,0,0,0,0,0,0,0,0],
        [7,0,10,10,10,10,0,10,3,3,60,0,0,0,0,0,0,0,0],
        [7,0,0,0,0,0,0,0,0,0,0,100,0,0,0,0,0,0,0],
        [7,0,10,10,10,10,0,10,10,3,3,60,0,0,0,0,0,0,0],
        [7,0,0,0,0,0,0,0,0,0,0,0,100,0,0,0,0,0,0],
        [10,0,10,10,10,10,0,10,10,5,3,3,60,0,0,0,0,0,0],
        [10,0,0,0,0,0,0,0,0,0,0,0,0,100,0,0,0,0,0],
        [10,0,10,10,10,10,0,10,10,10,5,3,3,60,0,0,0,0,0],
        [10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100,0,0,0],
        [10,0,10,10,10,10,0,10,10,10,5,5,3,3,0,60,0,0,0],
        [10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100,0,0],
        [10,0,10,10,10,10,0,10,10,10,10,5,5,3,0,3,60,0,0],
        [10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100,0],
        [10,0,10,10,10,10,0,10,10,10,10,5,5,5,0,3,3,60,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,5,5,0,3,3,50,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,5,5,40,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,30,0],
        [10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100],
        [10,0,10,10,10,10,0,10,10,10,10,5,5,5,0,3,3,3,60],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,3,3,3,40],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,3,20],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
        [10,0,0,100,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
        [10,0,0,0,0,0,0,50,0,0,0,0,0,0,0,0,0,50,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
        [10,0,0,0,0,0,0,0,50,0,0,0,0,50,0,0,0,0,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
        [10,0,0,0,0,0,0,0,0,50,0,50,0,0,0,0,0,0,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
        [10,0,0,0,0,0,0,0,0,0,50,0,50,0,0,0,0,0,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
        [10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,50,0,0],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
        [10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,50],
        [10,0,10,10,10,10,0,10,10,10,10,10,10,10,0,10,10,10,10],
    ]

    # --- Difficulty levels ---
    difficulty = [
        {'index':0,'name':'Wimp',   'rebound':True, 'speed_factor':0.7,'enemy_frequency':0.7},
        {'index':1,'name':'Timid',  'rebound':True, 'speed_factor':1.0,'enemy_frequency':1.0},
        {'index':2,'name':'Average','rebound':False,'speed_factor':1.0,'enemy_frequency':1.0},
        {'index':3,'name':'Tricky', 'rebound':False,'speed_factor':1.5,'enemy_frequency':1.2},
        {'index':4,'name':'Inhuman','rebound':False,'speed_factor':2.0,'enemy_frequency':1.5},
    ]

    # --- Power-up durations (in frames, at 67 fps) ---
    framerate = 67
    powerups = [
        {'name':'Shield',    'time_min_frames':10*framerate,'time_ran_frames':15*framerate},
        {'name':'AimedFire', 'time_min_frames':30*framerate,'time_ran_frames':60*framerate},
        {'name':'RapidFire', 'time_min_frames':60*framerate,'time_ran_frames':90*framerate},
        {'name':'MultiFire', 'time_min_frames':60*framerate,'time_ran_frames':90*framerate},
        {'name':'AssFire',   'time_min_frames':60*framerate,'time_ran_frames':90*framerate},
        {'name':'HeavyFire', 'time_min_frames':60*framerate,'time_ran_frames':90*framerate},
        {'name':'Bounce',    'time_min_frames':30*framerate,'time_ran_frames':60*framerate},
    ]

    # --- Sound names ---
    sound_names = {
        1:'fire6', 2:'fire5', 3:'phew', 4:'fire4', 5:'fire',
        6:'boing', 7:'squelch', 8:'woohoo', 9:'allright', 10:'ohyeah',
        11:'getcrystal', 12:'explosn', 13:'explosn2', 14:'explosn3',
        15:'retaliate', 16:'ow', 17:'countdown', 18:'gatesound',
        19:'sxtsmash', 20:'bark', 21:'applause', 22:'enemyent',
        23:'menuclick', 24:'doh', 25:'repulse',
    }

    # --- Game-clocked completion ranks ---
    clocked_ranks = [
        'XQUEST WARRIOR', 'XQUEST WARRIOR SUPREME',
        'XQUEST COMMANDER', 'XQUEST WARLORD', 'XQUEST GOD',
    ]

    # --- Smart bomb flash palette ---
    smart_bomb_palette = [
        {'r6':0,'g6':0,'b6':0},
        {'r6':8,'g6':0,'b6':0},
        {'r6':14,'g6':0,'b6':0},
        {'r6':20,'g6':4,'b6':0},
        {'r6':26,'g6':12,'b6':0},
        {'r6':32,'g6':20,'b6':10},
        {'r6':38,'g6':28,'b6':20},
        {'r6':44,'g6':36,'b6':30},
        {'r6':50,'g6':44,'b6':40},
        {'r6':56,'g6':52,'b6':50},
        {'r6':63,'g6':63,'b6':63},
    ]

    # --- Font ASCII map (maps ASCII char index to font glyph index 1..40) ---
    fontmap_raw = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                   0,39,0,0,0,0,0,0,0,0,0,0,37,0,0,0,0,1,2,3,4,5,6,7,8,9,10,38,0,
                   0,0,0,40,0,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
                   28,29,30,31,32,33,34,35,36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    fontmap = {chr(i+1): fontmap_raw[i] for i in range(len(fontmap_raw)) if fontmap_raw[i] != 0}

    # --- Game constants ---
    constants = {
        'max_enemies': 40,
        'max_missiles': 50,
        'max_enemy_missiles': 70,
        'max_enemy_mines': 60,
        'max_enemy_frames': 5,
        'max_enemy_kinds': 18,
        'max_levels': 50,
        'max_ship_pics': 24,
        'max_objects': 65,
        'max_sounds': 25,
        'max_missile_kinds': 6,
        'max_diff_level': 5,
        'max_game_clocked': 5,
        'framerate': 67,
        'ticks_per_second': 180,
        'page_width': 392,
        'page_height': 320,
        'physical_page_width': 320,
        'split_screen_line': 217,
        'max_sprite_width': 24,
        'max_sprite_height': 24,
        'max_font_entries': 40,
        'font_entry_size': 116,
        'start_bombs': 3,
        'start_lives': 3,
        'start_level': 1,
        'max_ship_speed': 640,
        'base_game_speed': 64,
        'max_demo_frames': 13100,
        'missile_life': 300,
    }

    # --- Title screen palette (from titlemap.inc, used with title.pbm) ---
    # Format: array[0..769]: [start_index, num_entries, R,G,B, R,G,B, ...]
    # start=0, num=255 — full 255-entry palette for the menu/title screen
    title_palette_raw = [
        0,255,
        0,0,0,32,0,0,11,1,1,31,28,29,0,0,21,4,4,4,10,10,10,0,20,0,
        35,52,37,52,2,1,13,4,21,55,16,8,4,4,8,11,6,34,21,0,0,0,0,6,
        45,31,31,10,15,11,31,13,12,0,41,0,11,35,24,5,8,8,0,0,34,3,7,3,
        55,20,12,57,58,60,40,38,39,52,13,5,0,0,4,14,39,16,21,8,6,43,1,0,
        15,17,4,0,0,10,12,5,10,0,12,0,4,4,10,34,34,38,16,0,0,0,0,28,
        19,53,21,61,5,0,42,44,45,31,17,16,1,13,10,21,11,6,13,13,48,0,28,0,
        16,16,20,2,2,2,16,15,15,0,0,40,51,51,51,17,9,10,53,21,17,32,6,2,
        0,61,0,2,2,4,0,0,8,2,9,22,21,5,3,0,9,14,14,5,5,61,5,5,
        55,46,46,4,10,4,0,0,15,21,39,5,10,4,4,46,13,7,4,4,12,4,4,6,
        62,18,8,20,20,21,10,10,12,44,6,1,62,12,2,33,33,49,46,40,41,54,54,55,
        27,28,30,16,20,39,40,18,20,23,15,16,12,12,56,6,6,8,52,7,0,26,8,6,
        6,6,42,6,6,6,1,23,9,54,25,22,0,0,2,31,32,34,13,14,14,12,12,14,
        0,0,25,45,46,50,62,10,0,0,33,0,7,8,12,6,6,12,36,6,1,60,32,29,
        62,62,62,0,0,32,2,2,6,37,0,0,24,6,4,37,38,39,11,11,44,62,12,4,
        13,12,21,10,10,16,15,8,6,0,25,0,23,24,25,1,15,8,3,3,35,6,0,0,
        41,42,46,28,10,8,52,9,6,47,0,1,13,26,3,2,2,10,24,10,11,62,24,14,
        11,52,12,10,6,6,2,2,8,5,15,5,39,26,4,22,21,23,1,49,1,54,18,10,
        14,14,16,46,22,3,24,24,58,8,6,6,49,50,54,24,8,6,43,26,24,49,8,1,
        0,17,1,26,26,27,5,4,28,17,18,18,6,6,10,62,24,16,17,5,4,32,34,36,
        31,23,23,8,8,10,0,0,45,26,0,0,6,56,6,31,8,7,7,4,4,2,2,24,
        47,46,46,4,19,21,10,1,5,62,16,6,17,17,49,55,55,58,14,11,29,62,21,12,
        62,14,4,26,52,28,38,13,10,26,10,8,7,0,4,62,8,0,12,6,6,4,6,6,
        2,3,20,47,47,52,3,4,15,0,0,36,12,11,16,62,19,10,57,7,0,55,40,40,
        26,26,40,5,5,49,61,9,8,43,51,47,40,40,43,48,4,0,21,5,6,6,6,14,
        46,17,11,57,2,2,18,7,16,15,1,3,12,12,12,8,8,8,0,37,0,26,4,3,
        18,18,20,30,30,33,20,11,39,61,37,33,37,8,5,25,20,21,36,36,40,2,28,3,
        16,16,56,0,0,18,6,24,1,30,30,60,62,29,25,57,13,5,2,21,2,31,3,1,
        16,16,18,37,38,42,59,60,61,46,20,18,16,6,6,0,0,30,10,37,4,1,15,15,
        60,50,48,6,10,6,6,9,36,62,16,8,38,2,0,42,42,62,60,55,54,62,26,16,
        5,48,5,6,0,6,10,10,14,5,5,32,18,8,6,43,44,48,8,8,14,11,4,26,
        14,14,45,61,44,42,44,8,5,6,8,6,1,12,3,56,7,7,11,12,37,16,54,18,
        62,10,2,20,20,24,3,42,4,49,8,4,34,34,35,52,51,56,27,15,15,7,13,8,
    ]
    title_palette_start = title_palette_raw[0]
    title_palette_count = title_palette_raw[1]
    title_palette_data = title_palette_raw[2:]
    title_palette = []
    for i in range(title_palette_count):
        r6, g6, b6 = title_palette_data[i*3], title_palette_data[i*3+1], title_palette_data[i*3+2]
        title_palette.append({
            'index': title_palette_start + i,
            'r6': r6, 'g6': g6, 'b6': b6,
            'r': vga_to_rgb(r6), 'g': vga_to_rgb(g6), 'b': vga_to_rgb(b6),
        })

    # --- Title logo palette (from titlmap0.inc, used with title0.gfx/.pbm) ---
    # 32 entries starting at VGA index 224 — mostly red-channel values for the logo
    title0_palette_raw = [
        224, 32,
        5,0,0,  19,0,0,  36,0,0,  10,0,0,  25,0,0,  31,0,0,  20,0,0,  16,0,0,
        3,0,0,  35,0,0,  44,0,0,  45,0,0,  18,0,0,   8,0,0,  28,0,0,   4,0,0,
        15,0,0, 34,0,0,  23,0,0,   1,0,0,  41,0,0,   7,0,0,  13,0,0,  30,0,0,
        32,0,0, 12,0,0,  27,0,0,  29,0,0,  39,0,0,  22,0,0,   9,0,0,  24,0,0,
    ]
    t0_start = title0_palette_raw[0]
    t0_count = title0_palette_raw[1]
    t0_data = title0_palette_raw[2:]
    title0_palette = []
    for i in range(t0_count):
        r6, g6, b6 = t0_data[i*3], t0_data[i*3+1], t0_data[i*3+2]
        title0_palette.append({
            'index': t0_start + i,
            'r6': r6, 'g6': g6, 'b6': b6,
            'r': vga_to_rgb(r6), 'g': vga_to_rgb(g6), 'b': vga_to_rgb(b6),
        })

    # --- Starfield parameters (from starunit.pas) ---
    starfield = {
        'max_stars': 400,
        'speed': 128,
        'screen_width': 320,
        'screen_height': 240,
        'center_x': 160,
        'center_y': 120,
        'init_x_range': [-5000, 5000],
        'init_y_range': [-5000, 5000],
        'init_z_range': [256, 12255],
        'new_x_range': [-8191, 8191],
        'new_y_range': [-8191, 8191],
        'new_z_range': [14500, 15755],
        'note': 'Stars rendered procedurally; no static asset. Parameters from starunit.pas.',
    }

    # --- Music note ---
    music_note = {
        'music': None,
        'note': (
            'XQuest has no music. AdLib FM synthesis code was written '
            '(16 instrument definitions visible in xqinit.pas lines 329-370) '
            'but was commented out before release. The 25 PCM sound effects '
            'in sounds.json are the complete audio content of the game.'
        ),
    }

    data = {
        'palette': palette,
        'title_palette': title_palette,
        'title_logo_palette': title0_palette,
        'enemy_kinds': enemy_kinds,
        'missile_kinds': missile_kinds,
        'levels': levels,
        'level_enemy_probs': probs,
        'difficulty': difficulty,
        'powerups': powerups,
        'sound_names': sound_names,
        'clocked_ranks': clocked_ranks,
        'smart_bomb_flash_palette': smart_bomb_palette,
        'font_ascii_map': fontmap,
        'constants': constants,
        'starfield': starfield,
        'music': music_note,
    }
    save('gamedata.json', data)

# ---------------------------------------------------------------------------
# 2. xquest.gfx  — sprites
# ---------------------------------------------------------------------------

def decode_gfx():
    print('Decoding xquest.gfx...')
    path = src('xquest.gfx')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        data = f.read()

    pos = 0
    out_data = {}

    def next_sprite(label=None):
        nonlocal pos
        sprite, pos = read_sprite(data, pos)
        return sprite

    # 24 ship frames
    ship_frames = [next_sprite() for _ in range(24)]
    out_data['ship_frames'] = ship_frames

    # 1 player missile
    out_data['player_missile'] = next_sprite()

    # 3 collectibles: crystal, mine, smart_bomb
    out_data['objects'] = {
        'crystal':    next_sprite(),
        'mine':       next_sprite(),
        'smart_bomb': next_sprite(),
    }

    # 1 enemy mine sprite
    out_data['enemy_mine'] = next_sprite()

    # Enemy sprites — numframes from xqenter.pas
    enemy_numframes = [5,5,3,3,3,3,0,3,5,3,3,3,3,3,3,3,3,3,5]
    enemy_sprites = []
    for i, nf in enumerate(enemy_numframes):
        frames = [next_sprite() for _ in range(nf + 1)]
        enemy_sprites.append({'enemy_index': i, 'frames': frames})
    out_data['enemy_sprites'] = enemy_sprites

    # 6 enemy missile kinds
    emissile_sprites = [next_sprite() for _ in range(6)]
    out_data['enemy_missile_sprites'] = emissile_sprites

    # PBM-format images (same on-disk format)
    out_data['ship_icon']     = next_sprite()
    out_data['smart_bomb_hud']= next_sprite()
    out_data['crystal_hud']   = next_sprite()

    powerup_names = ['shield','aimed_fire','rapid_fire','multi_fire','ass_fire','heavy_fire','bounce']
    out_data['powerup_hud'] = {name: next_sprite() for name in powerup_names}

    out_data['gates'] = {
        'left':  next_sprite(),
        'right': next_sprite(),
    }

    out_data['corners'] = {
        'tl': next_sprite(),
        'tr': next_sprite(),
        'br': next_sprite(),
        'bl': next_sprite(),
    }

    out_data['enemy_gate_left']  = [next_sprite() for _ in range(6)]
    out_data['enemy_gate_right'] = [next_sprite() for _ in range(6)]
    out_data['attractor']        = next_sprite()

    # Small font digits 0-9
    out_data['small_font_digits'] = {str(d): next_sprite() for d in range(10)}

    print(f'  read {pos} of {len(data)} bytes ({len(data)-pos} remaining)')
    save('sprites.json', out_data)

# ---------------------------------------------------------------------------
# 3. xquest.fnt  — in-game display font (40 glyphs)
# ---------------------------------------------------------------------------

def decode_fnt():
    print('Decoding xquest.fnt...')
    path = src('xquest.fnt')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()

    ENTRY_SIZE = 116  # FontEntrySize from xqvars.pas
    MAX_ENTRIES = 40  # MaxFontEntries

    glyphs = []
    for i in range(MAX_ENTRIES):
        block = list(raw[i * ENTRY_SIZE : (i+1) * ENTRY_SIZE])
        if len(block) < ENTRY_SIZE:
            break
        # Pascal code does:
        #   p[2] := p[3]   (height = byte at offset 2, 1-indexed → raw[2])
        #   for j:=5 to 116 do p[j-2]:=p[j]  (shift pixel data)
        # So in 0-indexed terms:
        #   stride = block[0]   (bmwidth stored in byte 0)
        #   height = block[2]   (p[3] in 1-indexed)
        #   pixels start at block[4] (p[5] in 1-indexed, shifted to p[3] = index 2)
        stride = block[0]
        height = block[2]
        if stride == 0 or height == 0:
            glyphs.append({'index': i+1, 'stride': 0, 'height': 0, 'pixels': []})
            continue
        pixel_bytes = block[4 : 4 + stride * height]
        glyphs.append({
            'index': i+1,
            'stride': stride,
            'height': height,
            'pixels': pixel_bytes,
        })

    save('font.json', {'glyphs': glyphs})

# ---------------------------------------------------------------------------
# 4. xquest2.fnt  — Comix display font (variable glyphs)
# ---------------------------------------------------------------------------

def decode_fnt2():
    print('Decoding xquest2.fnt...')
    path = src('xquest2.fnt')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()

    pos = 0
    glyphs = []
    while pos < len(raw):
        if pos >= len(raw):
            break
        char_code = raw[pos]; pos += 1
        if pos + 4 > len(raw):
            break
        width, height = struct.unpack_from('<HH', raw, pos); pos += 4
        bmwidth = ((width - 1) // 4 + 1) * 4
        pixels = []
        for row in range(height):
            if pos + width > len(raw):
                break
            row_pixels = list(raw[pos : pos + width])
            # Pad to bmwidth
            row_pixels += [0] * (bmwidth - width)
            pixels.extend(row_pixels)
            pos += width
        glyphs.append({
            'char_code': char_code,
            'char': chr(char_code) if 32 <= char_code < 127 else None,
            'width': width,
            'height': height,
            'bmwidth': bmwidth,
            'pixels': pixels,
        })

    save('font2.json', {'glyphs': glyphs})

# ---------------------------------------------------------------------------
# 5. xquest.snd  — 25 raw PCM samples (8-bit, 11025 Hz, mono)
# ---------------------------------------------------------------------------

def decode_snd():
    print('Decoding xquest.snd...')
    path = src('xquest.snd')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()

    sound_names = {
        1:'fire6', 2:'fire5', 3:'phew', 4:'fire4', 5:'fire',
        6:'boing', 7:'squelch', 8:'woohoo', 9:'allright', 10:'ohyeah',
        11:'getcrystal', 12:'explosn', 13:'explosn2', 14:'explosn3',
        15:'retaliate', 16:'ow', 17:'countdown', 18:'gatesound',
        19:'sxtsmash', 20:'bark', 21:'applause', 22:'enemyent',
        23:'menuclick', 24:'doh', 25:'repulse',
    }

    pos = 0
    sounds = []
    for i in range(1, 26):
        length = struct.unpack_from('<H', raw, pos)[0]; pos += 2
        pcm = raw[pos : pos + length]; pos += length
        sounds.append({
            'index': i,
            'name': sound_names.get(i, f'sound_{i}'),
            'length_bytes': length,
            'sample_rate_hz': 11025,
            'bits': 8,
            'channels': 1,
            'encoding': 'unsigned_pcm',
            'data_base64': base64.b64encode(pcm).decode('ascii'),
        })
        print(f'    sound {i:2d} {sound_names.get(i,"?"):12s}  {length:6d} bytes  '
              f'({length/11025*1000:.0f} ms)')

    save('sounds.json', {
        'format': {'sample_rate_hz': 11025, 'bits': 8, 'channels': 1, 'encoding': 'unsigned_pcm'},
        'sounds': sounds,
    })

# ---------------------------------------------------------------------------
# 6. xquest.scr  — high scores (5 diff levels × 10 entries)
# ---------------------------------------------------------------------------

def decode_scr():
    print('Decoding xquest.scr...')
    path = src('xquest.scr')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()

    # scoretype: longint score (4), word level (2), string[20] name (21)
    # = 27 bytes per record; 50 records total (5 diff × 10 each)
    RECORD_SIZE = 27
    diff_names = ['Wimp', 'Timid', 'Average', 'Tricky', 'Inhuman']

    tables = []
    pos = 0
    for diff in range(5):
        entries = []
        for rank in range(10):
            if pos + RECORD_SIZE > len(raw):
                break
            score = struct.unpack_from('<l', raw, pos)[0]; pos += 4
            level = struct.unpack_from('<H', raw, pos)[0]; pos += 2
            # Pascal string[20]: first byte is length, then chars
            name_len = raw[pos]; pos += 1
            name_bytes = raw[pos : pos + 20]; pos += 20
            name = name_bytes[:name_len].decode('latin-1', errors='replace')
            entries.append({'rank': rank+1, 'score': score, 'level': level, 'name': name})
        tables.append({'difficulty': diff_names[diff], 'diff_index': diff, 'entries': entries})

    save('hiscores.json', {'tables': tables})

# ---------------------------------------------------------------------------
# 7. xquest.cfg  — text config file (parse as structured JSON)
# ---------------------------------------------------------------------------

def decode_cfg():
    print('Decoding xquest.cfg...')
    path = src('xquest.cfg')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'r', errors='replace') as f:
        lines = [l.rstrip('\n\r') for l in f.readlines()]

    def read_int(line):
        return int(line.split()[0])

    key_names = ['UpKey','DownKey','LeftKey','RightKey','UpLeftKey','UpRightKey',
                 'DownLeftKey','DownRightKey','BrakeKey','FireKey','SmartBombKey']
    input_device_names = ['Mouse', 'Joystick', 'Keyboard']

    idx = 0
    def nextline():
        nonlocal idx
        while idx < len(lines):
            l = lines[idx]; idx += 1
            return l
        return ''

    cfg = {}
    try:
        cfg['sound_volume']   = read_int(nextline())
        cfg['num_players']    = read_int(nextline())

        players = []
        for p in range(2):
            nextline()  # blank
            nextline()  # "Player One" / "Player Two"
            h = read_int(nextline())
            v = read_int(nextline())
            diff = read_int(nextline())
            dev = read_int(nextline())
            mfire = read_int(nextline())
            msmart = read_int(nextline())
            jfire = read_int(nextline())
            jsmart = read_int(nextline())
            key_line = nextline()
            keys = [int(k) for k in key_line.split()[:11]]
            players.append({
                'h_sensitivity': h, 'v_sensitivity': v,
                'difficulty': diff,
                'input_device': input_device_names[dev] if dev < 3 else dev,
                'mouse_fire': mfire, 'mouse_smart': msmart,
                'joy_fire': jfire, 'joy_smart': jsmart,
                'keys': dict(zip(key_names, keys)),
            })
        cfg['players'] = players

        nextline()  # blank
        joy_line = nextline()
        joy_vals = [int(x) for x in joy_line.split()[:8]]
        joy_fields = ['xmin','xcentremin','xcentremax','xmax','ymin','ycentremin','ycentremax','ymax']
        cfg['joystick_calibration'] = dict(zip(joy_fields, joy_vals))
        cfg['joystick_calibrated']  = bool(read_int(nextline()))
        cfg['sound_card']           = read_int(nextline())
        cfg['sb_addr']              = read_int(nextline())
        cfg['sb_irq']               = read_int(nextline())
        cfg['sb_dma']               = read_int(nextline())
        cfg['max_sound_effects']    = read_int(nextline())
    except Exception as e:
        cfg['_parse_error'] = str(e)

    save('config.json', cfg)

# ---------------------------------------------------------------------------
# 8. xquest.dmo  — demo recording
# ---------------------------------------------------------------------------

def decode_dmo():
    print('Decoding xquest.dmo...')
    path = src('xquest.dmo')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()

    pos = 0
    rand_seed = struct.unpack_from('<l', raw, pos)[0]; pos += 4
    # GameModeType enum: 0=OnePlayer, 1=TwoPlayer (stored in 2 bytes in TP6)
    game_mode_raw = struct.unpack_from('<H', raw, pos)[0]; pos += 2
    game_mode = 'TwoPlayer' if game_mode_raw else 'OnePlayer'

    # Skip PlayerInfoType (74 bytes = 2 players × 37 bytes each)
    PLAYER_INFO_SIZE = 74
    pos += PLAYER_INFO_SIZE

    # demorectype: delx (int16), dely (int16), but (byte) = 5 bytes
    FRAME_SIZE = 5
    frame_bytes = raw[pos:]
    num_frames = len(frame_bytes) // FRAME_SIZE
    frames = []
    for i in range(num_frames):
        offset = i * FRAME_SIZE
        delx, dely = struct.unpack_from('<hh', frame_bytes, offset)
        but = frame_bytes[offset + 4]
        frames.append({'delx': delx, 'dely': dely, 'buttons': but})

    save('demo.json', {
        'rand_seed': rand_seed,
        'game_mode': game_mode,
        'num_frames': num_frames,
        'duration_sec': round(num_frames / 67, 2),
        'frames': frames,
    })
    print(f'  {num_frames} frames ({num_frames/67:.1f} seconds)')

# ---------------------------------------------------------------------------
# Helper: decode XLib PBM format
# Header: 1 byte bmwidth (= ceil(width/4)), 1 byte height
# Data:   bmwidth * 4 * height bytes in planar Mode X format
#         (for each row: bmwidth bytes plane0, bmwidth bytes plane1,
#          bmwidth bytes plane2, bmwidth bytes plane3)
# ---------------------------------------------------------------------------

def decode_pbm_file(raw, offset=0):
    bmwidth = raw[offset]
    height = raw[offset + 1]
    width = bmwidth * 4
    data_size = bmwidth * 4 * height
    planar = raw[offset + 2 : offset + 2 + data_size]
    # Convert planar → linear palette-index array
    # Plane p holds column bytes where x % 4 == p
    pixels = [0] * (width * height)
    for row in range(height):
        for plane in range(4):
            for col in range(bmwidth):
                byte_idx = row * (bmwidth * 4) + plane * bmwidth + col
                if byte_idx < len(planar):
                    x = col * 4 + plane
                    pixels[row * width + x] = planar[byte_idx]
    return {'width': width, 'height': height, 'pixels': pixels}, offset + 2 + data_size

# ---------------------------------------------------------------------------
# 9. title.pbm  — 320×240 title screen image (two 320×120 pages)
# ---------------------------------------------------------------------------

def decode_title_pbm():
    print('Decoding title.pbm...')
    path = src('title.pbm')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()
    # title.pbm contains two 320×120 PBM pages (double-buffer pages)
    # each: 2-byte header [bmwidth=80, height=120] + 80*4*120 = 38400 bytes
    page0, next_off = decode_pbm_file(raw, 0)
    page1, _ = decode_pbm_file(raw, next_off)
    # Combine into one 320×240 image
    full_pixels = page0['pixels'] + page1['pixels']
    print(f'  page0: {page0["width"]}×{page0["height"]}  page1: {page1["width"]}×{page1["height"]}')
    save('title_screen.json', {
        'width': page0['width'],
        'height': page0['height'] + page1['height'],
        'pixels': full_pixels,
        'note': 'Combined from two 320x120 double-buffer pages',
    })

# ---------------------------------------------------------------------------
# 9b. startpic.pbm  — 320×40 start/menu background strip
# ---------------------------------------------------------------------------

def decode_startpic_pbm():
    print('Decoding startpic.pbm...')
    path = src('startpic.pbm')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()
    image, _ = decode_pbm_file(raw, 0)
    print(f'  {image["width"]}×{image["height"]} pixels')
    save('startpic.json', image)

# ---------------------------------------------------------------------------
# 10. title0.gfx / title0.pbm — title logo graphic
# ---------------------------------------------------------------------------

def decode_title0():
    print('Decoding title0.gfx...')
    path = src('title0.gfx')
    if not os.path.exists(path):
        print(f'  SKIPPED: {path} not found')
        return
    with open(path, 'rb') as f:
        raw = f.read()
    sprite, _ = read_sprite(raw, 0)
    print(f'  {sprite["width"]}×{sprite["height"]} pixels')
    save('title_logo.json', sprite)

    print('Decoding title0.pbm...')
    path2 = src('title0.pbm')
    if os.path.exists(path2):
        with open(path2, 'rb') as f:
            raw2 = f.read()
        image, _ = decode_pbm_file(raw2, 0)
        print(f'  {image["width"]}×{image["height"]} pixels')
        save('title_logo_pbm.json', image)

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# 11. distrib/ factory defaults
# ---------------------------------------------------------------------------

def decode_distrib():
    distrib = os.path.join(SRC, 'distrib')

    # Factory config
    cfg_path = os.path.join(distrib, 'xquest.cfg')
    print('Decoding distrib/xquest.cfg (factory defaults)...')
    if os.path.exists(cfg_path):
        with open(cfg_path, 'r', errors='replace') as f:
            lines = [l.rstrip('\n\r') for l in f.readlines()]

        def read_int(line):
            return int(line.split()[0])

        key_names = ['UpKey','DownKey','LeftKey','RightKey','UpLeftKey','UpRightKey',
                     'DownLeftKey','DownRightKey','BrakeKey','FireKey','SmartBombKey']
        input_device_names = ['Mouse','Joystick','Keyboard']

        idx = 0
        def nextline():
            nonlocal idx
            while idx < len(lines):
                l = lines[idx]; idx += 1
                return l
            return ''

        cfg = {}
        try:
            cfg['sound_volume']   = read_int(nextline())
            cfg['num_players']    = read_int(nextline())
            players = []
            for p in range(2):
                nextline(); nextline()
                h = read_int(nextline()); v = read_int(nextline())
                diff = read_int(nextline()); dev = read_int(nextline())
                mfire = read_int(nextline()); msmart = read_int(nextline())
                jfire = read_int(nextline()); jsmart = read_int(nextline())
                key_line = nextline()
                keys = [int(k) for k in key_line.split()[:11]]
                players.append({
                    'h_sensitivity': h, 'v_sensitivity': v,
                    'difficulty': diff,
                    'input_device': input_device_names[dev] if dev < 3 else dev,
                    'mouse_fire': mfire, 'mouse_smart': msmart,
                    'joy_fire': jfire, 'joy_smart': jsmart,
                    'keys': dict(zip(key_names, keys)),
                })
            cfg['players'] = players
            nextline()
            joy_line = nextline()
            joy_vals = [int(x) for x in joy_line.split()[:8]]
            joy_fields = ['xmin','xcentremin','xcentremax','xmax','ymin','ycentremin','ycentremax','ymax']
            cfg['joystick_calibration'] = dict(zip(joy_fields, joy_vals))
            cfg['joystick_calibrated']  = bool(read_int(nextline()))
            cfg['sound_card']           = read_int(nextline())
            cfg['sb_addr']              = read_int(nextline())
            cfg['sb_irq']               = read_int(nextline())
            cfg['sb_dma']               = read_int(nextline())
            cfg['max_sound_effects']    = read_int(nextline())
        except Exception as e:
            cfg['_parse_error'] = str(e)
        save('config_defaults.json', cfg)

    # Factory high scores
    scr_path = os.path.join(distrib, 'xquest.scr')
    print('Decoding distrib/xquest.scr (factory high scores)...')
    if os.path.exists(scr_path):
        with open(scr_path, 'rb') as f:
            raw = f.read()
        RECORD_SIZE = 27
        diff_names = ['Wimp','Timid','Average','Tricky','Inhuman']
        tables = []
        pos = 0
        for diff in range(5):
            entries = []
            for rank in range(10):
                if pos + RECORD_SIZE > len(raw): break
                score = struct.unpack_from('<l', raw, pos)[0]; pos += 4
                level = struct.unpack_from('<H', raw, pos)[0]; pos += 2
                name_len = raw[pos]; pos += 1
                name_bytes = raw[pos : pos + 20]; pos += 20
                name = name_bytes[:name_len].decode('latin-1', errors='replace')
                entries.append({'rank': rank+1, 'score': score, 'level': level, 'name': name})
            tables.append({'difficulty': diff_names[diff], 'diff_index': diff, 'entries': entries})
        save('hiscores_defaults.json', {'tables': tables})


if __name__ == '__main__':
    print(f'Source: {os.path.abspath(SRC)}')
    print(f'Output: {os.path.abspath(OUT)}')
    print()
    decode_soft_data()
    decode_gfx()
    decode_fnt()
    decode_fnt2()
    decode_snd()
    decode_scr()
    decode_cfg()
    decode_dmo()
    decode_title_pbm()
    decode_startpic_pbm()
    decode_title0()
    decode_distrib()
    print()
    print('Done.')
