"""Hierarchical joint transforms for the ProGen3D anatomical-body prototype.

Planar sagittal motion in XY with a right-handed Z rotation. Transform
composition keeps child joints attached when the torso or pelvis rotates.
Units: metres and degrees.
"""
from dataclasses import dataclass
import math

@dataclass(frozen=True)
class Transform:
    origin: tuple=(0.0,0.0,0.0)
    angle: float=0.0

    def apply(self,p):
        a=math.radians(self.angle);c=math.cos(a);s=math.sin(a)
        return (self.origin[0]+c*p[0]-s*p[1],
                self.origin[1]+s*p[0]+c*p[1],
                self.origin[2]+p[2])

    def child(self,offset,angle=0.0):
        return Transform(self.apply(offset),self.angle+angle)

def skeleton(torso_lean=0.0,pelvis_tilt=0.0,
             shoulder_left=0.0,shoulder_right=0.0,
             hip_left=0.0,hip_right=0.0,
             knee_left=0.0,knee_right=0.0):
    values=(torso_lean,pelvis_tilt,shoulder_left,shoulder_right,
            hip_left,hip_right,knee_left,knee_right)
    if not all(math.isfinite(x) for x in values):
        raise ValueError("nonfinite skeleton angle")
    if abs(torso_lean)>35 or abs(pelvis_tilt)>25:
        raise ValueError("torso or pelvis rotation outside supported range")
    pelvis=Transform((0.0,0.05,0.0),pelvis_tilt)
    torso=pelvis.child((0.0,0.0,0.0),torso_lean-pelvis_tilt)
    joints={"pelvis":pelvis,"torso":torso}
    for side,sign,shoulder,hip,knee in (
        ("left",-1,shoulder_left,hip_left,knee_left),
        ("right",1,shoulder_right,hip_right,knee_right)):
        joints[side+"_shoulder"]=torso.child((sign*0.22,0.50,0),shoulder)
        joints[side+"_hip"]=pelvis.child((sign*0.105,0,0),sign*hip)
        joints[side+"_knee"]=joints[side+"_hip"].child((0,-0.43,0),-sign*knee)
        joints[side+"_ankle"]=joints[side+"_knee"].child((0,-0.43,0))
    return joints

def segment_lengths(joints):
    return {side:(
        math.dist(joints[side+"_hip"].origin,joints[side+"_knee"].origin),
        math.dist(joints[side+"_knee"].origin,joints[side+"_ankle"].origin))
        for side in ("left","right")}
