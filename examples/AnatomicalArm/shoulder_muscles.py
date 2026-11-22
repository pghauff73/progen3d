"""Reduced-order shoulder muscle envelopes driven by scapular kinematics.

Ellipsoidal spindle tubes between approximate torso/scapular and humeral
landmarks. These are visualization proxies, not anatomically validated
muscle attachment maps or force-producing actuators.
"""
import math

def spindle(a,b,belly=0.025,end_radius=0.006,rings=21,sides=16):
    if belly<=0 or end_radius<=0 or rings<3 or sides<8:
        raise ValueError("invalid muscle envelope parameters")
    # Generate a variable-radius tube along a line segment. The reference
    # radial frame is obtained from the existing spatial segment generator.
    from spatial_body import segment_surface
    vertices,faces=segment_surface(a,b,1.0,1.0,rings,sides)
    center_delta=tuple(b[k]-a[k] for k in range(3))
    output=[]
    for i in range(rings):
        t=i/(rings-1)
        center=tuple(a[k]+t*center_delta[k] for k in range(3))
        radius=end_radius+(belly-end_radius)*math.sin(math.pi*t)**2
        for j in range(sides):
            p=vertices[i*sides+j]
            output.append(tuple(center[k]+radius*(p[k]-center[k]) for k in range(3)))
    return output,faces

def shoulder_muscles(joints):
    result={}
    for side,sign in (("left",-1),("right",1)):
        torso=joints["torso"]
        scapula=joints[side+"_scapula"]
        shoulder=joints[side+"_shoulder"]
        elbow=joints[side+"_elbow"]
        # Approximate anatomical regions, not measured origins/insertions.
        deltoid_start=scapula.apply((sign*0.025,0.012,0.015))
        deltoid_end=shoulder.apply((0,-0.105,0.015))
        trapezius_start=torso.apply((sign*0.045,0.46,-0.045))
        trapezius_end=scapula.apply((0.0,0.0,-0.012))
        pectoral_start=torso.apply((sign*0.06,0.39,0.085))
        pectoral_end=shoulder.apply((0,-0.07,0.045))
        for name,a,b,belly in (
            ("deltoid",deltoid_start,deltoid_end,0.031),
            ("trapezius",trapezius_start,trapezius_end,0.019),
            ("pectoralis",pectoral_start,pectoral_end,0.024)):
            result[side+"_"+name]=spindle(a,b,belly)
    return result
