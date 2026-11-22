# Native facial membrane and muscle-cable study

`elastic_study.py` exercises `FacialSkin(response(elastic))` through the actual
ProGen3D grammar evaluator. It supplies two named tensile cables, changes the
skin material or a local attachment, and records the resulting equilibria. The
HTML inspector displays exported native vertices, normals, landmarks, cable
endpoints, forces and energies. It contains no replacement deformation model.

This is a **neutral-reference static plane-stress St Venant–Kirchhoff (StVK)
membrane** with elastic support and authored cable insertions. The defaults and
the cable paths are synthetic engineering examples. They are not fitted to a
person or claimed to reproduce anatomical muscle architecture.

## Run the study

From the repository root, using the existing native build dependencies:

```sh
make -j2 build/form-study-sample
python3 examples/FacialSkin/elastic_study.py \
  --sampler build/form-study-sample \
  --out build/facial-elastic-study
```

If using the repository's unpacked GLFW library, set
`LD_LIBRARY_PATH=third_party/debs/glfw/usr/lib/x86_64-linux-gnu` for the command.
The default contains static comparisons and an actual native replay check.
Add `--timeline-frames 13` to store thirteen equilibria over six seconds. A
timeline is a sequence of independently solved states, not a dynamic simulation.

The destination contains:

- `Facial_Elastic_Study.html`: self-contained inspector using indexed native
  mesh assets, camera controls, landmarks, actual cable paths and solver values.
- `Facial_Elastic_Study.p3d`: native time-dependent grammar for the six-second
  activation cycle; both activations follow `0.25*(1-cos(radians(60)*t))`.
- `study.json`: the same presentation data, including native solver records.
- `validation.json`: full-precision exported-geometry checks, equilibrium
  records, material comparisons and replay differences.
- `samples/`: the grammar and native OBJ export for every evaluation.

The validated `Facial_Elastic_Study.p3d` is also kept beside this document as a
native grammar example. It uses the same generated neutral eye references and
the same six-second cable-activation expression.

Run `make facial-skin-check` for the repository's analytic, elastic-kernel,
elastic-geometry and end-to-end grammar gates. The study script exits with a
nonzero status if a required sample fails its diagnostics. Nonconverged native
geometry is rejected by the engine; it is not presented as an equilibrium.

## Explicit grammar

The standalone skin definition below uses metres, newtons and pascals. Native
axes are +X character left, +Y up and +Z forward.

```text
Start -> Face(0.5 0)

Face(leftActivation rightActivation) ->
  I(FacialSkin(
    size(0.16 0.21 0.045) segments(48 64) response(elastic)
    skinMaterial(50000 .35 .001 1000000) solve(2000 .0001)
    smile(0 0) jawOpen(0) blink(0 0) browRaise(0 0)
    muscle(left_smile .87 .56 34/48 20/64 .055 .08 leftActivation .04 40)
    muscle(right_smile .13 .56 14/48 20/64 .055 .08 rightActivation .04 40)
  ) material(softpinkmatteplaster) 1 1)
```

`response(analytic)` is the default and retains the existing analytic behavior.
Elastic mode requires all seven legacy analytic control values to be zero. It
does not treat an analytic expression as a new stress-free material reference.
Elastic-only options in analytic mode are rejected rather than ignored.

| Option | Meaning and supported domain |
| --- | --- |
| `response(analytic\|elastic)` | Select analytic deformation or a neutral-reference elastic equilibrium. |
| `skinMaterial(E nu h q)` | Young modulus `E` in [100, 10⁷] Pa; Poisson ratio `nu` in [0, .49]; thickness `h` in [10⁻⁵, .02] m; support coefficient `q` in [1, 10⁸] N/m³. Defaults: 50000, .35, .001, 1000000. |
| `solve(iterations tolerance)` | Integer iteration limit [8, 2000], default 500; relative free-force tolerance [10⁻⁷, 10⁻²], default 10⁻⁴. This study explicitly uses 2000 and 10⁻⁴. |
| `muscle(name originU originV insertionU insertionV radiusU radiusV activation maxShortening stiffness)` | A fixed neutral origin and one connected, distributed insertion patch. Activation [0, 1], maximum shortening [0, .25], tensile stiffness [0, 10000] N/m. |
| `attachment(name u v radiusU radiusV stiffness)` | Extra distributed springs to the neutral skin positions. Total patch stiffness [0, 10⁶] N/m; zero disables the added stiffness. |
| `tissueRegion(name u v radiusU radiusV youngScale supportScale)` | Local dimensionless scales for Young modulus and distributed support, each [.1, 10]. |

