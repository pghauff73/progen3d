"""Topological stitching of anatomical surface patches.

Welds a collar's first ring to a limb's first ring using shared indices.
This produces a connected shoulder/hip patch, but the opposite collar rim
still requires a torso/pelvis junction for a globally watertight body.
"""
import math

def stitch_limb_collar(limb,collar,sides=20,tolerance=1e-8):
    lv,lf=limb
    cv,cf=collar
    if sides<3 or len(lv)<sides or len(cv)<2*sides:
        raise ValueError("invalid ring topology")
    if any(math.dist(lv[i],cv[i])>tolerance for i in range(sides)):
        raise ValueError("collar and limb attachment rings differ")
    # Reuse limb vertices for the collar attachment ring. Do not duplicate
    # seam vertices: one shared index per rim point.
    vertices=list(lv)+list(cv[sides:])
    mapping={i:i for i in range(sides)}
    mapping.update({i:len(lv)+(i-sides) for i in range(sides,len(cv))})
    faces=list(lf)+[tuple(mapping[i] for i in f) for f in cf]
    return vertices,faces

def edge_incidence(faces):
    uses={}
    for face in faces:
        for a,b in ((face[0],face[1]),(face[1],face[2]),(face[2],face[0])):
            edge=(min(a,b),max(a,b))
            uses[edge]=uses.get(edge,0)+1
    return uses

def stitch_report(mesh):
    vertices,faces=mesh
    uses=edge_incidence(faces)
    return {"vertices":len(vertices),"triangles":len(faces),
            "boundary_edges":sum(v==1 for v in uses.values()),
            "nonmanifold_edges":sum(v>2 for v in uses.values()),
            "watertight":bool(uses) and all(v==2 for v in uses.values())}
