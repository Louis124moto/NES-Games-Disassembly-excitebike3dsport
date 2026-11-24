import re,json,sys
S=json.load(open('build/race_world_structs.json'))
names={'obj_pos_Y_lo':0xB8,'btn_hold':0x14,'btn_press':0x15,'obj_pos_X_lo':0xC8}
def count(dims):
    n=1
    for d in re.findall(r'\[(\w+)\]',dims):
        n*=int(d)
    return n
# overrides: (struct, field) -> (addr, count)
OV={
('RaceOpponentCueState','cue_flag'):(0x3AA,3),
('RaceObstacleHitState','x_high_slot3'):(0x87,1),
('RaceObstacleHitState','rider_present_0373'):(0x373,1),
('RaceObstacleHitState','obstacle_y_006F'):(0x6F,1),
('RaceObstacleHitState','obstacle_x_0393'):(0x393,1),
('RaceObstacleHitState','lane_delta_slot3'):(0xDF,1),
('RaceObstacleHitState','state_039B'):(0x39B,1),
('RaceObstacleHitState','object_y_player'):(0xB8,1),
('RaceHideSpritesState','rider_present_0373'):(0x373,1),
('RaceHideSpritesState','slot_active_0088'):(0x88,4),
('RaceOffscreenSpriteState','slot_object_index'):(0x88,4),
('RaceFrameControlState','input_005C'):(0x5C,1),
('BikeTemperatureState','fraction'):(0x3B5,1),('BikeTemperatureState','whole'):(0x3B6,1),
('BikeTemperatureState','overheat_state'):(0x3E0,1),('BikeTemperatureState','overheat_timer'):(0x3C,1),
('BikeTemperatureState','sound_flags'):(0xFC,1),
('BikeRaceCountdownState','timer_003C'):(0x3C,1),('BikeRaceCountdownState','overheat_timer_03B6'):(0x3B6,1),
('RaceElapsedTimeState','decimal_work'):(0x3D1,7),('RaceElapsedTimeState','display'):(0x3D9,7),
('RaceScriptState','cue_tiles_03D9'):(0x3D9,7),
('RaceSpawnState','block_039C'):(0x39C,4),('RaceSpawnState','rng'):(0x18,8),('RaceSpawnState','track_pages'):(0x400,384),
('RaceCourseStreamState','buffer'):(0x300,0x12),('RaceFrameControlState','ppu_packet'):(0x312,10),
('RaceAiState','terrain_samples'):(0x3C0,16),
}
NESTED={'BikeMovementState':'movement','RaceSpawnState':'spawn','BikeStateMachineState':'states','NativeOamState':'oam'}
SPECIAL_OAM={('RaceStartBannerState','banner'):0x2C0,('RaceEffectSpriteState','sprite'):0x294}
def addr_of(sname,fn,cm,dims):
    if (sname,fn) in OV: return OV[(sname,fn)]
    n=count(dims)
    m=re.search(r'ram_([0-9A-Fa-f]{4})\w*\s*\+\s*\$([0-9A-Fa-f]{2})',cm or '')
    if m: return (int(m.group(1),16)+int(m.group(2),16),n)
    m=re.search(r'ram_([0-9A-Fa-f]{4})',cm or '') or re.search(r'\$([0-9A-Fa-f]{4})',cm or '')
    if m: return (int(m.group(1),16),n)
    m=re.search(r'ram_(\w+)',cm or '')
    if m:
        k=m.group(1).replace('_obj','')
        if k in names: return (names[k],n)
        m2={'0052_track_finished_flag':0x52,'004F_race_started_flag':0x4F,'0012_scroll_X':0x12,'pause_timer':0x3B2,'pause_flag':0x3B1,'0046_flag':0x46,'03A2_useless':0x3A2,'03A3_flag':0x3A3,'03A6_flag':0x3A6,'0032_timer':0x32,'003C_timer':0x3C,'003A_timer':0x3A,'0034_timer':0x34,'0024_timer':0x24,'003B_timer':0x3B,'0022_timer':0x22,'0023_timer':0x23,'004D_base_nametable':0x4D,'00FE_flag':0xFE}
        for kk,v in m2.items():
            if m.group(1).startswith(kk): return (v,n)
    return None
