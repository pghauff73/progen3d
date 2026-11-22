"""Illustrative elbow articular surfaces and joint-clearance diagnostics.

Three simplified surfaces: humeral trochlea, ulnar trochlear notch and
proximal radial head. All parameters are in metres. These are deliberately
nonclinical geometric proxies; contact uses sampled proximity only.
"""
import math
from spatial_skeleton import rotate

def annular_patch(center,axis,r_major,r_minor,start_angle,end_angle,
                  rings=12,sides=28):
    if rings<2 or sides<3 or min(r_major,r_minor)<=0:
        raise ValueError("invalid articular patch parameters")
    norm=math.sqrt(sum(v*v for v in axis))
    if norm<1e-10:raise ValueError("invalid patch axis")
    direction=tuple(v/norm for v in axis)
    ref=(0.,0.,1.) if abs(direction[2])<0.9 else (0.,1.,0.)
    cross=lambda a,b:(a[1]*b[2]-a[2]*b[1],
                       a[2]*b[0]-a[0]*b[2],
                       a[0]*b[1]-a[1]*b[0])
    u=cross(direction,ref)
    mag=math.sqrt(sum(x*x for x in u));u=tuple(x/mag for x in u)
    v=cross(direction,u)
    vertices=[];faces=[]
    for i in range(rings):
        theta=start_angle+(end_angle-start_angle)*i/(rings-1)
        for j in range(sides):
            phi=2*math.pi*j/sides
            r=r_major+r_minor*math.cos(phi)
            point=tuple(center[k]+r*(u[k]*math.cos(theta)+v[k]*math.sin(theta))
                        +r_minor*math.sin(phi)*direction[k] for k in range(3))
            vertices.append(point)
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides;c=(i+1)*sides+j;d=(i+1)*sides+(j+1)%sides
            faces.extend(((a,b,c),(b,d,c)))
    return vertices,faces

def elbow_articular_meshes(joints,side):
    if side not in ("left","right"):raise ValueError("invalid side")
    shoulder=joints[side+"_shoulder"]
    elbow=joints[side+"_elbow"]
    sign=-1 if side=="left" else 1
    # Surface centers are approximate; humeral component follows the
    # shoulder frame while ulnar and radial components follow elbow frame.
    trochlea_center=shoulder.apply((0.,-0.30,0.))
    ulna_center=elbow.apply((0.,-0.014,0.))
    radial_center=elbow.apply((sign*0.021,-0.012,0.))
    # Thin toroidal sectors provide a visually legible joint interface.
    humerus=annular_patch(trochlea_center,(sign,0.,0.),0.016,0.006,
                         -2.5,2.5)
    ulna=annular_patch(ulna_center,(sign,0.,0.),0.020,0.004,
                      -2.2,2.2)
    radius=annular_patch(radial_center,(0.,1.,0.),0.009,0.003,
                        0.,2*math.pi)
    return {side+"_humeral_trochlea":humerus,
            side+"_ulnar_notch":ulna,
            side+"_radial_head":radius}

def elbow_landmark_clearance(joints,side):
    elbow=joints[side+"_elbow"]
    sign=-1 if side=="left" else 1
    # Radial head to ulnar center separation. This is not signed
    # surface-to-surface penetration or a ligament stability test.
    ulna=elbow.apply((0.,-0.014,0.))
    radius=elbow.apply((sign*0.021,-0.012,0.))
    return {"radial_ulnar_center_distance_m":math.dist(ulna,radius),
            "contact_verified":False,
            "limitation":"landmark separation only; no signed contact solve"}
