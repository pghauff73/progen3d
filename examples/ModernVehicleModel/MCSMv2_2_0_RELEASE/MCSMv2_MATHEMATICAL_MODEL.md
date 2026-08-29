# MCSMv2.2 Mathematical Model

## Representation

\[
\mathcal M_{2.2}=(P,G,\Sigma,C,S_{UV},S_I,D,H,W,K_g,V)
\]

- \(P\): package and platform parameters;
- \(G\): typed dependency graph;
- \(\Sigma\): ordered semantic sections;
- \(C\): character curves;
- \(S_{UV}\): persistent UV semantic surface;
- \(S_I\): independently tessellated implicit outer scaffold;
- \(D\): exclusive panel/aperture domain partition;
- \(H\): suspension hardpoints and wheel-pose solver;
- \(W\): actual tyre mesh sweeps;
- \(K_g\): closure and helical-glass kinematics;
- \(V\): independent validation evidence.

## Semantic projection

For semantic point \(p\), section interior centre \(c\), and ray \(d=p-c\),
MCSMv2.2 finds \(t\) with a bracketed solve:

\[
F_o(c+t d)=0.
\]

The projected point retains the original \((u,v)\) identity.

## Surface partition

Every face receives exactly one base panel owner and optionally one aperture
owner. Apertures override base panel ownership in the resolved ledger:

\[
O(f)=\begin{cases}
A(f),&A(f)\neq\varnothing\\
P(f),&\text{otherwise.}
\end{cases}
\]

## Wheel pose

For steer \(\delta\) and suspension travel \(j\), each wheel receives a finite
rigid transform with derived centre, camber and toe. A real elliptical-section
tyre mesh is transformed through the sampled pose grid. The sweep envelope is
the convex hull of all transformed tyre vertices.

## Closure transform

A revolute closure uses

\[
T(q)=T(p)R_{\hat a}(q\theta_{max})T(-p),\qquad q\in[0,1].
\]

Bonnet and hatch systems add a small evidence-labelled rise translation.

## Helical side glass

Door-local glass motion combines downward travel, inward motion, longitudinal
shift and barrel-following rotation:

\[
T_g(q)=T(\Delta xq,\Delta yq,-hq)R_x(\phi q).
\]

The world transform is the parent-door transform multiplied by \(T_g\).

## Assurance

V3 in this release means sampled concept kinematics passed against independent
triangle and cavity checks. It is not production suspension, closure, regulator
or homologation approval.