All UV locations are in the **rest texture chart [0, 1]²**. Patch radii are
in [.01, .5] UV units. A UV origin or insertion must lie on actual neutral skin,
not in an aperture. An insertion must contain free skin vertices and have a
nonzero physical rest length. A patch that touches both banks of an aperture
uses the connected component containing its nearest material node; it does not
bridge the hole with insertion weights.

The three named options are repeatable, with at most 16 declarations of each
kind. Names are case-sensitive identifiers of 1–64 letters, digits or
underscores, starting with a letter or underscore, and unique within their kind.
Numeric arguments support the ordinary grammar expressions, variables and `t`.
There are no implicit built-in muscles. For this bounded elastic prototype,
detail is limited to `segments(48 64)`, `segments(96 64)`, `segments(48 128)` or
`segments(96 128)`; the analytic resolution range is unchanged.

The model solves in the primitive's local physical coordinates. Set physical
dimensions with `size(...)`. An outer scene transform is a presentation
transform and does not recompute material forces for a differently sized body.

## What is minimized

Each triangle owns its neutral area and an orthonormal material frame. If
`D_m` is its 2×2 neutral edge matrix and `D_s` its current 3×2 edge matrix, the
surface deformation gradient and Green strain are

\[
F=D_sD_m^{-1},\qquad G=\tfrac12(F^TF-I).
\]

For neutral area `A`, thickness `h`, Young modulus `E` and Poisson ratio `nu`,
the implemented triangle energy is

\[
U_{\mathrm{mem},t}=\frac{AhE}{2(1-\nu^2)}
\left[G_{11}^2+G_{22}^2+2\nu G_{11}G_{22}
+2(1-\nu)G_{12}^2\right].
\]

