"""XPBD bending constraints for triangulated cloth.

Uses distance constraints between opposite vertices of adjacent triangles as a
simple bending proxy. This is not a dihedral-angle bending model.
"""
from collections import defaultdict
import math

def opposite_vertex_bends(faces):
    edge_faces=defaultdict(list)
    for a,b,c in faces:
        for u,v,opposite in ((a,b,c),(b,c,a),(c,a,b)):
            edge_faces[tuple(sorted((u,v)))].append(opposite)
    return sorted({tuple(sorted(opp)) for opp in edge_faces.values()
                   if len(opp)==2 and opp[0]!=opp[1]})

def solve_distance_constraints(positions, pairs, rest, inverse_mass,
                               compliance, dt, iterations=1):
    if dt<=0 or compliance<0 or iterations<1:
        raise ValueError("invalid solver settings")
    if len(pairs)!=len(rest):
        raise ValueError("constraint length mismatch")
    alpha=compliance/(dt*dt)
    lambdas=[0.0]*len(pairs)
    for _ in range(iterations):
        for e,(i,j) in enumerate(pairs):
            wi,wj=inverse_mass[i],inverse_mass[j]
            if wi+wj==0:continue
            p,q=positions[i],positions[j]
            delta=[p[k]-q[k] for k in range(3)]
            length=math.sqrt(sum(x*x for x in delta))
            if length<1e-12:continue
            change=(-(length-rest[e])-alpha*lambdas[e])/(wi+wj+alpha)
            lambdas[e]+=change
            for k in range(3):
                gradient=delta[k]/length
                p[k]+=wi*change*gradient
                q[k]-=wj*change*gradient
    return positions
