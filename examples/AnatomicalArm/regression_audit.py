"""Deterministic regression harness for anatomical arm animation.

Runs the same prescribed motion twice, checks reproducibility and tracks
contact clearance, edge strain and frame-to-frame vertex displacement.
"""
import argparse
import json
import math
from pathlib import Path
from coupled_animation import CoupledArm, angle_at

def sample(frames=12,fps=30,contact="radial",iterations=8):
    if frames<2 or fps<=0:raise ValueError("invalid sampling")
    arm=CoupledArm(contact_mode=contact)
    records=[]
    previous=None
    for i in range(frames):
        result=arm.step(angle_at(i/fps),dt=1/fps,iterations=iterations)
        vertices=arm.meshes()["sleeve"][0]
        max_step=0 if previous is None else max(
            math.dist(a,b) for a,b in zip(vertices,previous))
        previous=vertices
        records.append({"frame":i,"angle":result["angle"],
                        "clearance_m":result["minimum_radial_gap_m"],
                        "max_edge_strain":result["maximum_edge_strain"],
                        "max_vertex_step_m":max_step,
                        "passed":result["passed"]})
    return records

def audit(frames=12,fps=30,contact="radial"):
    first=sample(frames,fps,contact)
    second=sample(frames,fps,contact)
    stable=first==second
    # Record a reproducible digest for comparing runs across machines.
    import hashlib
    digest=hashlib.sha256(json.dumps(first,sort_keys=True,separators=(",",":")).encode()).hexdigest()
    maximum_strain=max(r["max_edge_strain"] for r in first)
    max_step=max(r["max_vertex_step_m"] for r in first)
    min_gap=min(r["clearance_m"] for r in first)
    return {"schema":"progen3d.arm-regression.v1",
            "contact":contact,"frames":frames,"fps":fps,
            "deterministic":stable,"sha256":digest,
            "minimum_clearance_m":min_gap,
            "maximum_edge_strain":maximum_strain,
            "maximum_frame_displacement_m":max_step,
            "passed":stable and all(r["passed"] for r in first),
            "records":first,
            "limitations":["not a cloth constitutive validation",
                           "no global self-collision detection",
                           "no visual render comparison"]}

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--out",default="build/arm-regression.json")
    p.add_argument("--frames",type=int,default=12)
    p.add_argument("--contact",choices=("radial","triangle"),default="radial")
    args=p.parse_args()
    result=audit(args.frames,contact=args.contact)
    path=Path(args.out);path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({k:v for k,v in result.items() if k!="records"}))
    raise SystemExit(0 if result["passed"] else 1)
