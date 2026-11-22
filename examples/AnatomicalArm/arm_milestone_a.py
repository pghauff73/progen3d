"""Parametric anatomical arm reference mesh. Units: metres. No external dependencies."""
import argparse
import math
from pathlib import Path

UPPER = 0.30
FOREARM = 0.26
RINGS = 56
SIDES = 20

def landmarks(degrees):
    if not 0 <= degrees <= 145:
        raise ValueError("Elbow flexion must be in [0,145] degrees")
    a = math.radians(degrees)
    elbow = (0.0, -UPPER, 0.0)
    wrist = (FOREARM * math.sin(a), -UPPER - FOREARM * math.cos(a), 0.0)
    return (0.0, 0.0, 0.0), elbow, wrist

def mix(a,b,t):
    return tuple((1-t)*x+t*y for x,y in zip(a,b))

def norm(v):
    n = math.sqrt(sum(x*x for x in v))
    return tuple(x/n for x in v)

def mesh_tube(points, radii, sides=SIDES):
    """Swept elliptical tube with continuous vertices and shared rings."""
    vertices, faces = [], []
    for i,p in enumerate(points):
        tangent = norm(tuple(points[min(i+1,len(points)-1)][k] -
                             points[max(i-1,0)][k] for k in range(3)))
        # Motion is in XY; normal is perpendicular to tangent in that plane.
        normal = (-tangent[1], tangent[0], 0.0)
        for j in range(sides):
            t = 2*math.pi*j/sides
            rx, rz = radii[i]
            vertices.append((p[0]+rx*math.cos(t)*normal[0],
                             p[1]+rx*math.cos(t)*normal[1],
                             p[2]+rz*math.sin(t)))
    for i in range(len(points)-1):
        for j in range(sides):
            a=i*sides+j; b=i*sides+(j+1)%sides
            c=(i+1)*sides+j; d=(i+1)*sides+(j+1)%sides
            faces.extend(((a,c,b),(b,c,d)))
    return vertices, faces

def skin(degrees):
    shoulder,elbow,wrist = landmarks(degrees)
    points=[]; radii=[]
    a=math.radians(degrees)
    for i in range(RINGS):
        s=i/(RINGS-1)
        if s <= 0.53:
            t=s/0.53
            p=mix(shoulder,elbow,t)
            radius=0.051-0.013*t+0.004*math.sin(math.pi*t)
        else:
            t=(s-0.53)/0.47
            p=mix(elbow,wrist,t)
            radius=0.038-0.016*t+0.005*math.sin(math.pi*t)
        # Smooth transition around the elbow to avoid an abrupt radius step.
        elbow_window=math.exp(-((s-0.53)/0.045)**2)
        radius += 0.004*elbow_window*math.sin(a/2)
        points.append(p)
        radii.append((radius, radius*0.94))
    return mesh_tube(points,radii)

def shaft(start,end,radius):
    points=[mix(start,end,i/13) for i in range(14)]
    return mesh_tube(points,[(radius,radius)]*len(points),12)

def parts(degrees):
    shoulder,elbow,wrist=landmarks(degrees)
    a=math.radians(degrees)
    # Separate humerus and paired radius/ulna, with modest anatomical offsets.
    forearm_dir=(math.sin(a),-math.cos(a),0.0)
    perp=(-forearm_dir[1],forearm_dir[0],0.0)
    def shift(p,amount):
        return tuple(p[k]+amount*perp[k] for k in range(3))
    result={"skin":skin(degrees),
            "humerus":shaft(shoulder,elbow,0.012),
            "radius":shaft(shift(elbow,0.010),shift(wrist,0.010),0.007),
            "ulna":shaft(shift(elbow,-0.010),shift(wrist,-0.010),0.007)}
    # Simplified biceps/triceps bellies; shortened flexor fattens with flexion.
    for name,offset,bulge in (("biceps",0.026,1+0.22*math.sin(a/2)),
                              ("triceps",-0.026,1-0.08*math.sin(a/2))):
        p=[mix(shoulder,elbow,0.13+0.75*i/15) for i in range(16)]
        p=[(v[0]+offset,v[1],v[2]) for v in p]
        r=[(0.004+0.014*math.sin(math.pi*i/15)*bulge,)*2 for i in range(16)]
        result[name]=mesh_tube(p,r,12)
    return result

def write_obj(path,parts_map):
    with open(path,"w",encoding="utf-8") as f:
        f.write("# ProGen3D anatomical arm milestone A; metres\n")
        base=0
        for name,(vertices,faces) in parts_map.items():
            f.write("o "+name+"\n")
            for x,y,z in vertices:
                f.write(f"v {x:.8f} {y:.8f} {z:.8f}\n")
            for a,b,c in faces:
                f.write(f"f {a+base+1} {b+base+1} {c+base+1}\n")
            base+=len(vertices)

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--angle",type=float,default=90)
    parser.add_argument("--out",type=Path,default=Path("arm.obj"))
    args=parser.parse_args()
    write_obj(args.out,parts(args.angle))
    print(f"Wrote {args.out} at {args.angle:g} degrees")
