# Facial shell bending and mouth mesh study

Milestone C adds two independent, opt-in features to native `FacialSkin`:
rest-curvature bending and a new mouth discretization. `shell_study.py` compares
their effects using actual native equilibria. It does not introduce a second
face deformer in Python or JavaScript.

The material, support, cable and equilibrium definitions from
[ELASTIC.md](ELASTIC.md) remain in use. This extension is a reference-chart,
small-strain shell prototype. Its material defaults and muscle paths remain
synthetic, and its aperture creases are not a calibrated biological shell.

## Run and inspect

From the repository root, with the existing native dependencies installed:

```sh
make -j2 build/form-study-sample
python3 examples/FacialSkin/shell_study.py \
  --sampler build/form-study-sample \
  --out build/facial-shell-study
```

For the unpacked repository GLFW library, prefix the Python command with
`LD_LIBRARY_PATH=third_party/debs/glfw/usr/lib/x86_64-linux-gnu`.

The output directory contains `Facial_Shell_Study.html`, `study.json`,
`validation.json`, and all input grammar/native OBJ samples. A successful
required study also writes `Facial_Shell_Study.p3d`, using the demonstrated
quality mesh, unit bending scale and left cable activation .5. The validated
example is retained beside this document.

The inspector shows native triangle edges, semantic landmarks, actual cable
endpoints, bending energy, curvature reconstruction diagnostics and separate
double/float residuals. A dedicated section reports both stronger
weak-foundation candidates, including the exact native rejection if a case
fails. No rejected geometry is displayed as a converged state.

To render the four left-activation comparisons with Blender:

```sh
blender -b --python examples/FacialSkin/render_shell_study.py -- \
  --study build/facial-shell-study \
  --output build/facial-shell-study/Facial_Shell_Study.png
```

The renderer reads the actual exported meshes for `legacy_membrane_left`,
`legacy_shell_left`, `quality_membrane_left` and `quality_shell_left`, together
with their corresponding neutral samples. It does not approximate their
deformation from control values.

## Grammar and compatibility

The new options are:

| Option | Meaning |
| --- | --- |
| `bending(scale)` | Finite dimensionless stiffness multiplier in [0,100]. Default 0 disables bending. Positive values multiply flexural rigidity derived from the existing local material. |
| `mouthMesh(legacy\|quality)` | Default `legacy` preserves the original surface discretization. `quality` selects the deterministic mouth annulus independently of bending. |

Both options require `response(elastic)`, even when `bending(0)` or
`mouthMesh(legacy)` is explicitly written. All legacy smile, jaw, blink and brow
controls must still be zero in elastic mode. Ordinary numeric grammar
expressions, including variables and `t`, are supported by `bending(...)`.
Time selects static material/activation snapshots; it does not create dynamics.

```text
Start -> Face(.5 0)

Face(leftActivation rightActivation) ->
  I(FacialSkin(
    size(.16 .21 .045) segments(48 64) response(elastic)
    bending(1) mouthMesh(quality)
    skinMaterial(50000 .35 .001 1000000) solve(2000 .0001)
    smile(0 0) jawOpen(0) blink(0 0) browRaise(0 0)
    muscle(left_smile .87 .56 34/48 20/64 .055 .08 leftActivation .04 40)
    muscle(right_smile .13 .56 14/48 20/64 .055 .08 rightActivation .04 40)
  ) material(softpinkmatteplaster) 1 1)
```

Existing analytic v1 and membrane v2 specification keys and canonical text are
preserved when the new features are absent. Explicit zero bending with the
legacy mesh also canonicalizes to the unchanged membrane specification.
Positive bending or the quality mesh selects a v3 key that records both
settings. An unloaded canonical round trip and an expression-based bending
round trip are checked by the native study, together with malformed arguments,
unsupported enum values, out-of-range/nonfinite scales and forbidden analytic
mode declarations.

Zero bending bypasses curvature-stencil assembly. It therefore preserves the
old membrane path without imposing new curvature-conditioning requirements on
existing models. The original analytic and membrane studies remain separate
regressions.

