"""Stable JSON scene interchange for the ProGen3D anatomical arm prototype.

Exports a time-sampled scene for a future native importer. This does not add
native C++ parser support; schema versioning makes that integration explicit.
"""
import argparse
import json
import math
from pathlib import Path
from arm_milestone_a import landmarks
from coupled_animation import CoupledArm, angle_at

SCHEMA="progen3d.anatomical-arm.scene.v1"

def frame_record(arm, frame, fps):
    meshes=arm.meshes()
    return {"frame":frame,"time_s":frame/fps,"elbow_degrees":arm.angle,
            "landmarks_m":{"shoulder":landmarks(arm.angle)[0],
                           "elbow":landmarks(arm.angle)[1],
                           "wrist":landmarks(arm.angle)[2]},
            "objects":[{"name":name,"vertices_m":vertices,
                        "triangles":faces,"topology":"triangles"}
                       for name,(vertices,faces) in meshes.items()]}

def export_scene(out, frames=10, fps=30, contact="radial"):
    if frames<1 or fps<=0:raise ValueError("invalid animation sampling")
    if contact not in ("radial","triangle"):raise ValueError("invalid contact mode")
    arm=CoupledArm(contact_mode=contact)
    data={"schema":SCHEMA,"units":"m","coordinate_system":"Y-up",
          "simulation":{"type":"kinematic-xpbd-reference","fps":fps,
                        "contact":contact},
          "limitations":["not a native ProGen3D grammar",
                         "not a calibrated anatomical model",
                         "triangle contact uses local correspondence"],
          "frames":[]}
    for index in range(frames):
        angle=angle_at(index/fps)
        result=arm.step(angle,dt=1/fps)
        if not result["passed"]:
            raise RuntimeError(f"frame {index} failed clearance: {result}")
        data["frames"].append(frame_record(arm,index,fps))
    out=Path(out)
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(data,separators=(",",":"))+"\n",encoding="utf-8")
    return data

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--out",default="build/arm-scene.json")
    parser.add_argument("--frames",type=int,default=10)
    parser.add_argument("--fps",type=int,default=30)
    parser.add_argument("--contact",choices=("radial","triangle"),default="radial")
    args=parser.parse_args()
    result=export_scene(args.out,args.frames,args.fps,args.contact)
    print(json.dumps({"schema":result["schema"],"frames":len(result["frames"])}))
