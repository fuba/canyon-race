"""Summarize an actual run; raw key values are UE inputs, never synthetic controller evidence."""
import csv,json,sys
from pathlib import Path
import numpy as np
root=Path(sys.argv[1]) if len(sys.argv)>1 else max((Path(__file__).resolve().parents[1]/'build/Windows/CanyonRace/Saved/Telemetry').iterdir(),key=lambda p:p.stat().st_mtime)
def rows(name):
    with (root/name).open(encoding='utf-8-sig') as f:
        return [r for r in csv.DictReader(f) if None not in r and all(v is not None for v in r.values())]
p=rows('poses.csv');f=rows('frames.csv');i=rows('inputs.csv')
def bounds(key):
    v=[float(r[key]) for r in p if r.get(key)]
    return {'min':min(v),'max':max(v)} if v else None
def timing(key):
    v=np.array([float(r[key]) for r in f if float(r['t_s'])>5 and r.get(key)],dtype=float)
    return {'mean_ms':float(v.mean()),'p95_ms':float(np.percentile(v,95)),'p99_ms':float(np.percentile(v,99))} if len(v) else None
active={}
for r in i:
    v=float(r['value'])
    if abs(v)>.02:
        a=active.setdefault(r['key'],{'events':0,'min':v,'max':v});a['events']+=1;a['min']=min(a['min'],v);a['max']=max(a['max'],v)
result={'session':str(root.resolve()),'duration_s':float(f[-1]['t_s']) if f else 0,'frames':len(f),'missing_frame_rows':int(f[-1]['frame'])-int(f[0]['frame'])+1-len(f) if f else 0,'pose_samples':len(p),
 'profiles':{side:sorted({r[side+'_profile'] for r in p}) for side in ['left','right']},
 'state_counts':{k:{v:sum(r[k]==v for r in p) for v in sorted({r[k] for r in p})} for k in ['hmd_enabled','hmd_tracked','has_vr_focus','seat_calibrated','left_tracking','right_tracking']},
 'ranges':{k:bounds(k) for k in ['throttle','steer','brake','pitch_input','pawn_pitch','pitch_rate_deg_s','left_reverse','right_reverse','thrust_cm_s2','contact_hits','penetration_attempts','requested_move_cm','actual_move_cm','blocked_s','lift','left_airbrake','right_airbrake','lift_cm_s2','bank_deg','yaw_rate_deg_s','bank_accel_cm_s2','speed_cm_s','camera_vs_nose_deg','hp','gate']},
 'timings':{k:timing(k) for k in ['wall_ms','game_thread_ms','render_thread_ms','rhi_thread_ms','gpu_ms']},'active_raw_inputs':active}
(root/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