## Bending energy

The new energy is added to the existing membrane, support and cable energies:

\[
U=U_{\mathrm{mem}}+U_{\mathrm{sup}}+U_{\mathrm{cable}}+U_{\mathrm{bend}}.
\]

For each active triangle, a local orthonormal rest chart and a connected mesh
neighborhood define a quadratic reconstruction. Its fixed coefficients produce
three vector second derivatives of the current positions. The current central
triangle normal projects these derivatives into the curvature components
`kappa_xx`, `kappa_yy` and `kappa_xy`. The last component is tensor shear, not a
doubled engineering component. The neutral reconstruction supplies `kappa_0`.

With rest area `A`, existing local Young modulus `E`, Poisson ratio `nu`,
thickness `h`, and authored bending scale `s`,

\[
D=s\frac{Eh^3}{12(1-\nu^2)},\qquad
\Delta\kappa=\kappa(x)-\kappa_0,
\]

\[
U_{\mathrm{bend},t}=\frac{AD}{2}
\left[(\Delta\kappa_{xx})^2+(\Delta\kappa_{yy})^2
+2\nu\Delta\kappa_{xx}\Delta\kappa_{yy}
+2(1-\nu)(\Delta\kappa_{xy})^2\right].
\]

`D` has units N·m, curvature has units m⁻¹, and the energy is in joules.
Thickness now affects membrane stiffness linearly and enabled bending
stiffness cubically. Regional Young-modulus scales affect both contributions.
Changing bending scale leaves the membrane material and the neutral shape
unchanged.

The gradient differentiates both the reconstructed derivatives and the current
unit normal. Freezing that normal would give a different force model. The
kernel retains the exact normal derivative, and its numerical checks include
finite differences, rigid transformations, translation invariance, tensor
basis changes and material scaling.

The constitutive and smoothed-curvature construction is informed by Qixin
Liang's [Corotational Hinge-based Thin Plates/Shells](https://arxiv.org/html/2502.10872v7),
particularly the smoothed-shell equations 54–57 (published in 2025; linked
version dated 2 January 2026). The production neighborhood selection and
least-squares reconstruction are adaptations for this procedural face. They
are not a reproduction of the paper's boundary model or a claim that its
validation transfers to facial tissue.

An independent scalar hinge-angle sum was not used: the investigated weighting
showed substantial cylinder-energy dependence on grid orientation. The retained
curvature tensor model is tested against a continuum cylinder energy at
multiple orientations and both diagonal patterns, rather than assigned an
arbitrary global correction factor.

## Reconstruction and boundary diagnostics

Each reconstruction is selected from mesh geometry before deciding whether all
of its participating vertices are fixed. This matters at a boundary: a fixed
central triangle can have a wider neighborhood containing free vertices, and
its coupling must not disappear merely because the first small neighborhood
was fixed. Dedicated native tests exercise this case.

The assembly reports active and fully fixed counts, neighborhood expansion,
patch size, conditioning, polynomial reproduction error, rest curvature and
patch height. A completely fixed neighborhood that cannot be reconstructed
after the bounded expansion is reported separately as
`fixedOnlyUnreconstructableTriangles`, a subset of `fixedOnlyTriangles`.
An active reconstruction failure is diagnosed rather than silently assigned
zero bending stiffness.

The exported `bendingAssembly` record contains:

| Field | Interpretation |
| --- | --- |
| `activeStencils` | Curvature reconstructions that participate in the solve. |
| `fixedOnlyTriangles` | Reported fully fixed neighborhoods. |
| `fixedOnlyUnreconstructableTriangles` | Fully fixed neighborhoods that exhausted reconstruction attempts. |
| `expandedStencils` | Reconstructions requiring a larger connected neighborhood. |
| `maximumPatchVertices`, `maximumNeighborhoodRings` | Largest stencil support used. |
| `maximumReconstructionCondition` | Worst reported conditioning of the reconstruction. |
| `maximumPolynomialReproductionError` | Largest measured reproduction error for the polynomial basis. |
| `maximumRestCurvaturePerM` | Largest measured rest curvature, in m⁻¹. |
| `maximumPatchHeightRatio` | Largest reported out-of-plane patch-height ratio. |

