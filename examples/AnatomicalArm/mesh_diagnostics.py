"""Geometric diagnostics for ProGen3D anatomical-arm meshes.

Detects nonfinite coordinates, degenerate faces, edge incidence, and triangle
orientation discontinuities. Open sleeve ends are intentional boundary edges.
"""
import math
from collections import defaultdict

def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])

def subtract(a,b):
    return tuple(x-y for x,y in zip(a,b))

def inspect_mesh(vertices,faces,area_epsilon=1e-12):
    errors=[]
    if not vertices or not faces:
        return {"passed":False,"errors":["empty mesh"]}
    if any(len(v)!=3 or not all(math.isfinite(x) for x in v) for v in vertices):
        return {"passed":False,"errors":["invalid vertices"]}
    edge_uses=defaultdict(list)
    minimum_area=float("inf")
    for fi,face in enumerate(faces):
        if len(face)!=3 or any(not isinstance(i,int) or i<0 or i>=len(vertices) for i in face):
            errors.append(f"invalid triangle {fi}")
            continue
        a,b,c=(vertices[i] for i in face)
        normal=cross(subtract(b,a),subtract(c,a))
        area=0.5*math.sqrt(sum(x*x for x in normal))
        minimum_area=min(minimum_area,area)
        if area<=area_epsilon:
            errors.append(f"degenerate triangle {fi}")
        for i,j in ((face[0],face[1]),(face[1],face[2]),(face[2],face[0])):
            key=(min(i,j),max(i,j))
            edge_uses[key].append((i,j))
    nonmanifold=[edge for edge,uses in edge_uses.items() if len(uses)>2]
    winding=[edge for edge,uses in edge_uses.items()
             if len(uses)==2 and uses[0]==uses[1]]
    boundaries=sum(len(uses)==1 for uses in edge_uses.values())
    if nonmanifold:errors.append(f"{len(nonmanifold)} nonmanifold edges")
    if winding:errors.append(f"{len(winding)} inconsistent shared-edge windings")
    return {"passed":not errors,"errors":errors,
            "vertices":len(vertices),"triangles":len(faces),
            "boundary_edges":boundaries,"nonmanifold_edges":len(nonmanifold),
            "winding_errors":len(winding),
            "minimum_triangle_area_m2":minimum_area}

def inspect_assembly(assembly):
    results={name:inspect_mesh(*mesh) for name,mesh in assembly.items()}
    return {"passed":all(result["passed"] for result in results.values()),
            "objects":results}
