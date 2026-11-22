# Lyra facial integration

This directory builds and checks a full-character Lyra candidate using the
native `FacialSkin` mechanics. It also renders the exported character and
packages the validation receipts in a self-contained HTML report.

The model remains a synthetic static supported shell with tensile cables.
The material and force formulation are documented in
[the membrane milestone](../FacialSkin/ELASTIC.md) and
[the bending milestone](../FacialSkin/SHELL.md). Character fitting does not
establish anatomical material calibration, physical jaw or eyelid actuation,
dynamic muscle response, or general contact equilibrium.

## Authored and generated files

| File | Purpose |
| --- | --- |
| `reference/Lyra_Procedural_NoMesh.p3d` | Pinned original procedural character and choreography. |
| `reference/Lyra_Active_Hologram.p3d` | Pinned source hologram declaration; its appearance is a renderer concern. |
| `fit_manifest.json` | Declared native face dimensions, rigid placement, anchor correspondences, source-head window and collar configuration. |
| `build_character.py` | Builds the integrated ordinary/hologram grammars using a freshly evaluated native neutral face reference. |
| Generated `Lyra_Procedural_Facial_Elastic.p3d` | Full native character with fitted facial mechanics and the shared choreography clock. |
| Generated `Lyra_Facial_Hologram.p3d` | The same native character geometry with the hologram declaration. |
| Generated `fit_manifest.json` | Authored configuration plus actual source/reference and seam provenance. |

The authored fitting anchors are the original left/right eye centers and
mouth center. Fitting those anchors does not establish exact facial likeness:
the generic facial rest surface changes the jaw/chin silhouette, which the
integration study measures separately. The collar joins coincident component
boundaries; it does not make the whole head one welded mechanical shell.

**The jaw/chin change is substantial.** At head-local Y = 25 mm, the measured
native core-skin section is **41.37 mm wider in total** and its front outline
is **44.42 mm farther forward** than the pinned source. The report presents
the actual section measurements prominently. This is a functional procedural
integration of the generic native face; it does not preserve Lyra's facial
identity or establish anatomical fitting.

The accepted hair envelope also changes explicitly: **636 carrier control
vertices and 21 strand paths** are refitted, with a maximum authored movement
of **21.45 mm**. Whole-cell triangle bounds repair the earlier carrier/skin
intersections. Rear carrier controls, bun and earpieces retain their declared
source geometry; the revised earpiece mounting interface is specified below.

The face uses the same clamped 0–15 second choreography time as the body.
Left and right cable activation each stay in [0, 0.5] and are evaluated as
independent static equilibria at the requested time. They do not add inertia,
activation dynamics, physical jaw motion or eyelid actuation.

## Build and validate the native character

Run from the repository root after installing the normal native build
dependencies:

```sh
make lyra-facial-full-check
make lyra-facial-silhouette
make lyra-facial-timing
```

The full check rebuilds the derived grammar/reference metadata before
validation. The next two targets consume that accepted snapshot without
rebuilding it. Run them in this order so the silhouette and timing receipts
refer to the manifest and native program that validation approved. To perform
only the bounded CI check, use `make lyra-facial-check`; to generate the native
character without validation, use `make lyra-facial-build`.

The build target invokes the grammar builder with the actual native sampler:

```sh
python3 examples/LyraFacialIntegration/build_character.py \
  --sampler build/form-study-sample \
  --output build/lyra-facial-integration

python3 examples/LyraFacialIntegration/integration_check.py \
  --sampler build/form-study-sample \
  --integration-dir build/lyra-facial-integration \
  --mode full
```

The bounded `smoke` mode is used by `lyra-facial-check`. The full mode also
evaluates all 35 selected clip times. The validation JSON is the acceptance
record; a rendered image is not a substitute for its geometry, mechanics,
provenance, and deterministic-evaluation checks.

The silhouette target reads the accepted native captures; it performs no new
geometry generation or mechanical solve. Its equivalent command is:

```sh
python3 examples/LyraFacialIntegration/measure_skin_envelope.py \
  --integration-dir build/lyra-facial-integration
```

This writes the separate `skin_silhouette_sections.json` receipt. It requires
passed `validation.json`, verifies the current manifest and ordinary grammar
hashes, then verifies the exact `source_head_reference.p3d` and
`head_neutral.p3d` source/request/report receipts recorded by that validation.
It measures the actual native triangles and leaves the approved manifest
unchanged. The receipt retains its validation binding and both native buffer
inventories.

