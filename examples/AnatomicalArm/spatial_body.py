"""Render-ready 3D skeletal envelope driven by quaternion joint transforms.

This separate prototype uses the 3D skeleton directly. It does not yet
replace the planar soft-tissue and XPBD garment system.
"""
import argparse
import json
import math
from pathlib import Path
from spatial_skeleton import spatial_skeleton,length_errors
from arm_milestone_a import write_obj
from spatial_joint_skin import joint_envelope
from spatial_torso import torso_envelope

def segment_surface(start,end,radius_start,radius_end,rings=24,sides=20):
    if rings<2 or sides<3 or min(radius_start,radius_end)<=0:
        raise ValueError("invalid segment mesh parameters")
    direction=tuple(end[k]-start[k] for k in range(3))
    length=math.sqrt(sum(x*x for x in direction))
    if length<1e-10:raise ValueError("zero length segment")
    tangent=tuple(v/length for v in direction)
    reference=(0.,0.,1.) if abs(tangent[2])<0.9 else (1.,0.,0.)
    u=(tangent[1]*reference[2]-tangent[2]*reference[1],
       tangent[2]*reference[0]-tangent[0]*reference[2],
       tangent[0]*reference[1]-tangent[1]*reference[0])
    norm=math.sqrt(sum(v*v for v in u))
    u=tuple(v/norm for v in u)
    v=(tangent[1]*u[2]-tangent[2]*u[1],
       tangent[2]*u[0]-tangent[0]*u[2],
       tangent[0]*u[1]-tangent[1]*u[0])
    vertices=[];faces=[]
    for i in range(rings):
        t=i/(rings-1)
        radius=radius_start+(radius_end-radius_start)*t
        center=tuple(start[k]+t*direction[k] for k in range(3))
        for j in range(sides):
            a=2*math.pi*j/sides
            vertices.append(tuple(center[k]+radius*(math.cos(a)*u[k]+math.sin(a)*v[k])
                                  for k in range(3)))
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides
            c=(i+1)*sides+j;d=(i+1)*sides+(j+1)%sides
            faces.extend(((a,b,c),(b,d,c)))
    return vertices,faces

def spatial_body(pose=None):
    joints=spatial_skeleton(pose)
    mesh={"torso_skin":torso_envelope(joints),
          "torso_garment":torso_envelope(joints,clearance=0.006)}
    segments=(("shoulder","elbow",0.047,0.037),
              ("elbow","wrist",0.037,0.022),
              ("hip","knee",0.083,0.053),
              ("knee","ankle",0.055,0.031))
    for side in ("left","right"):
        for start,end,r0,r1 in segments:
            mesh[side+"_"+start+"_"+end+"_skin"]=segment_surface(
                joints[side+"_"+start].origin,joints[side+"_"+end].origin,r0,r1)
    for side in ("left","right"):
        for label,a,b,c,radii in (
            ("arm","shoulder","elbow","wrist",(0.047,0.037,0.022)),
            ("leg","hip","knee","ankle",(0.083,0.055,0.031))):
            envelope=joint_envelope(
                joints[side+"_"+a].origin,
                joints[side+"_"+b].origin,
                joints[side+"_"+c].origin,*radii)
            if label=="arm":
                elevation=(pose or {}).get(side+"_arm_elevation",0.0)
                envelope=shoulder_corrective(
                    envelope,joints[side+"_shoulder"].origin,
                    joints[side+"_elbow"].origin,elevation=elevation)
            mesh[side+"_"+label+"_continuous_skin"]=envelope
    mesh.update(shoulder_muscles(joints))
    return mesh,joints

def render_meshes(meshes,layer="skin"):
    if layer=="diagnostic":return dict(meshes)
    if layer not in ("skin","clothed"):
        raise ValueError("invalid spatial body render layer")
    torso="torso_skin" if layer=="skin" else "torso_garment"
    return {name:mesh for name,mesh in meshes.items()
            if name==torso or name.endswith("_continuous_skin")}

def audit(pose=None):
    meshes,joints=spatial_body(pose)
    errors=length_errors(joints)
    return {"schema":"progen3d.spatial-body.v1",
            "passed":max(errors.values())<1e-9,
            "max_segment_length_error_m":max(errors.values()),
            "objects":len(meshes),
            "vertices":sum(len(v) for v,f in meshes.values()),
            "triangles":sum(len(f) for v,f in meshes.values()),
            "limitations":["continuous limb skins remain open at torso and distal ends",
                           "torso is an open envelope, not welded to limbs",
                           "muscle proxies are not force-producing actuators",
                           "no soft tissue or garment dynamics"]}

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--out",default="build/spatial-body")
    p.add_argument("--render-layer",choices=("skin","clothed","diagnostic"),default="skin")
    args=p.parse_args()
    target=Path(args.out);target.mkdir(parents=True,exist_ok=True)
    meshes,joints=spatial_body()
    write_obj(target/"spatial_limbs.obj",render_meshes(meshes,args.render_layer))
    report=audit()
    (target/"validation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report))
    raise SystemExit(0 if report["passed"] else 1)
