"""Full-cycle articulated body geometry and joint continuity audit.

Samples both flexion and extension, verifies invariant skeletal segment lengths,
checks mesh indices/finite coordinates, and reports topology per pose.
This is a kinematic validation, not a soft-tissue physics simulation.
"""
import argparse
import json
import math
from pathlib import Path
from body_skeleton import skeleton,segment_lengths
from whole_body import scene,validate
from body_render_layers import render_subset,boundary_audit

def cycle_pose(t):
    if not 0<=t<=1:raise ValueError("cycle fraction outside [0,1]")
    wave=0.5*(1-math.cos(2*math.pi*t))
    return {"elbow_left":145*wave,"elbow_right":120*wave,
            "knee_left":140*wave,"knee_right":115*wave,
            "shoulder_left":30*math.sin(2*math.pi*t),
            "shoulder_right":-25*math.sin(2*math.pi*t),
            "hip_left":25*math.sin(2*math.pi*t),
            "hip_right":-20*math.sin(2*math.pi*t),
            "torso_lean":12*math.sin(2*math.pi*t),
            "pelvis_tilt":8*math.sin(2*math.pi*t)}

def audit(frames=17,layer="skin"):
    if not isinstance(frames,int) or frames<3:
        raise ValueError("at least three frames required")
    records=[]
    topology=None
    for frame in range(frames):
        t=frame/(frames-1)
        pose=cycle_pose(t)
        joints=skeleton(**{k:pose[k] for k in
            ("torso_lean","pelvis_tilt","shoulder_left","shoulder_right",
             "hip_left","hip_right","knee_left","knee_right")})
        lengths=segment_lengths(joints)
        length_error=max(abs(length-0.43) for pair in lengths.values() for length in pair)
        model=render_subset(scene(**pose),layer)
        geometry=validate(model)
        signature={name:(len(v),len(f)) for name,(v,f) in model.items()}
        if topology is None:topology=signature
        stable_topology=signature==topology
        boundaries=boundary_audit(model)
        nonmanifold=sum(v["nonmanifold_edges"] for v in boundaries.values())
        passed=geometry["passed"] and length_error<1e-9 and stable_topology and nonmanifold==0
        records.append({"frame":frame,"phase":t,"pose":pose,
                        "passed":passed,"length_error_m":length_error,
                        "stable_topology":stable_topology,
                        "nonmanifold_edges":nonmanifold,
                        "open_boundary_edges":sum(v["boundary_edges"] for v in boundaries.values()),
                        "objects":len(model)})
    return {"schema":"progen3d.body-cycle-audit.v1","frames":frames,
            "render_layer":layer,"passed":all(r["passed"] for r in records),
            "max_segment_length_error_m":max(r["length_error_m"] for r in records),
            "max_nonmanifold_edges":max(r["nonmanifold_edges"] for r in records),
            "records":records,
            "limitations":["open shoulder/hip torso junctions remain",
                           "does not detect triangle-triangle self-intersection",
                           "does not simulate dynamic cloth or tissue stress"]}

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--frames",type=int,default=17)
    p.add_argument("--layer",choices=("skin","clothed"),default="skin")
    p.add_argument("--out",default="build/body-cycle-audit.json")
    args=p.parse_args()
    result=audit(args.frames,args.layer)
    path=Path(args.out);path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({k:v for k,v in result.items() if k!="records"},indent=2))
    raise SystemExit(0 if result["passed"] else 1)
