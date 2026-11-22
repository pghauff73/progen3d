"""ProGen3D articulated whole-body reference, metres.

Extends the arm skin/sleeve envelope to bilateral shoulders, legs and torso.
Kinematic geometry only: not anatomical muscle attachment, FEM or self-contact.
"""
import argparse
import json
import math
from pathlib import Path
from arm_soft_sleeve import assembly as arm_assembly, surface as arm_surface
from arm_milestone_a import mesh_tube, write_obj
from body_skeleton import skeleton
from joint_collars import collar_from_ring
from surface_stitch import stitch_limb_collar
from body_render_layers import render_subset,check_duplicate_geometry,boundary_audit
from junction_proximity import audit_junctions

def add(a,b): return tuple(x+y for x,y in zip(a,b))
def scale(a,k): return tuple(k*x for x in a)
def rotate_z(p,angle):
    a=math.radians(angle); c=math.cos(a);s=math.sin(a)
    return (c*p[0]-s*p[1],s*p[0]+c*p[1],p[2])
def transform(mesh,origin,angle=0,mirror=False):
    vertices,faces=mesh
    out=[]
    for p in vertices:
        x,y,z=rotate_z(p,angle)
        if mirror:x=-x
        out.append(add(origin,(x,y,z)))
    # A reflection reverses orientation; reverse winding to preserve normals.
    return out,[(a,c,b) for a,b,c in faces] if mirror else list(faces)

def tube_between(a,b,r0,r1,rings=24,sides=20):
    d=tuple(b[k]-a[k] for k in range(3))
    if math.dist(a,b)<1e-9:raise ValueError("zero length limb")
    points=[add(a,scale(d,i/(rings-1))) for i in range(rings)]
    radii=[(r0+(r1-r0)*i/(rings-1),)*2 for i in range(rings)]
    return mesh_tube(points,radii,sides)

def torso_surface(rings=32,sides=32,garment=False):
    vertices=[];faces=[]
    for i in range(rings):
        u=i/(rings-1)
        y=0.05+0.57*u
        waist=0.17+0.045*math.sin(math.pi*u)**2
        chest=0.035*math.exp(-((u-0.78)/0.20)**2)
        rx=waist+chest+(0.006 if garment else 0)
        rz=0.105+0.035*math.sin(math.pi*u)**2+(0.006 if garment else 0)
        for j in range(sides):
            t=2*math.pi*j/sides
            vertices.append((rx*math.cos(t),y,rz*math.sin(t)))
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides
            c=(i+1)*sides+j;d=(i+1)*sides+(j+1)%sides
            faces.extend(((a,c,b),(b,c,d)))
    return vertices,faces

def leg_mesh(hip,knee,ankle,garment=False,rings=61,sides=20):
    """Continuous thigh-knee-calf envelope with smoothed knee tangent.

    A single indexed mesh avoids a hard topological seam at the knee.
    The bend zone is kinematic and is not a tissue constitutive model.
    """
    if rings<9 or sides<8:raise ValueError("insufficient leg tessellation")
    upper=tuple(knee[k]-hip[k] for k in range(3))
    lower=tuple(ankle[k]-knee[k] for k in range(3))
    upper_len=math.sqrt(sum(x*x for x in upper))
    lower_len=math.sqrt(sum(x*x for x in lower))
    if min(upper_len,lower_len)<1e-9:raise ValueError("degenerate leg segment")
    upper=tuple(x/upper_len for x in upper)
    lower=tuple(x/lower_len for x in lower)
    vertices=[];faces=[]
    for i in range(rings):
        u=i/(rings-1)
        if u<=0.5:
            t=u*2;p=add(hip,scale(tuple(knee[k]-hip[k] for k in range(3)),t))
            radius=0.086-0.029*t+0.012*math.sin(math.pi*t)**2
        else:
            t=(u-0.5)*2;p=add(knee,scale(tuple(ankle[k]-knee[k] for k in range(3)),t))
            radius=0.055-0.021*t+0.010*math.sin(math.pi*t)**2
        if garment:radius+=0.004
        # Smooth orientation in a narrow band while preserving segment endpoints.
        blend=max(0.0,min(1.0,(u-0.43)/0.14))
        blend=blend*blend*(3-2*blend)
        direction=tuple((1-blend)*upper[k]+blend*lower[k] for k in range(3))
        magnitude=math.sqrt(sum(x*x for x in direction))
        if magnitude<1e-8:
            # Extreme folds are not supported by this reduced-order envelope.
            direction=upper if u<=0.5 else lower
            magnitude=1.0
        tangent=tuple(x/magnitude for x in direction)
        normal=(-tangent[1],tangent[0],0)
        nlen=math.hypot(normal[0],normal[1])
        if nlen<1e-8:raise ValueError("leg tangent parallel to Z axis")
        normal=(normal[0]/nlen,normal[1]/nlen,0)
        for j in range(sides):
            theta=2*math.pi*j/sides
            vertices.append((p[0]+radius*math.cos(theta)*normal[0],
                             p[1]+radius*math.cos(theta)*normal[1],
                             p[2]+radius*0.94*math.sin(theta)))
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides
            c=(i+1)*sides+j;d=(i+1)*sides+(j+1)%sides
            faces.extend(((a,c,b),(b,c,d)))
    return vertices,faces

