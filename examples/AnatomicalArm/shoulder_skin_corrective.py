"""Pose-dependent shoulder skin displacement from proxy muscle envelopes.

Applies bounded radial displacement to the proximal arm rings. This is a
visual corrective field, not a constitutive tissue or muscle force model.
"""
import math

def shoulder_corrective(mesh,shoulder,elbow,elevation=0,amplitude=0.006,
                        rings=61,sides=20):
    vertices,faces=mesh
    if len(vertices)!=rings*sides or not 0<=elevation<=180:
        raise ValueError("invalid skin mesh or elevation")
    if not math.isfinite(amplitude) or amplitude<0:
        raise ValueError("invalid corrective amplitude")
    direction=tuple(elbow[k]-shoulder[k] for k in range(3))
    length=math.sqrt(sum(x*x for x in direction))
    if length<1e-9:raise ValueError("degenerate shoulder segment")
    axis=tuple(x/length for x in direction)
    # Raised-arm activation ramps smoothly from 30 to 120 degrees.
    t=max(0.,min(1.,(elevation-30)/90))
    activation=t*t*(3-2*t)
    if activation==0 or amplitude==0:
        return list(vertices),list(faces)
    result=[]
    for i,p in enumerate(vertices):
        u=(i//sides)/(rings-1)
        # Confine the bulge to proximal upper arm; vanish before elbow.
        w=max(0.,1-u/0.38)
        w=w*w*(3-2*w)
        center=tuple(shoulder[k]+min(u*2,1)*direction[k] for k in range(3))
        offset=tuple(p[k]-center[k] for k in range(3))
        axial=sum(offset[k]*axis[k] for k in range(3))
        radial=tuple(offset[k]-axial*axis[k] for k in range(3))
        norm=math.sqrt(sum(x*x for x in radial))
        if norm<1e-10:raise ValueError("degenerate skin radial frame")
        # Azimuthal modulation produces a directional deltoid contour.
        lateral=max(0.,radial[0]/norm)
        displacement=amplitude*activation*w*(0.35+0.65*lateral)
        result.append(tuple(p[k]+displacement*radial[k]/norm for k in range(3)))
    return result,list(faces)