def flat(sname,prefix=''):
    """yield (cpath, addr, count) for struct sname"""
    out=[]
    for ty,fn,dims,_,cm in S[sname][1]:
        path=prefix+fn
        if ty in S and ty not in ('RaceSprite','NativeOamState'):
            out+=flat(ty,path+'.')
            continue
        if ty=='RaceSprite':
            out.append((path,SPECIAL_OAM[(sname,fn)] ,count(dims)*4)); continue
        if ty=='NativeOamState':
            out.append((path+'.bytes',0x200,256)); continue
        a=addr_of(sname,fn,cm,dims)
        if a is None:
            out.append((path,None,count(dims))); continue
        out.append((path,a[0],a[1]))
    return out
if __name__=='__main__':
    for s in sys.argv[1:]:
        print(s)
        for p,a,c in flat(s):
            print('  %-28s %s x%d'%(p,'??' if a is None else '%04X'%a,c))

USED=['BikeMovementState','BikeStateMachineState','BikeRaceCountdownState','RaceScriptState','RaceFrameControlState','RaceSpawnState',
'RaceOpponentCueState','RaceElapsedTimeState','RaceBannerSpriteState','RaceBikeContactState','RaceObstacleHitState','RaceCrashRecoveryState',
'RaceStartBannerState','RaceEffectSpriteState','RaceCourseStreamState','RaceCollisionState','RaceEngineSoundState','BikeTemperatureState',
'RaceOffscreenSpriteState','RacePositionSmoothingState','RaceAiState','RaceHideSpritesState']
EXTRA4=[0x90,0x94,0xA4,0x70,0xB8,0x36C,0x5C,0x374,0x98,0x3E0,0x9C,0xB0,0x58,0xAC,0x368,0x388,0x26,0x2A,0xA8,0xD4,0x8C,0xBC,0x360,0x84,0x80,0x3E4,0x398,0xDC,0x6C,0x60,0x64,0xE0,0xD8,0xC0,0xC4,0xC8,0xA0,0xB4,0xD0,0xE4,0x370,0x390,0x7C,0x88,0x364,0x3AA]
EXTRA1=[0x3A0,0x3A1,0x3B9,0x3BB,0x3BD,0x3BE,0x60,0x18,0x4C,0x4F,0x52,0xFD,0xFF,0xFB,0xFC,0xFE,0x34,0x3A9,0x3F2,0x3F7,0x3F1,0x14,0x15,0x3F,0x49,0x4A,0x42,0x47,0x41,0x43,0x46,0x3A8,0x3A6,0x3B3,0x3A3,0x3B0,0x3B1,0x3B2,0xEB,0x40,0x44,0x23,0x3B,0x3C,0x3A,0x32,0x24,0x22,0x12,0x4D,0x50,0x4E,0x3BF,0x3B4,0x3B5,0x3B6,0x57,0x68,0x69,0x6A,0x6B,0x52,
        # results phases (CB3B/CB77/CBA4/CC6B) and the saved best-time table
        0x45,0x4B,0x53,0x54,0x55,0x56,0x3F3,0x3F8]
def build():
    ivs=[]
    for s in USED:
        for p,a,c in flat(s):
            if a is None: continue
            ivs.append((a,a+c))
    for a in EXTRA4: ivs.append((a,a+4))
    for a in EXTRA1: ivs.append((a,a+1))
    ivs.append((0x300,0x340))  # PPU command buffer ($0300 length + commands)
    ivs.append((0x580,0x5C0))  # saved best-time records
    ivs.append((0x20,0x3F))  # sub_D310 timers $0020-$003E (NMI tick)
    ivs.sort()
    blocks=[]
    for a,b in ivs:
        if blocks and a<blocks[-1][1]:
            blocks[-1][1]=max(blocks[-1][1],b)
        else: blocks.append([a,b])
    return blocks

