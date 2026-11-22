"""Lightweight XPBD distance-constraint solver for ProGen3D arm sleeve prototypes.

Pure Python; operates on arbitrary triangle meshes. No claim of anatomical FEM.
"""
import math

def distance(a,b):
    return math.sqrt(sum((x-y)**2 for x,y in zip(a,b)))

def edges_from_faces(faces):
    return sorted({tuple(sorted((a,b))) for face in faces for a,b in
                   ((face[0],face[1]),(face[1],face[2]),(face[2],face[0]))})

class XPBDCloth:
    def __init__(self, vertices, faces, mass=0.01, compliance=1e-7,
                 damping=0.985, pinned=()):
        if mass<=0 or compliance<0 or not 0<=damping<=1:
            raise ValueError("invalid cloth parameters")
        self.positions=[list(v) for v in vertices]
        self.previous=[list(v) for v in vertices]
        self.edges=edges_from_faces(faces)
        self.rest=[distance(vertices[i],vertices[j]) for i,j in self.edges]
        self.inverse_mass=[0.0 if i in pinned else 1.0/mass
                           for i in range(len(vertices))]
        self.compliance=compliance
        self.damping=damping

    def step(self,dt,iterations=8,gravity=(0,-9.81,0),
             collision=None):
        if dt<=0 or iterations<1:
            raise ValueError("invalid time step or iterations")
        # Verlet prediction.
        for i,p in enumerate(self.positions):
            if self.inverse_mass[i]==0: continue
            position_before_prediction=p[:]
            for k in range(3):
                vel=(p[k]-self.previous[i][k])*self.damping
                p[k]+=vel+gravity[k]*dt*dt
            # The next step measures velocity from this step's starting point,
            # not from an older position that would amplify motion each step.
            self.previous[i]=position_before_prediction
        # XPBD Lagrange multipliers reset each time step.
        lambdas=[0.0]*len(self.edges)
        alpha=self.compliance/(dt*dt)
        for _ in range(iterations):
            for e,(i,j) in enumerate(self.edges):
                wi,wj=self.inverse_mass[i],self.inverse_mass[j]
                if wi+wj==0: continue
                pi,pj=self.positions[i],self.positions[j]
                d=[pi[k]-pj[k] for k in range(3)]
                length=math.sqrt(sum(x*x for x in d))
                if length<1e-12: continue
                C=length-self.rest[e]
                dl=(-C-alpha*lambdas[e])/(wi+wj+alpha)
                lambdas[e]+=dl
                for k in range(3):
                    grad=d[k]/length
                    pi[k]+=wi*dl*grad
                    pj[k]-=wj*dl*grad
            if collision is not None:
                # Collision callback projects an unpinned vertex onto a safe surface.
                for i,p in enumerate(self.positions):
                    if self.inverse_mass[i]:
                        projected=collision(tuple(p))
                        if len(projected)!=3 or not all(math.isfinite(x) for x in projected):
                            raise ValueError("invalid collision projection")
                        p[:]=projected
        if not all(math.isfinite(x) for p in self.positions for x in p):
            raise FloatingPointError("nonfinite XPBD state")
        return [tuple(p) for p in self.positions]

def sphere_projector(center,radius):
    if radius<=0: raise ValueError("radius must be positive")
    def project(point):
        v=[point[k]-center[k] for k in range(3)]
        r=math.sqrt(sum(x*x for x in v))
        if r>=radius: return point
        if r<1e-12: v=[1.0,0.0,0.0];r=1.0
        return tuple(center[k]+radius*v[k]/r for k in range(3))
    return project
