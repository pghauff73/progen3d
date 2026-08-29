# MCSMv1: Research-Informed Mathematical Modern-Car Model

## Deliverable

This package contains four original parametric 3D vehicles. Manufacturer data are used as dimensional and architectural reference points only; none of the meshes is a copy of a production CAD model.

## Internet research data

| Reference vehicle | Length mm | Width mm | Height mm | Wheelbase mm | Track F/R mm | Cd |
|---|---:|---:|---:|---:|---:|---:|
| 2025 Volkswagen Golf R | 4295 | 1788 | 1468 | 2629 | not stated / not stated | 0.33 |
| 2025 Audi S3 Sportback | 4353 | 1816 | 1440 | 2624 | 1549 / 1518 | 0.34 |
| 2026 Toyota GRMN Corolla | 4409 | 1849 | 1473 | 2639 | 1587.5 / 1617.98 | not stated |
| 2026 Toyota C-HR BEV | 4519 | 1869 | 1621 | 2751 | not stated / not stated | not stated |

The **reference hot hatch** uses the median length, width, height and wheelbase of the Golf R, Audi S3 Sportback and GRMN Corolla. Track values are averaged where official track dimensions are available. The crossover package uses the C-HR BEV dimensions directly.

## Mathematical body field

The car is expressed in an SAE-style vehicle frame with longitudinal coordinate `x`, lateral coordinate `y`, and vertical coordinate `z`. The body is the zero-isosurface of an implicit field.

Let

```text
s(x) = (x - x_rear) / (x_front - x_rear)
```

and let `a_b(s)`, `c_b(s)`, and `b_b(s)` be PCHIP-interpolated lower-body half-width, vertical centre, and half-height fields. The lower body is

```text
F_b = |xi_b|^5.2 + |y/a_b|^6.0 + |(z-c_b)/b_b|^4.2 - 1
```

The greenhouse is a second generalized superellipsoid,

```text
F_c = |xi_c|^3.6 + |y/a_c|^4.5 + |(z-c_c)/b_c|^3.0 - 1
```

with a shorter longitudinal domain. Front and rear fender volumes use fourth/eighth/fourth-order superellipsoids. Fields are blended with

```text
smin_k(a,b) = -log(exp(-k a) + exp(-k b)) / k
```

and cylindrical wheel-house fields are subtracted using constructive field difference. The body mesh is the level set

```text
F(x,y,z) = 0.
```

The greenhouse glass is reconstructed from the greenhouse field, clipped between belt and roof curves. Character curves and station sections are exported separately, allowing the implicit prototype to be replaced later by an FGKv1 Class-A patch graph.

## Variations

| Variation | Length mm | Width mm | Height mm | WB mm | Track F/R mm | Frontal area proxy m² | Body faces |
|---|---:|---:|---:|---:|---:|---:|---:|
| Research Median AWD Hot Hatch | 4353 | 1816 | 1468 | 2629 | 1568/1568 | 2.237 | 67160 |
| Track Widebody AWD | 4409 | 1849 | 1473 | 2639 | 1588/1618 | 2.320 | 68388 |
| Aero Fastback AWD | 4353 | 1816 | 1440 | 2624 | 1549/1518 | 2.183 | 66320 |
| Urban Crossover EV AWD | 4519 | 1869 | 1621 | 2751 | 1600/1600 | 2.583 | 68436 |

### Research Median AWD Hot Hatch
Balanced daily-performance package and the primary mathematical reference.

### Track Widebody AWD
Wider fender field, wider track, stronger splitter and wing, and reduced nominal underbody clearance.

### Aero Fastback AWD
Reduced greenhouse height, longer rear taper and smaller front opening. This is a geometric design hypothesis, not a CFD-validated drag result.

### Urban Crossover EV AWD
Long wheelbase, taller greenhouse, raised body, underfloor battery volume and no exhaust system.

## Validation performed

- finite vertices and triangle indices
- watertight main body mesh
- exact post-normalized body package dimensions
- hard wheelbase and track parameters
- analytic left/right symmetry
- exported longitudinal character curves and station sections
- projected frontal-area proxy calculated from the implicit body field
- deterministic parameter and validation records

## Research and method references

- [SAE J1100 Motor Vehicle Dimensions](https://saemobilus.sae.org/standards/j1100_200509-motor-vehicle-dimensions): dimension conventions tied to the SAE three-dimensional reference system.
- [SAE J182 Motor Vehicle Fiducial Marks and Three-Dimensional Reference System](https://saemobilus.sae.org/standards/j182_202011-motor-vehicle-fiducial-marks-three-dimensional-reference-system): vehicle reference frame and fiducials.
- [Autodesk Alias NURBS / Class-A continuity documentation](https://help.autodesk.com/view/ALIAS/2024/ENU/?guid=GUID-366304CB-16FF-46F9-9F64-D7385358D855): G0/G1/G2/G3 continuity framing for future Class-A patch conversion.
- [TUM DrivAer open automotive model](https://www.epc.ed.tum.de/en/aer/research-groups/automotive/drivaer/): open, modular automotive geometry and validation precedent.
- [ApolloCar3D](https://arxiv.org/abs/1811.12222): semantic keypoints and deformable CAD-model fitting precedent.

## Limitations

The meshes are concept-level procedural geometry. They have not undergone CFD, crash, ergonomic, Class-A highlight, stamping, suspension-sweep, homologation, or manufacturing validation. The reported frontal area is a rasterized projection proxy; no drag coefficient is claimed for the generated vehicles. Wheel houses are geometric cylinder differences rather than suspension-swept envelopes. Glass, lighting, underbody, trim and wheel hardware are visual/packaging representations.

## Next high-value tests

1. Replace the smooth implicit body with an MVPv2.5 semantic curve network and G2 patch graph.
2. Fit section and character curves against calibrated front, side and top reference imagery.
3. Replace circular wheel-house cuts with full steer/jounce swept volumes.
4. Run mesh-independent CFD comparisons against a DrivAer validation case.
5. Add occupant H-points, eye points, head envelopes and ingress/egress checks.
6. Resolve body-in-white load paths and closure apertures from shared datums.
