"""Continuous elbow animation with an XPBD sleeve and moving skin envelope.

Reduced-order contact model: each sleeve vertex is projected radially against its
corresponding skin ring. It is not general triangle-mesh collision detection.
"""
import argparse
import json
import math
from pathlib import Path
from arm_soft_sleeve import RINGS, SIDES, centerline, surface, assembly
from arm_milestone_a import write_obj
from xpbd_cloth import XPBDCloth

def angle_at(time, period=2.0, maximum=145.0):
    if period <= 0: raise ValueError("period must be positive")
    return 0.5*maximum*(1-math.cos(2*math.pi*time/period))

class CoupledArm:
    def __init__(self, initial_angle=0, clearance=0.00035):
        if clearance < 0: raise ValueError("clearance must be nonnegative")
        verts, faces=surface(initial_angle,garment=True)
        self.cloth=XPBDCloth(verts,faces,compliance=2e-7,damping=0.96,
                             pinned=tuple(range(SIDES)))
        self.clearance=clearance
        self.angle=initial_angle
        self.time=0.0
        self._last_target=verts

    def step(self, angle, dt=1/120, iterations=8):
        if not 0 <= angle <= 145: raise ValueError("angle outside range")
        skin,_=surface(angle)
        target,_=surface(angle,garment=True)
        old_target=self._last_target
        self.angle=angle
        # Move material vertices with the changing pose before solving strain.
        for i,p in enumerate(self.cloth.positions):
            delta=[target[i][k]-old_target[i][k] for k in range(3)]
            for k in range(3):
                p[k]+=delta[k]
                self.cloth.previous[i][k]+=delta[k]
        self._last_target=target
        # Skin ring correspondence supplies a normal and an approximate radial
        # contact envelope. This does not detect nonlocal self-collisions.
        def project_index(i, point):
            u=(i//SIDES)/(RINGS-1)
            center=centerline(angle,u)
            sv=skin[i]
            radial=[sv[k]-center[k] for k in range(3)]
            length=math.sqrt(sum(x*x for x in radial))
            normal=[x/length for x in radial]
            signed=sum((point[k]-sv[k])*normal[k] for k in range(3))
            if signed >= self.clearance:
                return point
            return tuple(point[k]+(self.clearance-signed)*normal[k] for k in range(3))
        # The generic XPBD callback lacks a vertex index, so perform a custom
        # projected Gauss-Seidel loop using its edge constraints, with contact
        # correction on every iteration.
        cloth=self.cloth
        if dt<=0 or iterations<1: raise ValueError("invalid solver settings")
        alpha=cloth.compliance/(dt*dt)
        lambdas=[0.0]*len(cloth.edges)
        for i,p in enumerate(cloth.positions):
            if cloth.inverse_mass[i]==0:
                p[:]=target[i]
                cloth.previous[i][:]=target[i]
                continue
            old=cloth.previous[i][:]
            for k in range(3):
                velocity=(p[k]-old[k])*cloth.damping
                p[k]+=velocity
            cloth.previous[i]=old
        for _ in range(iterations):
            for e,(i,j) in enumerate(cloth.edges):
                wi,wj=cloth.inverse_mass[i],cloth.inverse_mass[j]
                if wi+wj==0: continue
                p,q=cloth.positions[i],cloth.positions[j]
                d=[p[k]-q[k] for k in range(3)]
                length=math.sqrt(sum(x*x for x in d))
                if length<1e-12: continue
                dl=(-(length-cloth.rest[e])-alpha*lambdas[e])/(wi+wj+alpha)
                lambdas[e]+=dl
                for k in range(3):
                    grad=d[k]/length
                    p[k]+=wi*dl*grad
                    q[k]-=wj*dl*grad
            for i,p in enumerate(cloth.positions):
                if cloth.inverse_mass[i]:
                    p[:]=project_index(i,p)
        # Final contact correction after constraint iterations.
        for i,p in enumerate(cloth.positions):
            if cloth.inverse_mass[i]:
                p[:]=project_index(i,p)
        self.time+=dt
        if any(not all(math.isfinite(x) for x in p) for p in cloth.positions):
            raise FloatingPointError("nonfinite sleeve position")
        min_gap=min(sum((cloth.positions[i][k]-skin[i][k])*
                        (skin[i][k]-centerline(angle,(i//SIDES)/(RINGS-1))[k])
                        for k in range(3))/
                    math.dist(skin[i],centerline(angle,(i//SIDES)/(RINGS-1)))
                    for i in range(len(skin)) if cloth.inverse_mass[i])
        return {"angle":angle,"time":self.time,"minimum_radial_gap_m":min_gap,
                "passed":min_gap>=self.clearance-1e-7}

    def meshes(self):
        model=assembly(self.angle)
        _,faces=model["sleeve"]
        model["sleeve"]=([tuple(p) for p in self.cloth.positions],faces)
        return model

def simulate(out, frames=60, fps=30, period=2.0):
    if frames<1 or fps<=0: raise ValueError("invalid frame settings")
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    arm=CoupledArm()
    records=[]
    for frame in range(frames):
        angle=angle_at(frame/fps,period)
        result=arm.step(angle,dt=1/fps,iterations=8)
        result["frame"]=frame
        records.append(result)
        write_obj(out/f"frame_{frame:04d}.obj",arm.meshes())
    report={"schema":"progen3d.arm-coupled-xpbd.v1",
            "frames":frames,"fps":fps,"passed":all(r["passed"] for r in records),
            "limitations":["indexed radial contact only","no friction or bending constraints",
                           "no mesh self collision","no native renderer integration"],
            "records":records}
    (out/"simulation.json").write_text(json.dumps(report,indent=2)+"\n")
    return report

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--out",default="build/arm-coupled")
    p.add_argument("--frames",type=int,default=60)
    p.add_argument("--fps",type=int,default=30)
    args=p.parse_args()
    report=simulate(args.out,args.frames,args.fps)
    print(json.dumps({"passed":report["passed"],"frames":report["frames"]}))
    raise SystemExit(0 if report["passed"] else 1)
