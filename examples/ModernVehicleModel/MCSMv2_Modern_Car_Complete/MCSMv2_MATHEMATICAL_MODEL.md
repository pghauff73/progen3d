# MCSMv2 Mathematical Model

## 1. Hybrid representation

MCSMv2 preserves the MCSMv1 implicit body as a topology-stable scaffold, but
promotes named semantic sections and character curves to first-class geometry.

\[
\mathcal M = (P,G_P,\Sigma,C,S_I,S_E,A,B,K,E,V)
\]

- \(P\): package and platform
- \(G_P\): parameter dependency graph
- \(\Sigma\): semantic section field
- \(C\): longitudinal character curves
- \(S_I\): implicit scaffold
- \(S_E\): explicit semantic surface
- \(A\): panels and closures
- \(B\): body-in-white
- \(K\): kinematic envelopes
- \(E\): evidence/provenance
- \(V\): validation

## 2. Semantic section

At longitudinal position \(x\), the right half-section contains named landmarks

\[
\Gamma_x = \{(w_k(x), z_k(x))\}_{k=0}^{7}
\]

for underbody, rocker, lower body, shoulder, belt, glass shoulder, roof rail and
roof crown. Each field is interpolated with a shape-preserving PCHIP curve.

The section-width field is

\[
w(x,z)=\operatorname{Interp}_z\left(\Gamma_x\right).
\]

## 3. Implicit scaffold

The base body is the intersection of signed constraints

\[
F_s=|y|-w(x,z),\quad
F_l=z_{min}(x)-z,\quad
F_u=z-z_{max}(x),
\]

plus exact front and rear package bounds. Smooth maximum combines these:

\[
\operatorname{smax}_k(a,b)=\frac{1}{k}\log(e^{ka}+e^{kb}).
\]

The zero level set is the principal body surface.

## 4. Swept wheel envelope approximation

For each wheel, steering and suspension ranges produce an envelope represented
by a fourth-order superellipsoid. Front longitudinal/lateral radii include the
maximum steering excursion; vertical radius includes jounce/rebound:

\[
r_x=r+\tfrac12 w\sin\delta_{max}+c,
\]
\[
r_y=\tfrac12w+r\sin\delta_{max}+c,
\]
\[
r_z=r+\max(|j_{min}|,|j_{max}|)+c.
\]

Each side-specific envelope is subtracted from the body field.

## 5. Explicit semantic surface

A PCHIP curve is fitted through each half-section. Mirrored closed section rings
are connected longitudinally to form an explicit semantic surface. It is not yet
an approved Class-A patch graph; it is a deterministic bridge to FGKv1
`CurveNetworkSurface`.

## 6. Evidence and uncertainty

Every parameter belongs to one of:

`Measured`, `Derived`, `Fitted`, `Interpolated`, `DesignChoice`, `Unknown`.

Records retain source, confidence and optional uncertainty. Unknown dimensions
are not silently promoted to measurements.

## 7. Inverse fitting

The package includes a constrained nonlinear least-squares demonstration over
semantic section metrics. It proves the same model can run forward and inverse,
but it is not an image-calibrated production fit.
