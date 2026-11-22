"""Volume-preserving reduced-order shoulder muscle belly geometry.

A spindle is sampled with a fixed longitudinal profile. Cross-section scales
as sqrt(rest_length/current_length), preserving the approximate integrated
volume of the belly when the attachment distance changes. Clamped to avoid
unbounded inflation near collapsed attachments. This is not a force model.
"""
import math
from shoulder_muscles import spindle

def volume_scale(rest_length,current_length,minimum=0.65,maximum=1.55):
    if not all(math.isfinite(v) for v in (rest_length,current_length,minimum,maximum)):
        raise ValueError("nonfinite muscle parameters")
    if rest_length<=0 or current_length<=0 or minimum<=0 or maximum<minimum:
        raise ValueError("invalid muscle parameters")
    return max(minimum,min(maximum,math.sqrt(rest_length/current_length)))

def volume_corrected_spindle(start,end,rest_length,belly=0.025,end_radius=0.006):
    current=math.dist(start,end)
    factor=volume_scale(rest_length,current)
    mesh=spindle(start,end,belly=belly*factor,end_radius=end_radius*factor)
    return mesh,{"rest_length_m":rest_length,"length_m":current,
                 "radial_scale":factor,
                 "approx_volume_ratio":(current/rest_length)*factor*factor,
                 "clamped":abs(factor-math.sqrt(rest_length/current))>1e-10}

def corrected_shoulders(joints,rest_joints):
    """Regenerate bilateral muscle proxies from current and neutral skeletons."""
    from shoulder_muscles import shoulder_muscles
    # Keep the existing muscle definitions as the source of proxy attachment
    # geometry. Use endpoint ring centroids to recover attachment centers.
    current=shoulder_muscles(joints)
    neutral=shoulder_muscles(rest_joints)
    output={};metadata={}
    for name,(vertices,faces) in current.items():
        rest_vertices=neutral[name][0]
        sides=16
        center=lambda points:tuple(sum(p[k] for p in points)/len(points) for k in range(3))
        start=center(vertices[:sides])
        end=center(vertices[-sides:])
        rest_start=center(rest_vertices[:sides])
        rest_end=center(rest_vertices[-sides:])
        rest_length=math.dist(rest_start,rest_end)
        mesh,info=volume_corrected_spindle(start,end,rest_length,
             belly={"deltoid":0.031,"trapezius":0.019,"pectoralis":0.024}[name.split("_",1)[1]])
        output[name]=mesh;metadata[name]=info
    return output,metadata