For a deliberately prepared **pre-validation** snapshot, the same tool still
accepts explicit `--source-head-report`, `--integrated-head-report`, `--manifest`
and `--output` paths. Only the additional `--attach-to-manifest` flag embeds the
measurement into that manifest; run validation afterward. That flag cannot be
combined with `--integration-dir`. Rebuilding refreshes derived metadata and
does not preserve an optional previously attached measurement. Run validation
again before producing new bound silhouette or timing receipts.

## Render exactly the exported geometry

`render_study.py` accepts a native triangle OBJ and preserves its exported
positions and corner normals. The ordinary and hologram images are rendered
from the same loaded mesh. No expression multiplier, shape smoothing,
displacement, remeshing, subdivision, or imported replacement face is applied.

For a native integrated neutral sample:

```sh
blender -b --python examples/LyraFacialIntegration/render_study.py -- \
  --obj build/lyra-facial-integration/samples/head_neutral.obj \
  --output build/lyra-facial-renders/neutral \
  --case-id neutral --case-label Neutral \
  --camera-offset 0 -1.48 0 \
  --hologram-grammar build/lyra-facial-integration/Lyra_Facial_Hologram.p3d \
  --presentation-time 0 \
  --views front,profile_left,profile_right,three_quarter,mouth,cutaway \
  --width 900 --samples 64
```

The sample paths above follow the integration study's output convention. For
an accepted active sample, use `samples/head_left_smile.obj` or
`samples/head_bilateral_smile.obj`, an explicit case label, and
`--camera-reference-obj` pointing to `samples/head_neutral.obj`. These
head-only samples have identical head placement; the mouth camera remains
fixed. Reported expression displacement removes each actual rigid native
face transform before comparing landmarks. Body/head choreography is not
included in the smile displacement.

Full-character views use `samples/character_neutral.obj` or
`samples/character_bilateral_smile.obj`, with no camera offset and `--views body`.
Their clip times are 0 and 7.6 seconds respectively. Keep geometry time and
`--presentation-time` explicit: presentation time freezes the hologram
material's animation without re-evaluating or deforming the native mesh. Add
`--comparison-obj` with the separately evaluated hologram grammar export to
require identical native position/UV/normal/triangle records before rendering.
The corresponding full-character exports are `samples/hologram_neutral.obj`
and `samples/hologram_bilateral_smile.obj`.

The renderer refuses integrated samples without native facial landmarks and
an accepted mechanical record. The explicit
`--allow-baseline-without-facial-metadata` option is only for a clearly marked
presentation prototype of an older character; its output cannot receive the
report's passed-integration banner.

### Declared rendering profile

The checked pipeline uses Blender's **Cycles CPU** backend. Each run records
the actual Blender version, image size, sample count, color transform,
exposure, native source hashes, imported-position hash, camera, and elapsed
rendering time in `render_manifest.json`.

- Native coordinates retain **+Y up and +Z forward**. Front, profile and
  three-quarter cameras explicitly preserve that up axis.
- The three common directional lights and camera parameters stay fixed.
  The hologram profile adds its explicitly recorded cyan upward projector;
  total lighting therefore differs between the two profiles.
- Denoising is disabled. The current color transform is AgX at exposure zero.
- The ordinary image uses a declared inspection palette keyed by the native
  material names. This palette is recorded; native material names alone do
  not establish renderer parity.
- The hologram adds an external Blender surface-opacity, scan-line, grain,
  edge-emission and glow profile. All values are recorded. It does not claim
  native-editor shader parity or device frame-rate performance.
- Blender rendering time is separate from native grammar evaluation and
  native mechanical solve time. None is substituted for the others.

`--hologram-grammar` is required for a hologram render. The adapter consumes
the actual `# @hologram {...}` JSON declaration in that file, validates all
12 declared parameters, and records the grammar, declaration and profile
hashes. Unsupported additional keys are listed explicitly. No missing
declared parameter is silently replaced with a default.

The Blender adapter uses these declared, reproducible conversions:

