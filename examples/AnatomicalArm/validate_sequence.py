"""Milestone A validation and reproducible pose-sequence OBJ export."""
import argparse
import json
import math
from pathlib import Path
from arm_milestone_a import UPPER, FOREARM, RINGS, SIDES, landmarks, parts, write_obj

ANGLES = (0, 30, 60, 90, 120, 145)

def triangle_area(a,b,c):
    u=[b[i]-a[i] for i in range(3)]
    v=[c[i]-a[i] for i in range(3)]
    w=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    return 0.5*math.sqrt(sum(x*x for x in w))

def validate_pose(angle):
    shoulder,elbow,wrist=landmarks(angle)
    geometries=parts(angle)
    errors=[]
    if abs(math.dist(shoulder,elbow)-UPPER)>1e-9: errors.append("upper bone length")
    if abs(math.dist(elbow,wrist)-FOREARM)>1e-9: errors.append("forearm bone length")
    if set(geometries)!={"skin","humerus","radius","ulna","biceps","triceps"}:
        errors.append("missing part")
    min_area=float("inf")
    for name,(vertices,faces) in geometries.items():
        if not vertices or not faces: errors.append(f"{name}: empty mesh"); continue
        if any(not all(math.isfinite(x) for x in v) for v in vertices):
            errors.append(f"{name}: nonfinite coordinate")
        if any(not all(0<=k<len(vertices) for k in f) for f in faces):
            errors.append(f"{name}: invalid face index"); continue
        for face in faces:
            min_area=min(min_area,triangle_area(*(vertices[k] for k in face)))
    skin_vertices,skin_faces=geometries["skin"]
    if len(skin_vertices)!=RINGS*SIDES or len(skin_faces)!=(RINGS-1)*SIDES*2:
        errors.append("skin topology")
    # Every interior skin ring must connect to adjacent rings without an open seam.
    for ring in range(RINGS-1):
        if not any(ring*SIDES<=v<(ring+1)*SIDES for f in skin_faces for v in f):
            errors.append(f"disconnected ring {ring}")
    if min_area<=1e-12: errors.append("degenerate triangle")
    return {"angle_degrees":angle,"wrist":list(wrist),
            "part_count":len(geometries),"skin_vertices":len(skin_vertices),
            "skin_triangles":len(skin_faces),"minimum_triangle_area_m2":min_area,
            "passed":not errors,"errors":errors}

def run(out,angles=ANGLES):
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    report={"schema":"progen3d.anatomical-arm.validation.v1","units":"m",
            "limitations":["kinematic reference only","no skin elasticity",
                           "no cloth solver","no collision test","no native grammar integration"],
            "poses":[]}
    for angle in angles:
        pose=validate_pose(angle)
        report["poses"].append(pose)
        if pose["passed"]:
            write_obj(out/f"arm_{angle:03d}.obj",parts(angle))
    report["passed"]=all(p["passed"] for p in report["poses"])
    (out/"validation.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    return report

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--out",default="build/anatomical-arm")
    args=p.parse_args()
    result=run(args.out)
    print(json.dumps({"passed":result["passed"],"poses":len(result["poses"]),"out":args.out}))
    raise SystemExit(0 if result["passed"] else 1)
