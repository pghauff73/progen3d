# MVP25 Bonnet 2D-to-3D Contour Reference Log

## Artifact identity

- generated: August 22, 2026;
- source sheet: `MVP25_Bonnet_SixView_Dimensioned_Assembly.png`;
- source SHA-256: `5513ba1f6fbfceb7f0e6445ebf2cd9384bbb34933f3e191fdcc643db33c21751`;
- reconstruction definition: `MVP25_Bonnet_3D_Contour_Definition.json`;
- definition SHA-256: `9866e2a51a811ce6fcec5f701ca4e530d927d76fa782c9ca28004da52775fc04`;
- executable grammar: `MVP25_Bonnet_3D_Contours.grammar`;
- grammar SHA-256: `c9fc423497d9ef2d2677f023ac4d153663bbb438b68d1363095d4c8a4ee52143`;
- generator: `generate_bonnet_3d_contours.py`.

## Source crops and extraction

The reconstruction uses the front, left-side, and top panels of the dimensioned bonnet sheet. The raster extractor selects the largest four-connected red-dominant component in each crop. This rejects red annotation fragments and keeps the bonnet silhouette.

| View | Sheet crop `(left, top, right, bottom)` | Dominant component bounds in crop | Pixels |
|---|---:|---:|---:|
| front | `(0, 62, 521, 405)` | `(49, 84, 438, 259)` | 61576 |
| left_side | `(999, 62, 1536, 405)` | `(84, 118, 416, 162)` | 7002 |
| top | `(522, 406, 998, 795)` | `(77, 61, 391, 341)` | 76808 |

The color predicate is `R > 120`, `R > 1.18G`, `R > 1.12B`, and `R-G > 18`. Samples use a two-pixel neighbourhood so antialiased edges and dimension lines do not create single-row gaps.

## Reconstruction theory

Three two-dimensional functions are extracted:

1. `W_observed(z)` from the top view: bonnet width at each longitudinal station;
2. `H(z)` from the left-side view: bonnet crown/thickness at each station;
3. `F_observed(u)` from the front view: normalized transverse camber from left edge through centre crown to right edge.

The exact front tip of the top-view raster creates a one-station inward width dip before the scheduled 1.280 m nose width. The executable width function uses eight neighbour-relaxation steps with 0.35 observation weight and 0.65 neighbour weight while pinning the 1.420 m rear, 1.500 m maximum-width, and 1.280 m front stations. The observed widths remain recorded separately.

The front raster boundary is nearly flat across several central samples, which creates collinear polygon triples and is not a stable section for loft realization. The executable contour therefore uses the declared regularization:

```text
F_symmetric(u) = [F_observed(u) + F_observed(-u)] / 2
F_prior(u) = 1 - u^2
F(u) = normalize[0.25 F_symmetric(u) + 0.75 F_prior(u)]
```

The observed samples remain recorded separately. The parabolic prior is a construction hypothesis used only to create a smooth, symmetric, non-collinear P0 camber; it is not presented as measured Class-A curvature.

They are fused with the separable surface model:

```text
X(u,z) = u W(z) / 2
Y(u,z) = H(z) [0.18 + 0.82 F(u)]
Z(u,z) = z
```

`u` lies in `[-1,1]`. The `0.18` edge-height floor prevents an impossible knife edge while retaining the front-view crown shape. The schedule constrains overall width to 1.500 m, length to 1.280 m, maximum crown to 0.055 m, rear width to 1.420 m, and front width to 1.280 m.

This is a **contour fusion**, not calibrated photogrammetry. The generated engineering sheet is itself an interpreted reference and the front view contains mild illustrative elevation. Therefore the resulting 3D correspondence is explicitly classified as inferred.

## Longitudinal stations

| Station | Z | Observed width | Regularized width | Crown height |
|---:|---:|---:|---:|---:|
| 00 | -0.6400 | 1.2604 | 1.4200 | 0.0280 |
| 01 | -0.5120 | 1.5000 | 1.5000 | 0.0465 |
| 02 | -0.3840 | 1.5000 | 1.4939 | 0.0465 |
| 03 | -0.2560 | 1.4808 | 1.4810 | 0.0508 |
| 04 | -0.1280 | 1.4760 | 1.4686 | 0.0487 |
| 05 | 0.0000 | 1.4569 | 1.4473 | 0.0487 |
| 06 | 0.1280 | 1.4281 | 1.4166 | 0.0487 |
| 07 | 0.2560 | 1.3946 | 1.3724 | 0.0487 |
| 08 | 0.3840 | 1.3227 | 1.3055 | 0.0465 |
| 09 | 0.5120 | 1.0831 | 1.2928 | 0.0550 |
| 10 | 0.6400 | 0.6470 | 1.2800 | 0.0260 |