def emit(outdir):
    blocks=build()
    def blk(a,c):
        for x,y in blocks:
            if x<=a and a+c<=y: return x
        raise Exception('no block %X x%d'%(a,c))
    h=['#ifndef EXCITEBIKE_RACE_WORLD_H','#define EXCITEBIKE_RACE_WORLD_H','',
       '/* GENERATED by tools/gen_race_world.py - do not edit. Semantic race state: each block is a',
       ' * named group of gameplay bytes keyed by its source RAM label range (not a NES RAM image;',
       ' * unmodelled RAM does not exist here). */','#include <stdint.h>','',
       'typedef struct RaceWorld {']
    for x,y in blocks: h.append('    uint8_t b%04X[%d]; /* $%04X-$%04X */'%(x,y-x,x,y-1))
    h+=['} RaceWorld;','',
        '/* Block table for host tests: base source address, length, and byte pointer. */',
        'typedef struct RaceWorldBlock { uint16_t base; uint16_t length; } RaceWorldBlock;',
        'extern const RaceWorldBlock kRaceWorldBlocks[%d];'%len(blocks),'#define RACE_WORLD_BLOCK_COUNT %d'%len(blocks),
        'uint8_t *raceWorldByte(RaceWorld *world, unsigned source_address);','']
    for s in USED:
        h.append('struct %s;'%s); 
    h.append('')
    c=['/* GENERATED by tools/gen_race_world.py - do not edit. */','#include <string.h>','#include "race_world.h"',
       '#include "bike_physics.h"','#include "race_countdown.h"','#include "race_frame.h"','#include "race_frame_control.h"',
       '#include "race_spawn.h"','#include "race_ai.h"','#include "race_script.h"','#include "race_sprites.h"','#include "race_course_stream.h"','#include "race_world_glue.h"','']
    c.append('const RaceWorldBlock kRaceWorldBlocks[%d] = {'%len(blocks))
    c.append('    '+', '.join('{0x%04X,%d}'%(x,y-x) for x,y in blocks))
    c.append('};')
    c+=['','uint8_t *raceWorldByte(RaceWorld *w, unsigned a)','{']
    for x,y in blocks: c.append('    if (a >= 0x%04Xu && a < 0x%04Xu) return &w->b%04X[a - 0x%04Xu];'%(x,y,x,x))
    c+=['    return 0;','}','']
    g=['#ifndef EXCITEBIKE_RACE_WORLD_GLUE_H','#define EXCITEBIKE_RACE_WORLD_GLUE_H','#include "race_world.h"','#include "bike_physics.h"','#include "race_countdown.h"','#include "race_frame.h"','#include "race_frame_control.h"','#include "race_spawn.h"','#include "race_ai.h"','#include "race_script.h"','#include "race_sprites.h"','#include "race_course_stream.h"','']
    for s in USED:
        g.append('void raceWorldLoad_%s(%s *s, const RaceWorld *w);'%(s,s))
        g.append('void raceWorldStore_%s(const %s *s, RaceWorld *w);'%(s,s))
        L=['void raceWorldLoad_%s(%s *s, const RaceWorld *w)'%(s,s),'{']
        T=['void raceWorldStore_%s(const %s *s, RaceWorld *w)'%(s,s),'{']
        for p,a,n in flat(s):
            if a is None: continue
            if p=='gate_tile':
                for i in range(7):
                    b=blk(0x2C1+4*i,1)
                    L.append('    s->gate_tile[%d] = w->b%04X[0x%X];'%(i,b,0x2C1+4*i-b))
                    T.append('    w->b%04X[0x%X] = s->gate_tile[%d];'%(b,0x2C1+4*i-b,i))
                continue
            b=blk(a,n)
            L.append('    memcpy(&s->%s, &w->b%04X[%d], %d);'%(p,b,a-b,n))
            T.append('    memcpy(&w->b%04X[%d], &s->%s, %d);'%(b,a-b,p,n))
        L.append('}'); T.append('}')
        c+=L+['']+T+['']
    h.append('#endif'); g.append('#endif')
    open(outdir+'/include/race_world.h','w').write('\n'.join(h)+'\n')
    open(outdir+'/include/race_world_glue.h','w').write('\n'.join(g)+'\n')
    open(outdir+'/source/engine/race_world.c','w').write('\n'.join(c)+'\n')
    print(len(blocks),'blocks emitted')
if __name__=='__main__' and len(sys.argv)==1: emit('.')