| Authored parameter | Effective Blender setting |
| --- | --- |
| `brightness` | Multiplies the complete emission sum. |
| `grain_scale` | 4D noise spatial scale, interpreted per metre. |
| `grain_speed` | Noise W coordinate = seed + speed × presentation time. |
| `grain_strength` | Emission coefficient = 0.01 × authored artist value. |
| `rim_strength` | Fresnel emission coefficient without scaling. |
| `scan_spacing` | Stripe angular frequency = 2π / spacing in metres. |
| `scan_speed` | Phase = −2π × speed × time / spacing; speed interpreted in m/s. |
| `scan_strength` | Stripe emission coefficient without scaling. |
| `pulse_speed` | Emission multiplier = 0.9 + 0.1 cos(2π × speed × time). |
| `bloom_threshold`, `bloom_mix` | Blender Fog Glow threshold and mix. |
| `projector_strength` | Hologram-only directional projector energy = 0.1 × authored artist value. |

Fixed adapter choices include 0.98 Principled / 0.02 Transparent surface,
base emission 0.025, cyan tint, stripe threshold 0.87, Fresnel IOR 1.45 and
glow size 8. The pulse amplitude and artist-unit conversion factors are
renderer conventions, not a physical calibration. This is a path-traced
transparent/emissive presentation; it does not claim native framebuffer
blend-equation parity. The manifest records actual connected shader-node
values, common/projector light settings, background, bounce limits, frozen
time/seed and thread count.

The focused binding regression independently perturbs every authored
parameter through the real parser and reads back production material nodes,
compositor inputs and projector energy. Material parameter nodes must have
a connected path to the surface output:

```sh
blender -b --python examples/LyraFacialIntegration/verify_hologram_profile.py -- \
  --grammar build/lyra-facial-integration/Lyra_Facial_Hologram.p3d \
  --output build/lyra-facial-integration/hologram_validation.json
```

### A real geometric section

The section view intersects triangles with the plane **world x ≤ 0 m** on a
derived mesh copy. It retains original corners exactly and computes edge-plane
intersections linearly. It adds no caps, fabricated skull, tissue volume, or
hidden anatomical mesh. Removed and intersected triangle counts and maximum
intersection-plane error are included in the render receipt.

Use `--section-retain-rules` to list any native source rules whose actual
interior geometry should remain whole, such as the final eye rule. Every
listed rule must exist in the exported `# native_primitive` metadata; a typo
causes an error. The exact list is recorded. Otherwise every object is clipped
by the same plane.

When native cable metadata is present, the section shows its actual origin
and solved insertion axis. A 0.3 mm bevel radius is a display annotation,
not an anatomical muscle thickness. No cable shape is inferred from an image.

## Package the evidence report

After rendering the desired accepted cases:

```sh
python3 examples/LyraFacialIntegration/build_report.py \
  --renders build/lyra-facial-renders \
  --validation build/lyra-facial-integration/validation.json \
  --editor-validation build/lyra-facial-integration/editor_validation.json \
  --baseline-validation build/lyra-facial-integration/baseline_validation.json \
  --hologram-validation build/lyra-facial-integration/hologram_validation.json \
  --silhouette-sections build/lyra-facial-integration/skin_silhouette_sections.json \
  --optimization-validation build/lyra-facial-integration/optimization/validation.json \
  --timing-validation build/lyra-facial-integration/timing_validation.json \
  --fit-manifest build/lyra-facial-integration/fit_manifest.json \
  --output build/lyra-facial-integration/Lyra_Facial_Integration.html
```

The report embeds the actual PNGs and receipts; it makes no external script
or image requests. It offers case and view selection, shows both render
profiles together, and reports double-solver and float-surface residuals
separately. Selecting cases shows discrete native equilibria and does not
interpolate between their vertices.

The acceptance banner requires a passed native receipt and a passed editor
receipt. It reads the top-level boolean `passed` and the explicit
`fullTimelineValidated` field from the native validation receipt. A successful
bounded smoke run remains qualified until the full timeline is validated.
An attached failed baseline receipt prevents a passed-integration banner.
Reports containing hologram images also require a passed graph-consumption
receipt matching their authored profile and renderer source hashes.
Missing receipts and prototype renders are visibly qualified. A failed
validation receipt remains visible as failed.

## Source identity