## Front-derived transverse camber

| Sample | Normalized lateral position `u` | Observed | Regularized `F(u)` |
|---:|---:|---:|---:|
| 00 | -1.000 | 0.0000 | 0.0000 |
| 01 | -0.750 | 0.9608 | 0.5711 |
| 02 | -0.500 | 1.0000 | 0.8140 |
| 03 | -0.250 | 0.9804 | 0.9529 |
| 04 | 0.000 | 0.9804 | 1.0000 |
| 05 | 0.250 | 0.9804 | 0.9529 |
| 06 | 0.500 | 0.9804 | 0.8140 |
| 07 | 0.750 | 0.9608 | 0.5711 |
| 08 | 1.000 | 0.0000 | 0.0000 |

## Grammar realization

- the translucent red `Loft` is a reconstruction envelope, not certified Class-A geometry;
- nine cyan longitudinal contours expose the front-derived camber across the side/top longitudinal functions;
- eleven silver transverse contours expose each reconstructed section;
- yellow edge contours isolate top-view plan evidence;
- the green centre crown isolates side-view evidence;
- `Object`, `Interface`, and `Connect(AlignedWith)` retain semantic ownership and evidence relationships.

## Suggested construction theory

The current surface is a useful P0 contour network. A stronger construction model should separate the following semantic layers:

1. **Class-A highlight surface** — solve a smooth exterior surface from contour and highlight-flow constraints, then validate curvature combs and reflection-line continuity;
2. **inner reinforcement neutral surface** — derive separately from stiffness, latch, hinge, and pedestrian-impact zones rather than uniformly offsetting the exterior;
3. **rolled hem and adhesive corridor** — reserve a perimeter band with local flange angle, hem radius, adhesive bead centreline, squeeze-out volume, and corrosion-drain breaks;
4. **hinge and latch load paths** — introduce anisotropic reinforcement patches whose fibre/load directions connect hinge pivots to latch and safety-catch datums;
5. **oil-canning stability field** — estimate large low-curvature unsupported regions and add embossments only where modal stiffness needs them;
6. **pedestrian head-impact compliance zones** — couple outer-to-inner clearance and local stiffness to deformable impact corridors;
7. **rainwater and wash-water flow graph** — calculate gradient-following drainage paths, rear-cowl shedding, latch-bowl drainage, and seal bypass risk;
8. **thermal paint-bake distortion allowance** — predict springback and adhesive cure distortion before committing the nominal surface;
9. **four-bar swept-clearance envelope** — validate that the reconstructed bonnet, hinges, strut, wipers, cowl, fenders, and lamps remain collision-free for every opening state;
10. **uncertainty tubes instead of single curves** — store every extracted contour as a mean curve plus pixel/calibration covariance, then weight later fitting by evidence confidence;
11. **asymmetry hypothesis testing** — retain symmetric and weakly asymmetric candidates until independent left/right observations decide between them;
12. **developability and draw-direction checks** — detect undercuts, excessive draw depth, thinning risk, and impossible stamping directions before mesh realization;
13. **feature-aware adaptive tessellation** — allocate triangles by curvature, hem proximity, power-dome crests, and attachment zones rather than uniform subdivision;
14. **reprojection residual ledger** — project the reconstructed surface back into every source view and retain per-station residuals instead of visually accepting the fit;
15. **semantic surface tags** — preserve `outerSkin`, `powerDomeLeft`, `powerDomeRight`, `rearCowlEdge`, `frontNoseEdge`, `hingeZone`, and `latchZone` through triangulation, collision, UVs, and export.

## Limitations and fail-closed boundaries

- no lens calibration or camera matrix is available;
- the source sheet is generated artwork, not metrology photography;
- hidden lower surfaces are not observed;
- power-dome topology is not reconstructed as a separate surface in this contour P0;
- the reconstruction enforces bilateral symmetry;
- no claim is made for production stamping feasibility, crash performance, pedestrian impact, water sealing, or hinge clearance;
- those claims require native engineering evidence and dedicated validators.
