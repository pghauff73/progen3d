"""3D torso skin envelope anchored to the quaternion pelvis/torso hierarchy.

Elliptical rings provide a stable reference surface for later shoulder and
hip remeshing. This is not a closed or anatomically calibrated torso.
"""
import math
from spatial_skeleton import spatial_skeleton

def torso_envelope(joints,rings=33,sides=32,clearance=0.0):
    if rings<3 or sides<8 or not math.isfinite(clearance) or clearance<0:
        raise ValueError("invalid torso envelope parameters")
    torso=joints["torso"]
    vertices=[];faces=[]
    for i in range(rings):
        u=i/(rings-1)
        y=0.57*u
        rx=0.17+0.045*math.sin(math.pi*u)**2+0.035*math.exp(-((u-0.78)/0.2)**2)
        rz=0.105+0.035*math.sin(math.pi*u)**2
        for j in range(sides):
            angle=2*math.pi*j/sides
            local=((rx+clearance)*math.cos(angle),y,
                   (rz+clearance)*math.sin(angle))
            vertices.append(torso.apply(local))
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides
            c=(i+1)*sides+j;d=(i+1)*sides+(j+1)%sides
            faces.extend(((a,c,b),(b,c,d)))
    return vertices,faces