The native sampler retains `part_N` OBJ object names and emits a
`progen3d.native-primitive.v1` record with source rule, primitive ordinal,
declaration hash, material, shape-key hash, native-buffer hash, source span,
and primary transform. Presentation code uses these records for semantic
selection rather than guessing a head component from its material or its
current object index. The fit manifest and validation receipt retain the
model-specific fitting and seam evidence.

## Editor integration and rejected edits

The editor regression gate uses the real `GrammarCompilationService` and
`SceneRegenerationCoordinator` path, alongside the sampler checks. Its receipt
is written to `build/lyra-facial-integration/editor_validation.json`.
Required behavior is that a native facial solver failure reaches the editor
diagnostic path instead of silently dropping the face. If the editor retains
the last accepted preview after a rejected edit, that preview must carry the
visible stale-state label; it must not be presented as the newly solved state.
The service-level harness does not establish live GUI pixel behavior or
interactive device performance. Its scope remains explicit in the receipt.

The baseline receipt at
`build/lyra-facial-integration/baseline_validation.json` records the existing
native facial/airflow and anatomical regression evidence. Performance
advisories remain separate from correctness gates and from Blender timings.

## Actual geometry contact and opening scope

`geometry_validation.py` reads actual native positions and connectivity. It
checks face/collar/retained-head seam correspondence, consistent boundary
orientation, eye proxy references, aperture rays and cross-component triangle
contacts. Its triangle narrow phase includes noncoplanar segment/plane and
coplanar intersection cases; vertex-only radial clearance cannot pass a
facet-intersection failure.

Only the **actual intended shared seam** may be excluded from new-skin
component contact. Every intersection witness must lie on that measured seam
within 0.1 µm. This is not an exemption for an entire pair of adjacent parts.
Hair carriers and refitted strands must be clear of the new face/collars.

The original positive-X earpiece (source line 239) has a separately declared **revised mounting
interface**. Its native position/normal buffers remain exact; the allowed
mounting triangles are 385, 438, 439, 440, 441 and 494. The current 20 triangle
contacts may shift up to an explicitly selected **0.25 mm assembly allowance**
from the corresponding original native skin-intersection curves; the measured
maximum is 0.240386 mm. Contacts outside that mounting base fail. This is not
unchanged contact geometry, a general contact exemption, or a mechanical
constraint. The receipt retains the actual current intersection witnesses.

Existing contacts between unchanged original accessory components are listed
by source identity only after exact native position/normal equality is
verified. They do not exempt newly introduced facial, collar or fitted-hair
contacts. These bounded pair checks do not prove arbitrary self-contact of
every component or provide a contact reaction solver.

Aperture rays are selected from the actual front/rim interface. Every tested
front ray and its blocker classification remains in the receipt. A finite
lip wall may be visible within the front loop's projection; the separately
tested front/back common aperture must have at least nine unobstructed samples
and a positive central passage to the intended eye or oral interior. The
bilateral native fixture retains four inner-rim occlusions among 19 front
mouth rays, while all 16 common-aperture rays are clear and its central
passage is 0.37939 mm. Foreign head/skin blockers and blockers inside the
common region fail. The same prepared static geometry index is reused for
all selected clip times only after the caller independently verifies its
rigid native geometry/transform identity. This finite ray sampling does not
establish general self-intersection freedom.

## Native implementation and timing receipts

This milestone repairs symbolic option handling for `CurveNetworkSurface`,
adds `GrammarDocument::reexpandAtTime` with a clean runtime state, and propagates
failed primitive construction as a source-located blocking diagnostic. The
editor keeps a visible stale-preview status after rejection and clears both
that status and prior document diagnostics on successful recovery. These
paths are exercised by the actual compiler/regeneration service harness;
no live GUI pixel verification is claimed.

The inactive curve-network basis evaluation optimization was checked against
917 dense oracle samples with zero difference and 143 bit-identical native
head buffers. The measured one-head generation interval changed from
35.1667 to 6.77069 seconds in the recorded loaded environment. That comparison
does not establish interactive playback performance.

`timing_validation.json` separately reports body-only native generation,
face-only native generation, mechanical assembly and exact repeated double
equilibrium solves. Face generation already includes its solve: the separate
assembly/solve timing repeats must not be added to it. Parsing, repeated
expansion, buffer construction/hash, export and Blender rendering remain
separately identified. GPU upload and native drawing are unmeasured in the
supported headless path. Machine and build/load limitations stay in the
receipt; no native FPS claim follows from these timings.
