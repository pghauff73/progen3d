"""Audit shoulder and hip junction gaps across an articulated pose sweep.

This is a diagnostic, not a watertightness or penetration proof. Every
collar rim vertex is sampled against the torso triangle surface.
"""
import argparse
import json
import math
from pathlib import Path
from whole_body import scene
from junction_proximity import audit_junctions

def sweep(steps=9,threshold_m=0.015):
    if not isinstance(steps,int) or isinstance(steps,bool) or steps<3:
        raise ValueError("steps must be an integer >= 3")
    if not math.isfinite(threshold_m) or threshold_m<0:
        raise ValueError("invalid gap threshold")
    records=[]
    worst=None
    for frame in range(steps):
        phase=frame/(steps-1)
        wave=math.sin(2*math.pi*phase)
        pose={"torso_lean":20*wave,"pelvis_tilt":12*wave,
              "shoulder_left":35+35*wave,
              "shoulder_right":20-30*wave,
              "elbow_left":70+60*(1-math.cos(2*math.pi*phase))/2,
              "elbow_right":45+50*(1-math.cos(2*math.pi*phase))/2,
              "hip_left":25*wave,"hip_right":-25*wave,
              "knee_left":70+60*(1-math.cos(2*math.pi*phase))/2,
              "knee_right":55+45*(1-math.cos(2*math.pi*phase))/2}
        report=audit_junctions(scene(**pose),threshold_m=threshold_m)
        for name,measurement in report["junctions"].items():
            record={"frame":frame,"phase":phase,"junction":name,
                    "gap_m":measurement["maximum_nearest_distance_m"],
                    "nearest_m":measurement["minimum_distance_m"],
                    "worst_rim_vertex":measurement["worst_sample_index"],
                    "within_gap_limit":measurement["within_gap_limit"]}
            records.append(record)
            if worst is None or record["gap_m"]>worst["gap_m"]:
                worst=record
    return {"schema":"progen3d.body-junction-sweep.v1",
            "steps":steps,"threshold_m":threshold_m,
            "passed":all(r["within_gap_limit"] for r in records),
            "worst_gap":worst,"records":records,
            "limitations":["unsigned distances cannot identify penetration",
                           "not a continuous triangle intersection test",
                           "gap threshold is provisional"]}

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--steps",type=int,default=9)
    p.add_argument("--threshold",type=float,default=0.015)
    p.add_argument("--out",default="build/body-junction-sweep.json")
    args=p.parse_args()
    result=sweep(args.steps,args.threshold)
    path=Path(args.out);path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({k:v for k,v in result.items() if k!="records"},indent=2))
    # Advisory metric: the topology is still open, so do not treat a gap
    # threshold violation alone as a failed geometry build.