def scene(shoulder_left=20,shoulder_right=20,elbow_left=30,elbow_right=30,
          hip_left=0,hip_right=0,knee_left=15,knee_right=15,torso_lean=0,pelvis_tilt=0):
    values=(shoulder_left,shoulder_right,elbow_left,elbow_right,
            hip_left,hip_right,knee_left,knee_right,torso_lean,pelvis_tilt)
    if not all(math.isfinite(v) for v in values):raise ValueError("nonfinite pose")
    if not all(0<=v<=145 for v in (elbow_left,elbow_right)):raise ValueError("elbow limit")
    if not all(0<=v<=140 for v in (knee_left,knee_right)):raise ValueError("knee limit")
    if not all(-75<=v<=110 for v in (shoulder_left,shoulder_right,hip_left,hip_right)):
        raise ValueError("shoulder/hip limit")
    if not -35<=torso_lean<=35:raise ValueError("torso lean limit")
    if not -25<=pelvis_tilt<=25:raise ValueError("pelvis tilt limit")
    joints=skeleton(torso_lean,pelvis_tilt,shoulder_left,shoulder_right,
                    hip_left,hip_right,knee_left,knee_right)
    model={}
    def emit(name,mesh,origin=(0,0,0),angle=0,mirror=False):
        model[name]=transform(mesh,origin,angle,mirror)
    # Torso geometry is authored around the pelvis origin (y=0.05).
    # Rotate about that pivot rather than the world origin so the lower
    # torso stays attached to the articulated pelvis.
    torso_pivot=(0.0,0.05,0.0)
    def articulated_torso(mesh):
        vertices,faces=mesh
        root=joints["torso"]
        return ([root.apply(tuple(p[k]-torso_pivot[k] for k in range(3)))
                 for p in vertices],list(faces))
    model["torso_skin"]=articulated_torso(torso_surface())
    model["torso_garment"]=articulated_torso(torso_surface(garment=True))
    for side,mirror,shoulder,elbow,hip,knee in (
        ("left",True,shoulder_left,elbow_left,hip_left,knee_left),
        ("right",False,shoulder_right,elbow_right,hip_right,knee_right)):
        sign=-1 if mirror else 1
        shoulder_origin=joints[side+"_shoulder"].origin
        for part,mesh in arm_assembly(elbow).items():
            emit(side+"_arm_"+part,mesh,shoulder_origin,joints[side+"_shoulder"].angle,mirror)
        # Collars share the first limb ring exactly, but remain separate
        # surfaces at their torso/pelvis ends pending a watertight skin union.
        for layer in ("skin","sleeve"):
            limb=model[side+"_arm_"+layer]
            model[side+"_shoulder_"+("garment" if layer=="sleeve" else "skin")+"_collar"]=(
                collar_from_ring(limb,shoulder_origin,
                                 inward=(-sign*0.045,0.018,0),
                                 radius_scale=1.55))
        # Leg hip pivot is below torso; knee and ankle preserve segment lengths.
        hip_origin=joints[side+"_hip"].origin
        knee_point=joints[side+"_knee"].origin
        ankle=joints[side+"_ankle"].origin
        emit(side+"_leg_skin",leg_mesh(hip_origin,knee_point,ankle))
        emit(side+"_leg_garment",leg_mesh(hip_origin,knee_point,ankle,garment=True))
        for layer in ("skin","garment"):
            limb=model[side+"_leg_"+layer]
            model[side+"_hip_"+layer+"_collar"]=collar_from_ring(
                limb,hip_origin,inward=(-sign*0.04,0.035,0),
                radius_scale=1.4)
        emit(side+"_femur",tube_between(hip_origin,knee_point,0.019,0.015))
        emit(side+"_tibia",tube_between(knee_point,ankle,0.014,0.010))
        emit(side+"_shoulder_joint",tube_between(shoulder_origin,
             joints[side+"_shoulder"].apply((0,-0.045,0)),0.048,0.048))
    # Create welded alternatives while retaining the original components
    # for backwards compatibility with the gallery and diagnostic tools.
    for side in ("left","right"):
        for region,limb_layer,collar_layer in (
            ("shoulder","arm_skin","shoulder_skin_collar"),
            ("shoulder","arm_sleeve","shoulder_garment_collar"),
            ("hip","leg_skin","hip_skin_collar"),
            ("hip","leg_garment","hip_garment_collar")):
            key=side+"_"+region+"_"+("clothed" if "garment" in collar_layer else "skin")+"_patch"
            model[key]=stitch_limb_collar(model[side+"_"+limb_layer],
                                         model[side+"_"+collar_layer])
    return model