This is a geometrically nonlinear, isotropic plane-stress StVK membrane law.
The constitutive choice permits rigid rotations while using a quadratic Green
strain energy. It is a tractable prototype law, not a claim that skin is an
isotropic StVK material. The biological-membrane formulation by
[Buganza Tepole et al. (2015)](https://pmc.ncbi.nlm.nih.gov/articles/PMC4522709/)
derives the thin-membrane plane-stress law and discusses the limits of StVK for
nonlinear, anisotropic biological tissue.

For each vertex, the rest area `A_i` is one third of its incident triangle areas.
Distributed support and explicit attachment patches give the stiffness

\[
K_i=q\,A_i\,s_i+\sum_j k_jw_{ji},\qquad
U_{\mathrm{sup}}=\tfrac12\sum_i K_i\|x_i-X_i\|^2.
\]

Here `X_i` is the genuine neutral position, `s_i` is the local support scale,
and each named attachment's normalized weights sum to one. Therefore `q`
has units N/m³, `q A_i` has units N/m, and `k_j` is the whole patch's added
stiffness in N/m. Refining a patch does not assign its full stiffness separately
to every new vertex.

Compact UV patches use

\[
b(u,v)=\max\left(0,1-\left(\frac{u-u_c}{r_u}\right)^2
-\left(\frac{v-v_c}{r_v}\right)^2\right)^2.
\]

Insertion and attachment weights are proportional to `A_i b(u_i,v_i)` on
their selected connected component, then normalized. Material regions blend
with the base material using a partition of unity; overlapping regions are
blended, not multiplied without bound. Triangle Young moduli are sampled at
rest-UV centroids; vertex support scales are sampled at rest vertices.

A cable has a fixed origin `o`, weighted insertion centroid `p`, neutral
length `l_0`, activation `a`, maximum shortening `c`, and stiffness `k`:

\[
p=\sum_iw_ix_i,\qquad
l=\|p-o\|,\qquad l_*=(1-ac)l_0,
\]
\[
e=\max(0,l-l_*),\qquad T=ke,\qquad
U_{\mathrm{cable}}=\tfrac12ke^2.
\]

The origin is interpolated on the native neutral skin chart and remains fixed
as a skull-frame proxy. It is not a modeled anatomical bone insertion. Cable
forces are distributed through the same insertion weights. A cable pulls when
stretched beyond its target; it does not push when slack. An inactive cable can
carry passive tension if another actuator stretches it.

The solver minimizes the sum of membrane, support and cable energies. It uses
an analytic gradient, a stiffness-based diagonal initial inverse Hessian,
limited-memory BFGS with eight correction pairs, positive-curvature updates and
Armijo backtracking. The limited-storage approach follows
[Nocedal (1980)](https://courses.grainger.illinois.edu/ece544na/fa2014/nocedal80.pdf);
that reference does not establish a global convergence guarantee for this
geometry-restricted implementation.

Convergence requires the free-position force norm divided by the exported
`forceScaleN` to be at most the requested relative tolerance. The scale is the
initial free-force norm with a stiffness/size-dependent roundoff floor. Fixed
vertices are excluded from the residual; their reactions are not counted as
unbalanced free forces. The export includes both `freeForceL2N` and
`relativeFreeForce`, so a convergence claim can be examined in physical units.

**Solver precision and display precision are distinct.** The authoritative
positions remain double precision in `FacialSkinMechanicalState`; the native
rendering `Mesh` stores float positions. The requested tolerance applies to the
double equilibrium. Nanometre-scale position rounding can produce a noticeably
larger residual when stiff membrane forces are reevaluated on the float mesh.
The sampler therefore exports `meshFreeForceL2N`, `meshRelativeFreeForce` and
`maximumPositionQuantizationM` alongside the double-state residual. The inspector
labels these **Double solve residual**, **Float surface residual** and
**Position rounding**. It does not claim that float display coordinates satisfy
the double solver's tolerance, and neither residual is relabeled to conceal
rounding error. Geometry feasibility is checked on the final float mesh as well
as during the double solve.

## Comparison design and evidence

The study attempts nine static cases: neutral, left activation .5, right .5,
both .5, softer global material, stiffer global material, a stiffer left region,
a stronger left attachment, and left activation .75. The last case is optional
and appears only if the native equilibrium and geometry checks pass; an omitted
case and its reason remain in `validation.json`.

The material comparisons all retain left activation .5 and right activation 0.
The demonstrated cables use maximum shortening .04 and stiffness 40 N/m, so
activation .5 requests a 2% reduction of cable target length. Distributed
support is 10⁶ N/m³ throughout. These explicit synthetic choices keep the
demonstration within a modest-deformation regime; they are not inferred tissue
measurements or a guarantee for every allowed parameter combination.
They change one authored quantity relative to the left-cable reference:

| Case | Changed property |
| --- | --- |
| Softer skin | Young modulus 25000 Pa. |
| Stiffer skin | Young modulus 100000 Pa. |
| Stiffer left region | `tissueRegion(left_cheek 34/48 20/64 .11 .13 3 1)`. |
| Stronger left attachment | `attachment(left_anchor 34/48 20/64 .055 .08 100)`. |

The material-sensitivity report records actual cable shortening, tension,
total energy and both mouth-corner displacements. A unilateral actuator may
move contralateral tissue through the membrane; an exactly motionless opposite
side is not an acceptance condition. Cable endpoints and corner positions
come from the solved native state, not from schematic guide placement.

The study also evaluates the time sequence 1.5, 5.25, 1.5, 6 and 0 seconds by
complete native grammar evaluations, even when no timeline is stored. It
compares the repeated 1.5-second geometry and the start/end neutral geometry.
The sampler starts a fresh process for each requested snapshot, so this check
establishes repeatability across fresh evaluations; it does not by itself test
a persistent editor's cache. Repeated solves inside one process are covered by
the native integration harness.
Every displayed state is checked for one connected component, four boundary
loops, nonmanifold edges, positive triangle areas, its native neutral-normal hemisphere,
normal consistency, finite values, energy partition and equilibrium tolerance.
These checks use full exported precision before presentation rounding.

The runner also parses and reevaluates the exported neutral canonical shape
text, checks that its key and geometry are unchanged, and checks that distinct
unloaded material, attachment and region configurations have distinct keys.
Nine invalid grammar configurations exercise mode mixing, legacy controls,
material limits, fractional iteration counts, duplicate names, patch radii,
attachment/region bounds and muscle argument counts without active solves.

The retained native test harnesses additionally exercise an exact one-point
force-balance fixture, energy/force derivatives, stiffness scaling, objectivity
and actual assembled facial material/insertion behavior. Passing numerical
tests demonstrates the specified numerical model; it does not establish
physiological accuracy.

A bounded refinement check of left activation .5 at `segments(96 128)` passed
in 528 iterations with explicit `solve(2000 .0001)`; the default 500-iteration
budget was insufficient for that tested refined case. Maximum displacement
changed from .638582 to .662093 mm (+3.68%), actual insertion shortening from
.168424 to .184552 mm (+9.58%), and the mouth landmark differed by 46.5853 µm.
This is one refinement observation, not a mesh-convergence study or anatomical
calibration. Earlier weak-support trials with maximum shortening .12 reached
rim-motion or iteration-budget limits and were rejected. The allowed input
bounds do not guarantee convergence for every combination.

## Boundaries of this milestone

The material reference is the existing procedural neutral face patch, with
connected lip/lid margins and three openings. The back of the head and skull
volume are absent. The outer boundary and protective eye collars are fixed to
neutral. Consequently, this milestone provides no physical jaw or eyelid
actuator, and the legacy analytic jaw/blink controls cannot be mixed into it.

The solver has no bending, volume, inertia, viscosity or activation dynamics.
Thickness scales in-plane membrane stiffness; it does not create a solid layer
or shell bending stiffness. StVK itself does not prevent reflected triangles.
The native acceptance check requires each moving triangle to stay within the
hemisphere of its own neutral normal, with a positive area margin, and checks
complete triangle-to-eye-globe feasibility during the solve and on the final
float mesh. This is a modest-deformation reference-hemisphere restriction. A
triangle may legitimately turn through the global +Z front plane while staying
close to its neutral orientation; global front projection is not the elastic
acceptance criterion. The hemisphere restriction is not a proof against
self-intersection or a general contact method. A force imbalance
blocked by geometry is reported as unconverged; this is not a constrained
contact-equilibrium solver with contact reactions. Lip self-contact and general
self-collision are not implemented. The prototype also rejects an equilibrium
whose maximum absolute principal Green strain exceeds .35; this is a numerical
operating bound, not a biological injury criterion.

The supporting papers describe distinct components and possible future work:

- [Strain Based Dynamics, Müller et al. (2014)](https://matthias-research.github.io/pages/publications/strainBasedDynamics.pdf)
  provides a material-chart/Green-strain construction. Its constraint dynamics
  are not the static energy minimizer implemented here.
- [XPBD, Macklin et al. (2016)](https://mmacklin.com/xpbd.pdf) concerns compliant
  dynamic constraints. This study does not run XPBD or infer physical time from
  the grammar's `t` variable.
- [Discrete Shells, Grinspun et al. (2003)](https://multires.caltech.edu/pubs/DS-CDROM/)
  describes a discrete bending model relevant to a later shell extension;
  bending is absent from the current implementation.

The illustrated eyes and recessed mouth proxy are native scene primitives for
viewing. They are not additional simulated tissue or a calibrated oral model.
