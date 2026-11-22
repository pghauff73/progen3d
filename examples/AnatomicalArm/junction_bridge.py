"""Geometric bridges from open joint-collar rims to the torso surface.

Each collar rim vertex is projected onto its closest torso triangle, and a
quad strip is triangulated between the original rim and those projected points.
This is a visual bridge only: target points are not inserted into the torso
triangle topology, so the assembled body is NOT watertight.
"""
import math
from triangle_contact import closest_triangle

def bridge_to_surface(collar,torso,sides=20):
    cv,cf=collar
    tv,tf=torso
    if sides<3 or len(cv)<2*sides or not tv or not tf:
        raise ValueError("invalid bridge topology")
    rim=cv[-sides:]
    targets=[]
    distances=[]
    for point in rim:
        nearest=None
        distance=float("inf")
        for i,j,k in tf:
            q=closest_triangle(point,tv[i],tv[j],tv[k])
            d=math.dist(point,q)
            if d<distance:
                nearest=q
                distance=d
        targets.append(nearest)
        distances.append(distance)
    vertices=list(rim)+targets
    faces=[]
    for j in range(sides):
        n=(j+1)%sides
        faces.extend(((j,n,sides+j),(n,sides+n,sides+j)))
    return (vertices,faces),{
        "maximum_bridge_length_m":max(distances),
        "mean_bridge_length_m":sum(distances)/len(distances),
        "minimum_bridge_length_m":min(distances),
        "target_topology_welded":False,
        "limitations":["target points not welded into torso triangles",
                       "bridge may self-intersect at extreme poses",
                       "not a watertight anatomical surface"]}

def make_bridges(model,layer="skin"):
    if layer not in ("skin","clothed"):
        raise ValueError("invalid body layer")
    torso=model["torso_skin" if layer=="skin" else "torso_garment"]
    output={}
    reports={}
    for side in ("left","right"):
        for region in ("shoulder","hip"):
            name=side+"_"+region+"_"+("skin" if layer=="skin" else "garment")+"_collar"
            bridge,report=bridge_to_surface(model[name],torso)
            key=side+"_"+region+"_"+layer+"_bridge"
            output[key]=bridge
            reports[key]=report
    return output,reports
