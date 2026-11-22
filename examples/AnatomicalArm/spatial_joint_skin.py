"""Continuous 3D two-bone skin envelope for elbows and knees.

One indexed tube spans both segments. A transported radial frame avoids
flipping at arbitrary 3D orientations. The joint is still a kinematic
centerline corner, not a volumetric tissue or self-collision simulation.
"""
import math

def _dot(a,b):return sum(x*y for x,y in zip(a,b))
def _cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def _unit(v):
    n=math.sqrt(_dot(v,v))
    if n<1e-10 or not math.isfinite(n):raise ValueError("degenerate segment or frame")
    return tuple(x/n for x in v)
def _mix(a,b,t):return tuple((1-t)*x+t*y for x,y in zip(a,b))
def _sub(a,b):return tuple(x-y for x,y in zip(a,b))

def joint_envelope(root,joint,tip,root_radius,joint_radius,tip_radius,
                   rings=61,sides=20,blend_width=0.12):
    if rings<9 or rings%2!=1 or sides<8 or not 0<blend_width<0.5:
        raise ValueError("invalid surface resolution or blend width")
    if min(root_radius,joint_radius,tip_radius)<=0:
        raise ValueError("invalid radius")
    before=_unit(_sub(joint,root));after=_unit(_sub(tip,joint))
    # At an exact reversal the tangent is undefined at the hinge. Such poses
    # require a true volume-aware joint model rather than a swept tube.
    if _dot(before,after)<-0.98:
        raise ValueError("near-reversal joint requires volume-aware surface")
    reference=min(((1.,0.,0.),(0.,1.,0.),(0.,0.,1.)),key=lambda a:abs(_dot(a,before)))
    radial=_unit(_cross(before,reference))
    vertices=[];faces=[]
    for i in range(rings):
        u=i/(rings-1)
        if u<=0.5:
            t=u*2;center=_mix(root,joint,t)
            radius=root_radius+(joint_radius-root_radius)*t
        else:
            t=(u-0.5)*2;center=_mix(joint,tip,t)
            radius=joint_radius+(tip_radius-joint_radius)*t
        blend=max(0.,min(1.,(u-(0.5-blend_width))/(2*blend_width)))
        blend=blend*blend*(3-2*blend)
        tangent=_unit(_mix(before,after,blend))
        # Project previous frame axis into the new tangent's normal plane.
        projected=tuple(radial[k]-_dot(radial,tangent)*tangent[k] for k in range(3))
        radial=_unit(projected)
        binormal=_unit(_cross(tangent,radial))
        for j in range(sides):
            a=2*math.pi*j/sides
            vertices.append(tuple(center[k]+radius*(math.cos(a)*radial[k]+math.sin(a)*binormal[k])
                                  for k in range(3)))
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides
            c=(i+1)*sides+j;d=(i+1)*sides+(j+1)%sides
            faces.extend(((a,b,c),(b,d,c)))
    return vertices,faces
