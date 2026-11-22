"""3D quaternion joint hierarchy for ProGen3D anatomical research.

Right-handed coordinates: X lateral, Y superior, Z anterior. Local joint
rotation order is intrinsic XYZ (implemented qz*qy*qx). All distances metres.
This is a kinematic skeleton, not a biomechanical dynamics model.
"""
from dataclasses import dataclass
import math

def quat_mul(a,b):
    w,x,y,z=a; v,i,j,k=b
    return (w*v-x*i-y*j-z*k,w*i+x*v+y*k-z*j,
            w*j-x*k+y*v+z*i,w*k+x*j-y*i+z*v)

def quat_normalize(q):
    n=math.sqrt(sum(x*x for x in q))
    if not math.isfinite(n) or n<1e-12:raise ValueError("invalid quaternion")
    return tuple(x/n for x in q)

def euler_xyz(rx=0,ry=0,rz=0):
    if not all(math.isfinite(v) for v in (rx,ry,rz)):
        raise ValueError("nonfinite rotation")
    def axis(a,which):
        t=math.radians(a)/2
        q=[math.cos(t),0.,0.,0.]
        q[which]=math.sin(t)
        return tuple(q)
    return quat_normalize(quat_mul(quat_mul(axis(rz,3),axis(ry,2)),axis(rx,1)))

def rotate(q,p):
    q=quat_normalize(q)
    w,x,y,z=q
    # Unit-quaternion vector rotation without allocating intermediate quats.
    u=(x,y,z)
    t=(2*(u[1]*p[2]-u[2]*p[1]),
       2*(u[2]*p[0]-u[0]*p[2]),
       2*(u[0]*p[1]-u[1]*p[0]))
    return (p[0]+w*t[0]+u[1]*t[2]-u[2]*t[1],
            p[1]+w*t[1]+u[2]*t[0]-u[0]*t[2],
            p[2]+w*t[2]+u[0]*t[1]-u[1]*t[0])

@dataclass(frozen=True)
class SpatialTransform:
    origin: tuple=(0.,0.,0.)
    rotation: tuple=(1.,0.,0.,0.)

    def apply(self,p):
        r=rotate(self.rotation,p)
        return tuple(self.origin[i]+r[i] for i in range(3))

    def child(self,offset,rotation=(1.,0.,0.,0.)):
        return SpatialTransform(self.apply(offset),
                                quat_normalize(quat_mul(self.rotation,rotation)))

def spatial_skeleton(pose=None):
    """Build shoulder, elbow, wrist, hip, knee and ankle joint chains.

    Pose entries use keys like left_shoulder_xyz, right_hip_xyz,
    left_elbow_flex, torso_xyz, pelvis_xyz. XYZ values are degrees.
    """
    pose={} if pose is None else dict(pose)
    allowed={"torso_xyz","pelvis_xyz"}
    for side in ("left","right"):
        allowed.update((side+"_shoulder_xyz",side+"_hip_xyz",
                        side+"_elbow_flex",side+"_knee_flex"))
    unknown=set(pose)-allowed
    if unknown:raise ValueError("unknown joints: "+", ".join(sorted(unknown)))
    def xyz(key,limit):
        angles=pose.get(key,(0.,0.,0.))
        if len(angles)!=3 or any(not math.isfinite(a) or abs(a)>limit for a in angles):
            raise ValueError("invalid rotation for "+key)
        return euler_xyz(*angles)
    def flex(key,limit):
        value=pose.get(key,0.)
        if not math.isfinite(value) or not 0<=value<=limit:
            raise ValueError("invalid flexion for "+key)
        return value
    pelvis=SpatialTransform((0.,0.05,0.),xyz("pelvis_xyz",25))
    torso=pelvis.child((0.,0.,0.),xyz("torso_xyz",35))
    joints={"pelvis":pelvis,"torso":torso}
    for side,sign in (("left",-1),("right",1)):
        shoulder=torso.child((sign*0.22,0.50,0.),xyz(side+"_shoulder_xyz",110))
        elbow=shoulder.child((0.,-0.30,0.),euler_xyz(rz=sign*flex(side+"_elbow_flex",145)))
        wrist=elbow.child((0.,-0.26,0.))
        hip=pelvis.child((sign*0.105,0.,0.),xyz(side+"_hip_xyz",110))
        knee=hip.child((0.,-0.43,0.),euler_xyz(rz=-sign*flex(side+"_knee_flex",140)))
        ankle=knee.child((0.,-0.43,0.))
        joints.update({side+"_shoulder":shoulder,side+"_elbow":elbow,
                       side+"_wrist":wrist,side+"_hip":hip,
                       side+"_knee":knee,side+"_ankle":ankle})
    return joints

def length_errors(joints):
    errors={}
    for side in ("left","right"):
        for a,b,target in (("shoulder","elbow",0.30),("elbow","wrist",0.26),
                           ("hip","knee",0.43),("knee","ankle",0.43)):
            measured=math.dist(joints[side+"_"+a].origin,joints[side+"_"+b].origin)
            errors[side+"_"+a+"_"+b]=abs(measured-target)
    return errors
