"""Reduced-order radius/ulna articulation for the spatial anatomical body.

Independent pronation/supination rotates the distal radial center around the
ulnar axis while keeping the proximal radial head near the lateral elbow.
The meshes are tapered visualization proxies, not CT-derived articular surfaces.
Units: metres; positive pronation angle follows the chosen local convention.
"""
import math
from spatial_skeleton import rotate,euler_xyz

def forearm_landmarks(joints,side,pronation_degrees=0.0):
    if side not in ("left","right"):
        raise ValueError("invalid side")
    if not math.isfinite(pronation_degrees) or not -85<=pronation_degrees<=85:
        raise ValueError("pronation outside supported range")
    elbow=joints[side+"_elbow"]
    wrist=joints[side+"_wrist"]
    sign=-1 if side=="left" else 1
    proximal_ulna=elbow.apply((0.0,-0.014,0.0))
    distal_ulna=wrist.apply((0.0,0.0,0.0))
    proximal_radius=elbow.apply((sign*0.021,-0.012,0.0))
    # Radius orbits the ulna at the distal forearm; its proximal center
    # remains close to the radial head at the elbow.
    distal_offset=rotate(euler_xyz(ry=pronation_degrees),
                         (sign*0.023,0.0,0.0))
    distal_radius=wrist.apply(distal_offset)
    return {"ulna":(proximal_ulna,distal_ulna),
            "radius":(proximal_radius,distal_radius)}

def forearm_bone_meshes(joints,side,pronation_degrees=0.0):
    from spatial_body import segment_surface
    landmarks=forearm_landmarks(joints,side,pronation_degrees)
    return {side+"_ulna_bone":segment_surface(*landmarks["ulna"],0.010,0.008),
            side+"_radius_bone":segment_surface(*landmarks["radius"],0.009,0.011)}
