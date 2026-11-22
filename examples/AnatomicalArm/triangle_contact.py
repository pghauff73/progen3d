"""Triangle-surface closest point and conservative cloth contact utilities.

Reference implementation, pure Python. Broad phase acceleration is deliberately
omitted; suitable for small validation meshes, not production real-time cloth.
"""
import math

def dot(a,b): return sum(x*y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def add(a,b): return tuple(x+y for x,y in zip(a,b))
def mul(a,s): return tuple(x*s for x in a)
def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def length(a): return math.sqrt(dot(a,a))

def closest_triangle(p,a,b,c):
    """Return closest point on triangle (Ericson region tests)."""
    ab=sub(b,a);ac=sub(c,a);ap=sub(p,a)
    d1=dot(ab,ap);d2=dot(ac,ap)
    if d1<=0 and d2<=0:return a
    bp=sub(p,b);d3=dot(ab,bp);d4=dot(ac,bp)
    if d3>=0 and d4<=d3:return b
    vc=d1*d4-d3*d2
    if vc<=0 and d1>=0 and d3<=0:
        v=d1/(d1-d3);return add(a,mul(ab,v))
    cp=sub(p,c);d5=dot(ab,cp);d6=dot(ac,cp)
    if d6>=0 and d5<=d6:return c
    vb=d5*d2-d1*d6
    if vb<=0 and d2>=0 and d6<=0:
        w=d2/(d2-d6);return add(a,mul(ac,w))
    va=d3*d6-d5*d4
    if va<=0 and d4-d3>=0 and d5-d6>=0:
        w=(d4-d3)/((d4-d3)+(d5-d6))
        return add(b,mul(sub(c,b),w))
    denom=va+vb+vc
    if abs(denom)<1e-20:return a
    v=vb/denom;w=vc/denom
    return add(a,add(mul(ab,v),mul(ac,w)))

class TriangleSurface:
    def __init__(self,vertices,faces):
        self.vertices=[tuple(v) for v in vertices]
        self.triangles=[]
        for face in faces:
            if len(face)!=3:raise ValueError("triangular faces required")
            a,b,c=(self.vertices[i] for i in face)
            n=cross(sub(b,a),sub(c,a))
            norm=length(n)
            if norm>1e-14:
                self.triangles.append((a,b,c,mul(n,1/norm)))
        if not self.triangles:raise ValueError("no nondegenerate triangles")

    def nearest(self,p):
        best=None;dist2=float("inf")
        for a,b,c,n in self.triangles:
            q=closest_triangle(p,a,b,c)
            d=dot(sub(p,q),sub(p,q))
            if d<dist2:best=(q,n);dist2=d
        return best[0],best[1],math.sqrt(dist2)

    def project(self,p,clearance=0.0004):
        if clearance<0:raise ValueError("negative clearance")
        q,n,d=self.nearest(p)
        signed=dot(sub(p,q),n)
        # Winding must be outward. Projecting against an open or misoriented
        # mesh is not a valid inside/outside collision classification.
        if signed>=clearance:return tuple(p)
        return add(p,mul(n,clearance-signed))
