"""Transition collars for anatomical-body joint surfaces.

These are visual seam reducers, not boolean unions or watertight skin.
Each collar shares its outer rim coordinates with an existing limb mesh.
"""
import math

def collar_from_ring(mesh, center, inward=(0.0,0.0,0.0),
                     radius_scale=1.6, sides=20, rings=7):
    vertices,faces=mesh
    if sides<8 or rings<2 or len(vertices)<sides:
        raise ValueError("invalid collar topology")
    if radius_scale<=0 or not math.isfinite(radius_scale):
        raise ValueError("invalid collar scale")
    if len(center)!=3 or len(inward)!=3 or not all(
            math.isfinite(v) for v in (*center,*inward)):
        raise ValueError("invalid collar transform")
    first=vertices[:sides]
    result=[]
    for i in range(rings):
        t=i/(rings-1)
        ease=t*t*(3-2*t)
        for p in first:
            offset=tuple(p[k]-center[k] for k in range(3))
            result.append(tuple(center[k]+ease*inward[k]+
                                (1+(radius_scale-1)*ease)*offset[k]
                                for k in range(3)))
    triangles=[]
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides
            c=(i+1)*sides+j;d=(i+1)*sides+(j+1)%sides
            triangles.extend(((a,c,b),(b,c,d)))
    return result,triangles

def rim_error(collar,limb,sides=20):
    return max(math.dist(a,b) for a,b in zip(collar[0][:sides],limb[0][:sides]))
