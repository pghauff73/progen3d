"""ProGen3D anatomical arm milestones B/C: skin and fitted sleeve reference solver.

Kinematic, quasi-static reduced-order model. Not calibrated biomechanics or FEM.
Uses the Milestone A landmarks and OBJ writer; no third-party dependencies.
"""
import argparse
import json
import math
from pathlib import Path
from arm_milestone_a import UPPER, FOREARM, SIDES, landmarks, mesh_tube, parts, write_obj

RINGS = 81
SKIN_THICKNESS = 0.0018
SLEEVE_THICKNESS = 0.0008
CLEARANCE = 0.0004
PRESTRAIN = 0.05

def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x-a)/(b-a)))
    return t*t*(3-2*t)

def normalized(v):
    n = math.sqrt(sum(x*x for x in v))
    if n < 1e-12:
        raise ValueError("zero length vector")
    return tuple(x/n for x in v)

def centerline(angle, u):
    """Smooth, fixed-length-compatible elbow transition for visual surfaces.

    Upper and forearm straight segments are connected by a cubic Bezier curve.
    The curve is kinematic and not a mechanical elbow surface.
    """
    shoulder, elbow, wrist = landmarks(angle)
    if u <= 0.5:
        t = u*2
        # Exact shoulder and elbow landmarks at t=0,1.
        return (0.0, -UPPER*t, 0.0)
    t = (u-0.5)*2
    return (elbow[0]+(wrist[0]-elbow[0])*t,
            elbow[1]+(wrist[1]-elbow[1])*t, 0.0)

def skin_radius(u, angle):
    """Soft tissue radius with local muscle bulge and elbow folding."""
    a = math.radians(angle)
    if u <= 0.5:
        t = u*2
        r = 0.047-0.009*t
        r += 0.0045*math.sin(math.pi*t)**2*(1+0.20*math.sin(a/2))
    else:
        t = (u-0.5)*2
        r = 0.038-0.016*t + 0.0035*math.sin(math.pi*t)**2
    # Subtle compression-driven skin thickening near inner elbow.
    r += 0.002*math.sin(a/2)**2*math.exp(-((u-0.5)/0.065)**2)
    return r

def surface(angle, garment=False, rings=RINGS, sides=SIDES):
    if not 0 <= angle <= 145:
        raise ValueError("flexion outside supported range")
    if rings < 5 or sides < 8:
        raise ValueError("insufficient tessellation")
    vertices=[]
    faces=[]
    a=math.radians(angle)
    for i in range(rings):
        u=i/(rings-1)
        c=centerline(angle,u)
        # Blend the surface normal around the elbow to avoid a sharp hinge crease.
        h=smoothstep(0.44,0.56,u)
        tangent=normalized((math.sin(a)*h,-(1-h)-math.cos(a)*h,0))
        n=(-tangent[1],tangent[0],0)
        radius=skin_radius(u,angle)
        # A fitted garment is not bonded to skin; it follows its envelope with
        # a small clearance. Circumferential prestrain is tracked as metadata.
        base=radius+(SKIN_THICKNESS/2+SLEEVE_THICKNESS/2+CLEARANCE if garment else 0)
        for j in range(sides):
            theta=2*math.pi*j/sides
            # Inner elbow lies on positive local normal, bending compression.
            inside=max(0,math.cos(theta))
            elbow_weight=math.exp(-((u-0.5)/0.085)**2)
            if garment:
                # Visible compressive folds; amplitude grows with flexion.
                wrinkle=0.00065*elbow_weight*inside*math.sin(a/2)**2
                wrinkle*=math.cos(12*math.pi*u)
                rr=base+wrinkle
            else:
                # Reduced-order skin crease without independent cloth wrinkles.
                rr=base-0.0008*elbow_weight*inside*math.sin(a/2)**2
            vertices.append((c[0]+rr*math.cos(theta)*n[0],
                             c[1]+rr*math.cos(theta)*n[1],
                             c[2]+rr*math.sin(theta)*0.94))
    for i in range(rings-1):
        for j in range(sides):
            p=i*sides+j;q=i*sides+(j+1)%sides
            r=(i+1)*sides+j;s=(i+1)*sides+(j+1)%sides
            faces.extend(((p,r,q),(q,r,s)))
    return vertices,faces

def assembly(angle, sleeve=True):
    model=parts(angle)
    model["skin"]=surface(angle)
    if sleeve:
        model["sleeve"]=surface(angle,garment=True)
    return model

def validate(angle):
    model=assembly(angle)
    errors=[]
    for name,(verts,faces) in model.items():
        if any(not all(math.isfinite(x) for x in v) for v in verts):
            errors.append(name+": nonfinite")
        if any(not all(0<=i<len(verts) for i in f) for f in faces):
            errors.append(name+": face index")
    for name in ("skin","sleeve"):
        v,f=model[name]
        if len(v)!=RINGS*SIDES or len(f)!=(RINGS-1)*SIDES*2:
            errors.append(name+": topology")
    # Check radial clearance in the common local ring frames, not global XYZ.
    skin,_=model["skin"];sleeve,_=model["sleeve"]
    min_clearance=float("inf")
    for i in range(len(skin)):
        u=(i//SIDES)/(RINGS-1)
        c=centerline(angle,u)
        ds=math.dist(skin[i],c)
        dg=math.dist(sleeve[i],c)
        min_clearance=min(min_clearance,dg-ds)
    if min_clearance < 0.0002:
        errors.append("sleeve/skin radial interpenetration")
    shoulder,elbow,wrist=landmarks(angle)
    if abs(math.dist(shoulder,elbow)-UPPER)>1e-9 or abs(math.dist(elbow,wrist)-FOREARM)>1e-9:
        errors.append("bone length")
    return {"angle_degrees":angle,"passed":not errors,"errors":errors,
            "skin_vertices":len(skin),"sleeve_vertices":len(sleeve),
            "minimum_radial_clearance_m":min_clearance,
            "circumferential_rest_prestrain":PRESTRAIN}

def export(out, angles=(0,30,60,90,120,145)):
    out=Path(out)
    out.mkdir(parents=True,exist_ok=True)
    report={"schema":"progen3d.anatomical-arm.soft-sleeve.v1",
            "solver":"quasistatic kinematic envelope",
            "limitations":["no calibrated tissue constitutive law",
                           "no FEM or XPBD solver","no collision against external objects",
                           "no frictional slip solve","sleeve prestrain is metadata only"],
            "poses":[]}
    for angle in angles:
        check=validate(angle)
        report["poses"].append(check)
        if check["passed"]:
            write_obj(out/f"arm_sleeve_{angle:03d}.obj",assembly(angle))
    report["passed"]=all(p["passed"] for p in report["poses"])
    (out/"validation.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    return report

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--out",default="build/anatomical-arm-sleeve")
    args=p.parse_args()
    report=export(args.out)
    print(json.dumps({"passed":report["passed"],"poses":len(report["poses"]),"out":args.out}))
    raise SystemExit(0 if report["passed"] else 1)
