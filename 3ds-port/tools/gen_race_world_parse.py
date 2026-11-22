import os
os.makedirs('build',exist_ok=True)  # run from 3ds-port/: python3 tools/gen_race_world_parse.py && python3 tools/gen_race_world.py
import re,sys,json
hdrs=['bike_physics','race_frame','race_spawn','race_ai','race_script','race_countdown','race_sprites','race_course_stream','race_frame_control','native_race_session']
names={'obj_pos_Y_lo':0xB8,'btn_hold':0x14,'btn_press':0x15,'0052_track_finished_flag':0x52,'004F_race_started_flag':0x4F,
'obj_pos_X_lo':0xC8,'pause_timer':0x3B2,'pause_flag':0x3B1,'0012_scroll_X':0x12,'0050_scroll_X':0x50,'0046_flag':0x46,'03A2_useless':0x3A2,
'0003_t':0,'03A3_flag':0x3A3,'03A6_flag':0x3A6,'0032_timer':0x32,'003C_timer':0x3C,'003A_timer':0x3A,'0034_timer':0x34,'0024_timer':0x24,'003B_timer':0x3B,'0022_timer':0x22,'0023_timer':0x23,'004D_base_nametable':0x4D,'004E':0x4E,'0050':0x50,'00FE_flag':0xFE}
out={}
for h in hdrs:
    t=open('include/%s.h'%h).read()
    for m in re.finditer(r'typedef struct (\w+)\s*\{(.*?)\}\s*(\w+);',t,re.S):
        sname=m.group(3); fields=[]
        for line in m.group(2).split('\n'):
            fm=re.match(r'\s*(\w+)\s+(\w+)((?:\[\w+\])*)\s*;\s*(?:/\*(.*?)\*/)?',line)
            if not fm: continue
            ty,fn,dims,cm=fm.groups()
            addr=None
            if cm:
                a=re.search(r'ram_([0-9A-Fa-f]{4})',cm) or re.search(r'\$([0-9A-Fa-f]{4})',cm)
                if a: addr=int(a.group(1),16)
                else:
                    a=re.search(r'ram_(\w+)',cm)
                    if a and a.group(1).split('_obj')[0] in names: addr=names[a.group(1).split('_obj')[0]]
                    elif a and a.group(1) in names: addr=names[a.group(1)]
            fields.append((ty,fn,dims,addr,(cm or '').strip()))
        out[sname]=(h,fields)
json.dump(out,open('build/race_world_structs.json','w'),indent=0)
for s,(h,f) in out.items():
    bad=[x for x in f if x[3] is None]
    print(s,h,len(f),'unresolved:',[(x[1],x[2],x[4][:30]) for x in bad])
