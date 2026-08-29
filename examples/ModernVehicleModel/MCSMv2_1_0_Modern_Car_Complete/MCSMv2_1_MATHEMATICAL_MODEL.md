# MCSMv2.1 Mathematical Model

## Registered semantic surface

Let `S0(u,v)` be the explicit section-network surface and `I` the independently
tessellated no-wheelhouse implicit scaffold. MCSMv2.1 registers each semantic
vertex to the nearest point on `I` while retaining the original rectangular
surface topology and persistent UV coordinates:

\[
S_r(u_i,v_j)=\operatorname*{argmin}_{q\in I}\|S_0(u_i,v_j)-q\|.
\]

The registration is not accepted merely because semantic vertices lie on the
scaffold. A second direction samples the scaffold against `S_r`. Both directions
report mean, RMS, P95 and maximum point-to-triangle distance, plus surface-normal
angle residuals.

## Surface-domain loops

Each panel or aperture is a closed polygonal set in the periodic semantic domain:

\[
\Omega_k\subset[0,1]\times S^1.
\]

`u=0` is rear, `u=1` is front. `v=0` is roof centre, `v=0.5` is underbody centre,
and `v=1` returns to roof centre around the opposite side.

Final body faces are mapped into the semantic domain and assigned one owner:

\[
owner(f)\in\{fixed, panel_1,\dots,panel_n, aperture_1,\dots,aperture_m\}.
\]

Apertures carve ownership before panels. The final invariant is:

\[
\forall f,\quad |owner(f)|=1.
\]

## Scope

The registered surface and UV-domain partition are concept-level geometry.
MCSMv2.1 does not yet add inner panels, flange/hem solids, moving closure joints,
helical glass motion, exact suspension hardpoints, stamping or physical analysis.