These diagnostics are part of the evidence, not interchangeable measures of
physical validity. In particular, high patch height and a sharp rest crease
with `h |kappa_0|` above one are outside a smooth thin-shell interpretation.
A fitted stencil or a converged face does not establish continuum convergence
at that crease.

## Independent mouth discretization

The quality option replaces the mouth neighborhood with an arc-length-aware
annular discretization and graded radial bands, joined to the surrounding
native grid. It retains the semantic mouth cardinals and changes neither the
authored cable actuation nor the analytical rest-surface recipe. The remaining
face grid and eye apertures keep their existing construction.

It is a separate option because remeshing alone can change force distribution,
rest-area quadrature and the weighted insertion centroid. The study therefore
uses a 2×2 comparison: legacy/quality mesh, each with bending 0/1. Each mesh and
resolution has its own native neutral reference, and topology stability is
tested within that group. Different discretizations are not expected to have
identical vertex counts or pointwise index correspondences.

At 48×64, the same-UV comparison measured a sampled maximum separation of
**2.445 mm** between the legacy and quality piecewise linear rest surfaces.
The measurement uses bidirectional triangle-centroid samples and evaluates the
other triangle surface at the identical material UV; it is not a Hausdorff
distance.
Reusing the analytical surface recipe does not make their finite triangle
interpolants identical. This discretization effect is why the study does not
attribute every cross-mesh difference to bending or treat remeshing as a purely
cosmetic change.

The mesh work improves mouth-front triangle aspect ratios in the tested
configurations; it does not guarantee a larger admissible actuation range.
The existing thin mouth wall is a sharp rest crease, and the neutral-normal
hemisphere restriction remains active. Shading-normal corrections are separate
from the mechanical positions, energies and forces.

The tested 48×64 mouth-front maximum aspect ratio decreased from 84.94 to
11.32; at 96×128 it decreased from 41.82 to 12.32. Aspect means
longest-edge-squared divided by twice triangle area. For the coarse shell
reconstructions, the worst reported condition decreased from 3581.31 to
368.38. These are useful discretization/conditioning improvements. The coarse
quality mesh still reported maximum rest curvature about 6215.71 m⁻¹ and patch
height ratio 1.865, giving `h |kappa_0| ≈ 6.22` at the stated 1 mm thickness.
The legacy values were about 90401.22 m⁻¹ and 2.883. The sharp-crease caveat is
therefore material, even when the force solve converges.

## Study scope and numerical evidence

The required set contains neutral, left activation .5 and right activation .5
for all four combinations at 48×64, plus a quality-shell left-activation case
at 96×128. The baseline material remains `50000 .35 .001 1000000`, cable
maximum shortening .04, cable stiffness 40 N/m, and `solve(2000 .0001)`.

Two deliberately stronger candidates use the same left activation .5 with
support reduced to 10⁴ N/m³ and maximum shortening increased to .12: one on the
legacy mesh and one on the quality mesh, both with bending scale 1. Their
successes and failures are retained independently. A diagnostic rejection does
not fail an otherwise valid required study, but it is visible in the HTML and
recorded verbatim in `validation.json`. Required-case failures still make the
runner return a nonzero status.

Every packaged sample passes native export, topology, area, reference-normal
hemisphere, shading-normal, four-term energy and convergence checks. The
modest-deformation orientation check uses each triangle's own neutral normal;
global +Z projection is not a shell validity test. Float geometry remains
separately checked after conversion from the retained double state.

For an actuated C sample, the study additionally requires

\[
\frac{\|f_{\mathrm{free,final}}\|_2}
     {\|f_{\mathrm{free,initial}}\|_2}\le 10^{-4}.
\]