def validate(model):
    errors=[]
    for name,(vertices,faces) in model.items():
        if not vertices or not faces:errors.append(name+": empty")
        if any(not all(math.isfinite(x) for x in v) for v in vertices):
            errors.append(name+": nonfinite")
        if any(len(f)!=3 or any(i<0 or i>=len(vertices) for i in f) for f in faces):
            errors.append(name+": invalid faces")
    return {"passed":not errors,"errors":errors,"objects":len(model),
            "vertices":sum(len(v) for v,f in model.values()),
            "triangles":sum(len(f) for v,f in model.values()),
            "limitations":["kinematic tubes, not calibrated anatomy",
                           "no inter-limb collision or muscle force model",
                           "joint collars are not watertight torso/limb unions",
                           "garments are static envelopes, not coupled XPBD cloth"]}

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--out",default="build/anatomical-body")
    parser.add_argument("--render-layer",choices=("skin","clothed","diagnostic"),default="skin")
    parser.add_argument("--bridge-junctions",action="store_true",help="include provisional visual torso junction bridges")
    args=parser.parse_args()
    out=Path(args.out);out.mkdir(parents=True,exist_ok=True)
    model=scene()
    selected=render_subset(model,args.render_layer)
    bridge_reports={}
    if args.bridge_junctions:
        if args.render_layer=="diagnostic":
            parser.error("--bridge-junctions requires skin or clothed render layer")
        bridges,bridge_reports=make_bridges(model,args.render_layer)
        selected.update(bridges)
    report=validate(selected)
    report["render_layer"]=args.render_layer
    report["junction_bridges"]=bridge_reports
    report["duplicate_geometry"]=check_duplicate_geometry(selected)
    report["boundary_audit"]=boundary_audit(selected)
    report["globally_watertight"]=all(v["watertight"] for v in report["boundary_audit"].values())
    report["junction_gap_audit"]=audit_junctions(model)
    # Junction gap screening is advisory while the body remains an open mesh.
    write_obj(out/"whole_body.obj",selected)
    (out/"validation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report))
    raise SystemExit(0 if report["passed"] else 1)
