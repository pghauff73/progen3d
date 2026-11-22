"""Topological stitching of anatomical surface patches.

Welds a collar's first ring to a limb's first ring using shared indices.
This produces a connected shoulder/hip patch, but the opposite collar rim
still requires a torso/pelvis junction for a globally watertight body.
"""
import math

def ring_edge_directions(faces,sides):
    """Return the directed boundary edges of the shared first ring."""
    expected={(min(i,(i+1)%sides),max(i,(i+1)%sides)) for i in range(sides)}
    directions={}
    for face in faces:
        for a,b in zip(face,face[1:]+face[:1]):
            edge=(min(a,b),max(a,b))
            if edge in expected:
                if edge in directions:
                    raise ValueError("attachment ring is not a boundary")
                directions[edge]=(a,b)
    if directions.keys()!=expected:
        raise ValueError("incomplete attachment ring")
    return directions

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
    # Match the actual boundary winding. Existing limb and collar producers
    # may already have opposite rim directions; a blanket reversal breaks it.
    limb_edges=ring_edge_directions(lf,sides)
    collar_edges=ring_edge_directions(cf,sides)
    same={limb_edges[edge]==collar_edges[edge] for edge in limb_edges}
    if len(same)!=1:
        raise ValueError("inconsistent attachment ring winding")
    reverse=same.pop()
    faces=list(lf)+[
        tuple(mapping[i] for i in ((face[0],face[2],face[1]) if reverse else face))
        for face in cf
    ]
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