It records `initialFreeForceL2N`, the ratio of the solver's `forceScaleN` to
that initial load, and whether the numerical normalization floor dominates.
This keeps an enlarged stiffness-based floor from making a loaded state appear
converged. An unloaded neutral sample marks the initial-load ratio as not
applicable. The independent float-surface force residual and position rounding
are still reported; neither is relabeled as the authoritative double residual.

These float residuals refer to the native float32 `Mesh` before the viewer's
presentation rounding. The inspector uses the existing eight-decimal position
and six-decimal normal display convention; validation uses the full native
export rather than those presentation tuples.

The retained kernel tests separately verify bending forces against finite
differences, the `h³` stiffness dependence, zero bending equivalence, neutral
rest curvature and invariance tests. Cylinder and loaded cantilever fixtures
give numerical evidence beyond appearance. The production assembly tests add
connected-boundary coupling and reconstruction invariance. The face study adds
actual parser/evaluator/export behavior. None of these establishes patient
specific anatomy, global convergence or unrestricted shell kinematics.

### Measured reference run

All 13 required cases passed. The table holds left activation at .5 and right
activation at zero, with the baseline material and support specified above.
Displacements use each mesh's own exported neutral reference; cable shortening
and bending energy come from the retained double mechanical state.

| Mouth mesh | Bending scale | Maximum vertex displacement (mm) | Left corner displacement (mm) | Actual cable shortening (mm) | Bending energy (µJ) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Legacy | 0 | 0.638583 | 0.227168 | 0.168424 | 0 |
| Legacy | 1 | 0.223913 | 0.165312 | 0.129545 | 0.196777 |
| Quality | 0 | 0.640310 | 0.257543 | 0.189644 | 0 |
| Quality | 1 | 0.563033 | 0.202825 | 0.171605 | 0.263839 |

The legacy coarse mesh has 3034 vertices/5712 triangles; the quality coarse
mesh has 3410/6432, and the quality 96×128 mesh has 13094/25408. All retained one
connected component and four boundary loops. There were no shading-normal or
reference-hemisphere violations among the packaged states. The largest active
initial-load-relative residual was `9.92163e-5`, with **no active case dominated
by the normalization floor**.

The quality shell's refined left case converged in 283 iterations. Its maximum
exported displacement was 0.528382 mm, versus 0.563033 mm at the coarse
resolution; cable shortening was 0.171616 versus 0.171605 mm, and the left
corner displacement vectors differed by 0.0249853 mm. This is the bounded
refinement observation reported by the study, not an assertion of mesh
independence.

Precision remains observable in these results:

| Quality shell sample | Double relative residual | Native float relative residual | Native float force norm (N) | Maximum native position rounding (nm) |
| --- | ---: | ---: | ---: | ---: |
| 48×64, left .5 | 9.64369e-5 | 0.00212061 | 1.34693e-5 | 4.61084 |
| 96×128, left .5 | 8.16924e-5 | 0.00797059 | 2.56380e-5 | 4.88099 |

The weak-foundation legacy shell converged in 1524 iterations, with initial-load
relative residual `9.54290e-5`, maximum Green strain 0.217905 and maximum
exported displacement 3.84812 mm. The same stronger inputs on the quality mesh
were **rejected after 30 iterations**, with residual `1.87191`, because trial
triangle 6428 crossed its neutral-normal hemisphere limit. The full native
message remains visible in the inspector. Better triangle aspect ratios and
reconstruction conditioning did not make that stronger trial admissible.

## Remaining limits

This is still an open front-face patch with two eye apertures and a mouth
aperture. The skull is a support reference, not a simulated bone volume. The
eye collars and outer attachments remain fixed. There is no physical jaw/lid
actuator, inertia, viscosity, activation dynamics, tissue volume, anatomical
fitting or general self-contact solver.

The energy is a reference-chart, small-strain bending model. It does not supply
a full large-strain metric/thickness update, and its crease treatment is not a
smooth-continuum shell limit. The native geometry gates reject area collapse,
reference-hemisphere violations and eye-globe intersections. A trial blocked
by these gates is not accepted as constrained contact equilibrium: unresolved
free force is reported honestly. The refined face is one bounded resolution
observation, not proof of mesh convergence.
