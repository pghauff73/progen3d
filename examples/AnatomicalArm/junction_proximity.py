"""Spatial proximity diagnostics for anatomical torso and limb junctions.

Computes nearest vertex-to-triangle distances with an optional sample stride.
Distances are unsigned; this is not a robust penetration or self-intersection
test, but exposes large gaps and suspiciously coincident geometry.
"""
import math
from triangle_contact import closest_triangle

def proximity(a,b,stride=4):
    av,af=a;bv,bf=b
    if not isinstance(stride,int) or isinstance(stride,bool) or stride<1:raise ValueError("invalid stride")
    if not av or not bv or not bf:raise ValueError("empty surface")
    if any(not all(math.isfinite(x) for x in p) for p in av+bv):
        raise ValueError("nonfinite surface vertex")
    minimum=float("inf")
    maximum=0.0
    count=0
    worst_sample=0
    for sample_index in range(0,len(av),stride):
        p=av[sample_index]
        best=min(math.dist(p,closest_triangle(p,bv[i],bv[j],bv[k]))
                 for i,j,k in bf)
        minimum=min(minimum,best)
        if best>maximum:
            maximum=best
            worst_sample=sample_index
        count+=1
    return {"samples":count,"minimum_distance_m":minimum,
            "maximum_nearest_distance_m":maximum,
            "worst_sample_index":worst_sample}

def junction_report(model,stride=8):
    result={}
    for side in ("left","right"):
        for region,torso_name,collar_name in (
            ("shoulder","torso_skin",side+"_shoulder_skin_collar"),
            ("hip","torso_skin",side+"_hip_skin_collar")):
            # Sample only the collar's far rim, not the whole collar.
            vertices,faces=model[collar_name]
            sides=20
            rim=vertices[-sides:]
            result[side+"_"+region]=proximity((rim,[]),model[torso_name],stride)
    return result
