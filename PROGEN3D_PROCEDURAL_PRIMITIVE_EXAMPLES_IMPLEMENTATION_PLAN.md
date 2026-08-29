# ProGen3D Procedural Primitive Examples Implementation Plan

## Document Control

- **Date:** August 21, 2026
- **Status:** Implementation plan; the comprehensive example library described here has not yet been written.
- **Primary objective:** Create a structured, comprehensive, executable example library for the canonical `Cylinder`, `Sphere`, and `AxialProfile` procedural families.
- **Teaching objective:** Progress from one-concept examples to comparisons, temporal behavior, and finished gallery scenes without turning the library into a combinatorial list of every parameter permutation.
- **Compatibility requirement:** Preserve the existing top-level showcase paths and all current grammar, rendering, export, physics, and editor behavior.
- **Release authority:** Catalog validation, grammar-to-scene execution, deterministic geometry evidence, selected GUI smoke checks, the full release gate, and supervised launch of a representative gallery scene.

## 1. Executive Decision

The example library should be treated as a maintained product surface rather than a directory of informal snippets.

The implementation shall separate five responsibilities:

```text
tutorial examples
comparison examples
temporal examples
gallery examples
negative diagnostic fixtures
```

Public examples must always open successfully. Expected failures belong only in `tests/fixtures/procedural_primitives/invalid`.

Comprehensiveness will be measured by **coverage of orthogonal language dimensions**:

```text
family
domain restriction
topology
closure
profile or clip construction
transform interaction
tessellation
mapping
temporal evaluation
physics evidence
alias equivalence
diagnostics
```

The project should not create one public file for every Cartesian-product combination. Instead:

1. one-concept tutorial files establish the language;
2. comparison scenes explain distinctions that are difficult to learn from text;
3. gallery scenes show useful composition;
4. deterministic fixtures enforce edge conditions and failures.

## 2. Confirmed Repository Baseline

### 2.1 Existing Public Examples

The current public example surface contains:

- `examples/curved_primitives_showcase.p3d`, a combined Cylinder and Sphere grid;
- `examples/curved_primitives_temporal.p3d`, one time-varying Cylinder and one time-varying Sphere;
- `examples/axial_profile_v1_showcase.p3d`, one combined AxialProfile object;
- `examples/P2_time_showcase.p3d`, the inherited general temporal grammar example.

The combined curved showcase already demonstrates:

- full Cylinder and Sphere;
- open surface domains;
- Tube, CylinderSector, DSection, and HalfCylinder aliases;
- shell sectors;
- spherical shells;
- Hemisphere, SphereCap, SphereBowl, SphereQuarter, and SphereOctant aliases;
- sphere slabs.

Evidence: `examples/curved_primitives_showcase.p3d:1`.

The temporal curved showcase demonstrates:

- a time-varying Cylinder azimuth sweep;
- a time-varying Sphere clip offset;
- preservation of the design nonce across temporal sampling.

Evidence: `examples/curved_primitives_temporal.p3d:1`.

### 2.2 Existing AxialProfile Fixtures

The repository contains isolated AxialProfile fixtures for:

- rectangle extrusion;
- linear taper;
- contained step;
- offset, scaled, rotated crown;
- temporal center and rotation expressions.

Evidence: `tests/fixtures/axialprofile/rectangle_extrusion.p3d:1`, `tests/fixtures/axialprofile/taper.p3d:1`, `tests/fixtures/axialprofile/step.p3d:1`, `tests/fixtures/axialprofile/offset_crown.p3d:1`, and `tests/fixtures/axialprofile/temporal.p3d:1`.

These fixtures are intentionally terse. They are suitable as regression inputs but are not a complete teaching sequence.

### 2.3 Documented Canonical Option Surfaces

Cylinder currently documents:

```text
radial
wall
axial
azimuth
chord
clip
topology
close
segments
mapping
```

Sphere currently documents:

```text
radial
wall
polar
azimuth
clip
slab
topology
close
segments
mapping
```

AxialProfile currently documents:

```text
axis
profile
at
hold
linear
step
cap
center
scale
rotate
```

Evidence: `src/geometry/service/ProceduralShapeCatalogRepository.cpp:21`, `src/geometry/service/ProceduralShapeCatalogRepository.cpp:41`, and `src/geometry/service/ProceduralShapeCatalogRepository.cpp:61`.

### 2.4 Existing Verification Infrastructure

The repository already provides:

- grammar and semantic harnesses;
- procedural mesh topology and volume tests;
- preview/export agreement tests;
- editor catalog and completion tests;
- Xvfb GUI smoke tests;
- temporal GUI smoke tests;
- a clean full release gate.

The example project should extend these systems rather than introduce a separate parser or renderer.

## 3. Scope

### 3.1 P0 Delivery

The first complete delivery includes:

- a stable example directory hierarchy;
- a machine-readable public example catalog;
- a human-readable example index;
- one-concept tutorial sequences for all three canonical families;
- side-by-side semantic comparison scenes;
- one temporal sequence for each family plus combined temporal scenes;
- finished gallery scenes using multiple families;
- valid and invalid deterministic fixtures;
- automated catalog, grammar, geometry, and GUI gates;
- preservation of existing top-level showcase paths.

### 3.2 Explicit Non-Goals

P0 does not require:

- new Cylinder, Sphere, or AxialProfile syntax;
- new mesh algorithms;
- automatic screenshot approval;
- general visual-difference testing;
- online example downloads;
- an editor example browser;
- per-example cloud metadata;
- arbitrary user-authored package installation;
- certification of the external Component 24 reconstruction.

An editor example browser is a useful P1 extension, but it must not delay the example library itself.

## 4. Directory Architecture

Create:

```text
examples/
  procedural_primitives/
    README.md
    catalog.json

    cylinder/
      01_full_cylinder.p3d
      ...

    sphere/
      01_full_sphere.p3d
      ...

    axial_profile/
      01_rectangle_extrusion.p3d
      ...

    comparisons/
      01_normalized_scaling.p3d
      ...

    temporal/
      01_cylinder_sweep.p3d
      ...

    gallery/
      01_observatory.p3d
      ...

tests/fixtures/procedural_primitives/
  valid/
    cylinder/
    sphere/
    axial_profile/
  invalid/
    cylinder/
    sphere/
    axial_profile/
```

Existing files remain valid compatibility entry points:

```text
examples/curved_primitives_showcase.p3d
examples/curved_primitives_temporal.p3d
examples/axial_profile_v1_showcase.p3d
```

They may be listed in the new catalog, but they must not be moved or renamed while current tests and user workflows reference them.

## 5. Public Example Contract

Every public `.p3d` file begins with a compact contract header:

```p3d
// Example: Cylinder / Angular Sector
// Purpose: Demonstrate a closed solid with a restricted azimuth domain.
// Primary features: azimuth, topology(solid), close(all).
// Expected result: One watertight partial cylinder with two radial cut faces.
// Deterministic: yes.
```

Required authoring rules:

1. Teach one primary concept per tutorial file.
2. Use canonical descriptors before introducing aliases.
3. Use purpose-revealing rule and profile names.
4. Keep meaningful polygon profiles vertically formatted.
5. Use normalized shape values and let `S(...)` own visible external size.
6. Avoid random expressions in deterministic examples.
7. Place time-dependent expressions only in catalog entries marked temporal.
8. Use materials that preserve readable silhouettes and cut surfaces.
9. Space comparison instances far enough to avoid overlap at supported FOV values.
10. Produce no grammar or runtime diagnostic.
11. Stay below ordinary geometry safety ceilings without relying on exceptional limits.
12. Do not use comments to claim watertightness, collision support, or volume unless the corresponding automated evidence exists.

## 6. Public Catalog Model

Create `examples/procedural_primitives/catalog.json` with this bounded schema:

```json
{
  "schema": "progen3d.procedural-example-catalog.v1",
  "examples": [
    {
      "id": "cylinder.angular-sector",
      "title": "Angular Sector",
      "path": "cylinder/07_angular_sector.p3d",
      "category": "tutorial",
      "families": ["Cylinder"],
      "difficulty": "beginner",
      "features": [
        "cylinder.azimuth",
        "topology.solid",
        "closure.all"
      ],
      "deterministic": true,
      "temporal": false,
      "expected_minimum_instances": 1,
      "documentation": "Closed angular restriction with generated cut faces."
    }
  ]
}
```

### 6.1 Catalog Invariants

- schema identifier is exact;
- IDs are unique and stable;
- paths are unique, relative, normalized, and remain below the example root;
- every path exists;
- every public `.p3d` below the new root appears exactly once;
- family values are `Cylinder`, `Sphere`, or `AxialProfile`;
- category values are `tutorial`, `comparison`, `temporal`, or `gallery`;
- difficulty values are `beginner`, `intermediate`, or `advanced`;
- temporal entries set `temporal=true`;
- deterministic temporal entries mean deterministic at an explicit `t` and design nonce;
- every documented canonical option has at least one feature-tagged example;
- every public alias has an alias example and canonical-equivalence fixture;
- invalid fixtures are never included in the public catalog.

### 6.2 Catalog Ownership

P0 validation may use Python's standard JSON parser in a test script. Runtime code does not need to parse the catalog.

If P1 adds an editor browser, introduce purpose-specific object-model classes:

```cpp
class ProceduralExampleDefinition;
class ProceduralExampleCategory;
class ProceduralExampleCatalog;
class ProceduralExampleCatalogReader;
class ProceduralExampleSelection;
```

Do not add a vague generic JSON utility or reuse the geometry catalog as an unrelated example repository.

## 7. Coverage Model

The catalog validator shall build a coverage table for:

| Dimension | Cylinder | Sphere | AxialProfile |
|---|---:|---:|---:|
| Bare legacy form | Required | Required | Not applicable |
| Normalized external scaling | Required | Required | Required |
| Surface form | Required | Required | Not in v1 |
| Solid form | Required | Required | Required |
| Shell form | Required | Required | Post-v1 |
| Domain restriction | Required | Required | Axial levels |
| Clip or step boundary | Required | Required | Required |
| Open form | Required | Required | Required |
| Fully closed form | Required | Required | Required |
| Mapping policy | Required | Required | Triplanar only |
| Tessellation control | Required | Required | Profile-authored |
| Temporal expression | Required | Required | Required |
| Alias equivalence | Required | Required | No aliases in v1 |
| Density acceptance | Representative | Representative | Representative |
| Density rejection | Representative | Representative | Representative |

Coverage is feature-based. A single comparison scene may satisfy several presentation goals, but focused fixtures remain responsible for independent correctness evidence.

## 8. Cylinder Tutorial Inventory

Create fourteen Cylinder tutorial files.

### C01 `01_full_cylinder.p3d`

Demonstrate:

```p3d
I(Cylinder material(plaster))
```

Gate:

- backwards-compatible bare form;
- normalized radius `0.5` and local-Y interval `0..1`;
- closed solid;
- analytic volume evidence.

### C02 `02_elliptical_cylinder.p3d`

Demonstrate nonuniform `S(...)` creating an elliptical cylinder without changing the local descriptor.

Gate:

- correct transformed bounds;
- inverse-transpose normal handling;
- transformed volume determinant.

### C03 `03_axial_interval.p3d`

Demonstrate `axial(min max)` as a normalized local-Y domain restriction, not external height control.

### C04 `04_open_surface.p3d`

Demonstrate:

```text
topology(surface)
close(none)
```

Gate:

- expected boundary edges;
- no enclosed-volume claim;
- density rejection fixture exists separately.

### C05 `05_radial_shell.p3d`

Demonstrate a closed tube through:

```text
radial(inner outer)
topology(shell)
close(all)
```

### C06 `06_wall_thickness.p3d`

Compare `wall(0.25)` with `radial(0.75 1)` and explain normalized wall semantics.

### C07 `07_angular_sector.p3d`

Demonstrate a closed solid sector with a sweep not greater than 180 degrees.

### C08 `08_reflex_sector.p3d`

Demonstrate a closed sector greater than 180 degrees and explain why visual closure does not imply convex dynamic collision support.

### C09 `09_tube_sector.p3d`

Combine shell and azimuth restriction. Expose outer, inner, axial, angular, and rim boundaries.

### C10 `10_half_cylinder.p3d`

Show the canonical centre chord first, then the `HalfCylinder` alias.

### C11 `11_d_section.p3d`

Demonstrate an offset chord and compare positive and negative retained sides.

### C12 `12_oblique_plane_clip.p3d`

Demonstrate a non-axial 3D plane clip and its generated closure face.

### C13 `13_multiple_clips.p3d`

Demonstrate deterministic ordered intersection of more than one half-space.

### C14 `14_mapping_and_segments.p3d`

Place low, default, and high tessellation variants beside cylindrical and triplanar mappings.

The comments must explain that segment count controls approximation density, not semantic dimensions.

## 9. Sphere Tutorial Inventory

Create sixteen Sphere tutorial files.

### S01 `01_full_sphere.p3d`

Demonstrate the bare backwards-compatible form and analytic full-sphere volume.

### S02 `02_ellipsoid.p3d`

Demonstrate nonuniform scaling of the normalized sphere.

### S03 `03_surface_sphere.p3d`

Demonstrate a zero-thickness mathematical skin and undefined density volume.

### S04 `04_radial_shell.p3d`

Demonstrate a closed spherical shell using `radial(inner outer)`.

### S05 `05_wall_thickness.p3d`

Compare `wall(thickness)` with its radial interval equivalent.

### S06 `06_polar_sector.p3d`

Demonstrate a solid polar restriction with center-directed radial boundaries.

### S07 `07_spherical_zone_surface.p3d`

Demonstrate a surface-only latitude zone.

### S08 `08_azimuth_wedge.p3d`

Demonstrate a restricted azimuth solid with angular closure.

### S09 `09_spherical_patch.p3d`

Combine polar and azimuth restrictions in a surface-only patch.

### S10 `10_hemisphere.p3d`

Show the canonical center-plane clip and the `Hemisphere` alias.

### S11 `11_sphere_caps.p3d`

Compare positive and negative sides at positive, zero, and negative offsets.

### S12 `12_polar_vs_plane_cap.p3d`

Place a polar restriction beside a visually similar plane-cut cap.

This is a required semantic comparison because their internal boundaries and volume differ.

### S13 `13_spherical_segment.p3d`

Demonstrate `slab(...)` as two ordered parallel clips.

### S14 `14_quarter_and_octant.p3d`

Show canonical multi-clip forms beside `SphereQuarter` and `SphereOctant` aliases.

### S15 `15_open_and_closed_bowls.p3d`

Compare:

```text
open shell bowl
clip-closed shell
clip-and-rim-closed shell
```

The example must label material closure rather than relying on visual appearance alone.

### S16 `16_mapping_and_segments.p3d`

Compare low, default, and high segment counts with spherical and triplanar mapping.

## 10. AxialProfile Tutorial Inventory

Create thirteen AxialProfile tutorial files.

### A01 `01_rectangle_extrusion.p3d`

Teach `axis`, `profile`, `at`, `hold`, and `cap(all)` with one rectangle.

### A02 `02_triangle_extrusion.p3d`

Demonstrate the minimum valid profile vertex count.

### A03 `03_concave_profile.p3d`

Demonstrate deterministic ear-clipped caps on one simple concave polygon.

The polygon must remain non-self-intersecting and avoid nearly collinear edges.

### A04 `04_linear_taper.p3d`

Demonstrate two profiles with preserved semantic vertex correspondence.

### A05 `05_multistage_loft.p3d`

Use several explicit linear states to show a piecewise ruled surface.

### A06 `06_inward_step.p3d`

Demonstrate a strictly contained smaller replacement profile.

### A07 `07_outward_step.p3d`

Demonstrate a strictly containing larger replacement profile.

### A08 `08_multistep_pedestal.p3d`

Combine inward and outward contained steps without crossing contours.

### A09 `09_center_drift.p3d`

Demonstrate successive section-center offsets.

### A10 `10_scale_drift.p3d`

Demonstrate nonzero, orientation-preserving per-section scaling.

### A11 `11_rotated_tower.p3d`

Use multiple explicit rings with progressive rotation:

```p3d
at(0 Square rotate(0))
linear(1 Square rotate(10))
linear(2 Square rotate(20))
linear(3 Square rotate(30))
linear(4 Square rotate(40))
```

The comments must state that v1 does not provide implicit smooth interpolation.

### A12 `12_combined_section_transforms.p3d`

Combine center, scale, and rotation on an asymmetric profile.

### A13 `13_cap_policies.p3d`

Place `cap(all)`, `cap(bottom)`, `cap(top)`, and `cap(none)` variants side by side.

## 11. Cross-Family Comparison Inventory

Create seven comparison scenes.

### X01 `01_normalized_scaling.p3d`

Compare:

```text
Cylinder + nonuniform S
Sphere + nonuniform S
AxialProfile + nonuniform S
```

Explain descriptor-owned topology versus scope-owned external dimensions.

### X02 `02_surface_solid_shell.p3d`

Compare Cylinder and Sphere surface, solid, and shell forms. Explicitly state that hollow AxialProfile is post-v1.

### X03 `03_closure_anatomy.p3d`

Expose axial caps, angular faces, clip faces, shell rims, AxialProfile caps, and AxialProfile step ledges.

### X04 `04_domain_vs_clip.p3d`

Compare:

- Cylinder sector versus chord segment;
- Sphere polar restriction versus plane-cut cap.

### X05 `05_wall_vs_radial.p3d`

Demonstrate equivalent Cylinder and Sphere shell specifications.

### X06 `06_alias_equivalence.p3d`

Place each readable alias beside its canonical descriptor:

```text
Tube
CylinderSector
DSection
HalfCylinder
Hemisphere
SphereCap
SphereBowl
SphereQuarter
SphereOctant
```

Automated tests, not visual similarity, remain the equivalence authority.

### X07 `07_tessellation_levels.p3d`

Compare low, default, and high tessellation across Cylinder and Sphere while showing AxialProfile authored profile density.

## 12. Temporal Example Inventory

Create six temporal files.

### T01 `01_cylinder_sweep.p3d`

Animate azimuth sweep while preserving closure.

### T02 `02_cylinder_clip.p3d`

Animate chord or plane-clip offset through a bounded valid range.

### T03 `03_sphere_cap.p3d`

Animate plane-cut cap offset without crossing tangent-invalid states.

### T04 `04_sphere_wedge.p3d`

Animate azimuth or polar domain in a bounded interval.

### T05 `05_axial_profile_center.p3d`

Animate one target section's center offset.

### T06 `06_axial_profile_rotation.p3d`

Animate a target rotation or scale while retaining the same profile topology.

Temporal gates sample at least:

```text
t = 0
t = 0.5
t = 1
t = 2
```

The design nonce remains unchanged between samples. Geometry changes only through explicit evaluated expressions.

## 13. Gallery Inventory

Create six polished but deterministic gallery scenes.

### G01 `01_observatory.p3d`

Use:

- AxialProfile base and tower;
- Hemisphere or SphereCap dome;
- Cylinder-sector shutters or supports.

### G02 `02_rocket_engine.p3d`

Use:

- AxialProfile nozzle or staged body;
- tube walls;
- clipped cylindrical mounting surfaces.

### G03 `03_architectural_tower.p3d`

Use a multistage AxialProfile shaft, contained ledges, asymmetric crown, and spherical roof form.

### G04 `04_industrial_valve.p3d`

Use tubes, D-sections, sphere segments, and an AxialProfile body.

### G05 `05_planetary_station.p3d`

Use spherical shells, bowls, cylinder sectors, and axial towers.

### G06 `06_procedural_chess_set.p3d`

Use AxialProfile for turned-like silhouettes, Cylinder for bases, and Sphere for rounded heads.

Gallery rules:

- no undocumented grammar;
- no imported STL dependency;
- deterministic by default;
- readable rule decomposition;
- ordinary primitive and geometry budgets;
- one scene must remain suitable as the supervised startup showcase.

## 14. Negative Fixture Inventory

Negative fixtures are required evidence, not public examples.

### 14.1 Cylinder Invalid Cases

Create fixtures for:

- inner radius with `topology(solid)`;
- shell with zero inner radius;
- reversed radial interval;
- zero outer radius;
- zero or invalid axial interval;
- zero or invalid azimuth sweep;
- tangent or outside chord degeneracy where unsupported;
- invalid clip normal;
- invalid segment count;
- density requested on open geometry.

### 14.2 Sphere Invalid Cases

Create fixtures for:

- solid topology with positive inner radius;
- shell with zero inner radius;
- reversed polar interval;
- invalid azimuth sweep;
- zero clip normal;
- contradictory or empty clips;
- tangent degeneration where unsupported;
- invalid slab ordering;
- invalid segment count;
- density requested on open geometry.

### 14.3 AxialProfile Invalid Cases

Create fixtures for:

- fewer than three vertices;
- duplicate or zero-length edges;
- collinear profile triples;
- zero signed area;
- self-intersection;
- duplicate profile names;
- undefined profile reference;
- mismatched profile vertex counts;
- missing initial `at`;
- non-increasing non-step levels;
- crossing step contours;
- touching step contours;
- coincident step contours;
- disjoint step contours;
- zero scale;
- orientation-reversing scale;
- non-finite expression evaluation;
- resource ceiling violations.

Each negative fixture must assert a stable diagnostic category or meaningful message fragment. It is not sufficient to assert only that generation failed.

## 15. Example Documentation

Create `examples/procedural_primitives/README.md` containing:

1. the recommended learning sequence;
2. a table generated or validated against `catalog.json`;
3. commands for opening examples;
4. explanations of normalized geometry;
5. links to `docs/CURVED_PRIMITIVES.md` and `docs/axial_profile_v1.md`;
6. the distinction between public examples and test fixtures;
7. temporal sampling guidance;
8. limitations of AxialProfilev1;
9. instructions for contributing a new example.

Example launch command:

```bash
./progen3d-editor-gui \
    --open examples/procedural_primitives/cylinder/07_angular_sector.p3d
```

Update `BUILDING.md` with:

- the new example index path;
- commands for the catalog and example suites;
- one representative GUI smoke command;
- the authoritative release gate.

## 16. Automated Validation Architecture

### 16.1 Catalog Validator

Add:

```text
tests/validate_procedural_example_catalog.py
```

Responsibilities:

- parse the bounded JSON schema;
- reject duplicate IDs and paths;
- reject missing files;
- reject unlisted public examples;
- enforce category, family, difficulty, determinism, and temporal enums;
- build a feature-coverage report;
- require every canonical option and public alias to be covered;
- ensure invalid fixtures are outside the public catalog;
- validate the required comment header fields.

Output a deterministic summary such as:

```text
Cylinder tutorials: 14
Sphere tutorials: 16
AxialProfile tutorials: 13
Comparisons: 7
Temporal examples: 6
Gallery examples: 6
Canonical option coverage: complete
Alias coverage: complete
```

### 16.2 Grammar-to-Scene Harness

Add:

```text
tests/procedural_example_scene_harness.cpp
tests/run_procedural_example_scene_checks.sh
```

For every public example:

1. load source from the catalog path;
2. parse and validate grammar;
3. generate the scene with a fixed design nonce;
4. require zero diagnostics;
5. require at least the catalog minimum instance count;
6. require finite scene bounds;
7. resolve every procedural mesh;
8. require nonempty generated geometry;
9. require preview/export triangle agreement;
10. rerun and require deterministic shape and topology hashes.

The harness should report the failing catalog ID, path, family, and stage.

### 16.3 Representative Geometry Evidence

Do not hard-code exact topology hashes for every public teaching file. That would make harmless presentation edits unnecessarily expensive.

Maintain exact evidence for representative fixtures:

| Family | Required evidence |
|---|---|
| Cylinder | full volume, tube volume, sector volume, chord topology, open-boundary behavior |
| Sphere | full volume, shell volume, hemisphere volume, cap/slab topology, open bowl boundary behavior |
| AxialProfile | extrusion volume, frustum volume, inward/outward step volume, cap policy, deterministic triangulation |

### 16.4 Alias Equivalence Harness

For every public alias:

1. parse alias syntax;
2. parse its canonical descriptor;
3. evaluate both in the same lexical environment;
4. compare canonical shape keys;
5. compare mesh topology hashes;
6. compare bounds and volume evidence.

### 16.5 Temporal Harness

For each temporal catalog entry:

- evaluate the required sample times;
- preserve the design nonce;
- require finite evaluated values;
- require valid shape specifications;
- require geometry within budgets;
- require no unexpected diagnostic;
- verify the intended descriptor component changes;
- verify unrelated instance appearance and source association remain stable.

### 16.6 GUI Smoke Selection

Running every example under a separate GUI process would be expensive and redundant.

Select representative GUI smoke documents:

```text
cylinder/13_multiple_clips.p3d
sphere/15_open_and_closed_bowls.p3d
axial_profile/12_combined_section_transforms.p3d
comparisons/06_alias_equivalence.p3d
temporal/06_axial_profile_rotation.p3d
gallery/03_architectural_tower.p3d
```

Each GUI smoke must verify:

- startup document load;
- initial scene generation;
- one complete rendered preview frame;
- FOV smoke values;
- clean shutdown;
- temporal sample publication where applicable.

## 17. Test Scripts

Add:

```text
tests/run_procedural_example_catalog_checks.sh
tests/run_procedural_example_scene_checks.sh
tests/run_procedural_example_alias_checks.sh
tests/run_procedural_example_temporal_checks.sh
tests/run_procedural_example_gui_checks.sh
tests/run_procedural_example_checks.sh
```

`run_procedural_example_checks.sh` runs the complete focused set.

Wire it into:

```text
tests/run_editor_architecture_checks.sh
tests/run_release_checks.sh
```

CI order:

```text
catalog schema
source/header rules
grammar-to-scene execution
representative geometry evidence
alias equivalence
temporal sampling
selected GUI smoke
diff check
```

## 18. Optional P1 Editor Integration

After the file library and automated gates are stable, add an editor example browser.

Recommended object model:

```text
ProceduralExampleCatalog
    aggregates ProceduralExampleDefinition

ProceduralExampleCatalogReader
    reads the bounded catalog schema

ProceduralExampleSelection
    identifies the chosen example

ExampleLibraryPanel
    presents family, category, difficulty, and feature filters

DocumentPersistenceService
    remains responsible for reading the selected source
```

User actions:

```text
browse by family
filter by feature
preview description
open as a new document
locate source file
```

Opening an example must never overwrite a dirty document without the existing explicit document workflow.

Optional thumbnails must be generated from the actual renderer and carry source and binary hashes. They are navigation aids, not correctness evidence.

## 19. File-Level Implementation Sequence

### New Public Content

```text
examples/procedural_primitives/README.md
examples/procedural_primitives/catalog.json
examples/procedural_primitives/cylinder/*.p3d
examples/procedural_primitives/sphere/*.p3d
examples/procedural_primitives/axial_profile/*.p3d
examples/procedural_primitives/comparisons/*.p3d
examples/procedural_primitives/temporal/*.p3d
examples/procedural_primitives/gallery/*.p3d
```

### New Test Content

```text
tests/fixtures/procedural_primitives/valid/**
tests/fixtures/procedural_primitives/invalid/**
tests/validate_procedural_example_catalog.py
tests/procedural_example_scene_harness.cpp
tests/procedural_example_alias_harness.cpp
tests/procedural_example_temporal_harness.cpp
tests/run_procedural_example_catalog_checks.sh
tests/run_procedural_example_scene_checks.sh
tests/run_procedural_example_alias_checks.sh
tests/run_procedural_example_temporal_checks.sh
tests/run_procedural_example_gui_checks.sh
tests/run_procedural_example_checks.sh
```

### Modified Files

```text
BUILDING.md
tests/README.md
tests/run_editor_architecture_checks.sh
tests/run_release_checks.sh
```

Optional P1 editor work adds:

```text
include/editor/model/ProceduralExampleDefinition.h
include/editor/model/ProceduralExampleCatalog.h
include/editor/model/ProceduralExampleSelection.h
include/editor/service/ProceduralExampleCatalogReader.h
include/editor/presentation/ExampleLibraryPanel.h

src/editor/service/ProceduralExampleCatalogReader.cpp
src/editor/presentation/ExampleLibraryPanel.cpp
```

## 20. Milestones

### Milestone 0: Freeze Coverage Contract

Deliver:

- final catalog schema;
- feature-tag vocabulary;
- tutorial/comparison/temporal/gallery definitions;
- required canonical option and alias coverage matrix;
- positive versus negative file boundary.

Gate:

- every currently documented option and alias maps to at least one planned example;
- no unsupported post-v1 capability is represented as available.

### Milestone 1: Catalog and Harness Skeleton

Deliver:

- directory hierarchy;
- catalog validator;
- grammar-to-scene harness;
- focused aggregate test script;
- README skeleton.

Gate:

- existing top-level showcases can be cataloged and pass through the new harness;
- empty or duplicate catalog cases fail deterministically.

### Milestone 2: Cylinder Curriculum

Deliver all fourteen Cylinder tutorial files and corresponding valid/invalid fixtures.

Gate:

- complete Cylinder option coverage;
- Tube, CylinderSector, DSection, and HalfCylinder canonical equivalence;
- representative analytic volume and topology evidence;
- public Cylinder examples produce zero diagnostics.

### Milestone 3: Sphere Curriculum

Deliver all sixteen Sphere tutorial files and corresponding valid/invalid fixtures.

Gate:

- complete Sphere option coverage;
- Hemisphere, SphereCap, SphereBowl, SphereQuarter, and SphereOctant equivalence;
- polar-versus-clip comparison exists;
- representative volume, topology, and open-boundary evidence passes.

### Milestone 4: AxialProfile Curriculum

Deliver all thirteen AxialProfile tutorial files and corresponding invalid fixtures.

Gate:

- extrusion, concave cap, taper, multistage loft, inward step, outward step, center, scale, rotation, and cap policies pass;
- public examples respect v1 equal-vertex correspondence and strict step containment;
- no example implies smooth interpolation or hollow-profile support.

### Milestone 5: Comparison and Temporal Sets

Deliver seven comparison and six temporal files.

Gate:

- semantic distinctions are visible and documented;
- all required temporal samples pass with the same design nonce;
- aliases compare exactly with canonical descriptors.

### Milestone 6: Gallery Set

Deliver six deterministic gallery scenes.

Gate:

- every gallery opens with zero diagnostics;
- scenes remain below ordinary budgets;
- at least one gallery uses all three canonical families;
- one gallery passes supervised runtime observation.

### Milestone 7: Diagnostic Completion

Deliver the complete invalid fixture matrix and diagnostic assertions.

Gate:

- every locked semantic invariant has at least one failing fixture;
- failure atomicity preserves the last valid scene;
- diagnostics name the relevant family and option or statement.

### Milestone 8: Documentation and Release

Deliver:

- complete example index;
- contribution instructions;
- BUILDING and test documentation;
- release integration;
- final catalog coverage report.

Run:

```bash
./tests/run_procedural_example_checks.sh
./tests/run_editor_architecture_checks.sh
./tests/run_release_checks.sh
git diff --check
```

Only after a clean pass:

1. record the exact example catalog SHA-256;
2. record the representative gallery SHA-256;
3. record the validated editor binary SHA-256 if application code changed;
4. restart `progen3d-editor-gui.service` with the representative gallery;
5. verify `ActiveState=active` and `SubState=running`;
6. verify `NRestarts=0` after observation;
7. inspect recent logs for grammar or geometry errors.

## 21. Acceptance Gates

### 21.1 Catalog

- every public example is cataloged exactly once;
- every catalog path exists;
- IDs and paths are unique;
- every canonical family has beginner, intermediate, and advanced entries;
- every canonical option is feature-covered;
- every public alias is represented;
- catalog validation is deterministic.

### 21.2 Public Examples

- zero grammar diagnostics;
- zero runtime geometry diagnostics;
- at least one generated procedural instance;
- finite bounds;
- nonempty mesh geometry;
- deterministic evaluated shape keys at fixed inputs;
- deterministic topology hashes at fixed inputs;
- preview/export geometry agreement;
- valid temporal behavior for temporal entries.

### 21.3 Geometry Evidence

- closed examples claimed as watertight pass topology analysis;
- open examples retain only expected boundary categories;
- representative analytic volumes match within existing tolerances;
- no degenerate triangles;
- no unexpected nonmanifold edges;
- aliases resolve to the same canonical geometry as their expansions.

### 21.4 Documentation

- every example has the required header;
- the README learning order matches the catalog;
- limitations are explicit;
- public examples do not contain expected failures;
- test fixtures are not presented as gallery content;
- no screenshot or thumbnail is treated as correctness authority.

### 21.5 Compatibility

- existing top-level showcase paths still open;
- current Cylinder and Sphere grammar remains unchanged;
- current AxialProfile grammar remains unchanged;
- existing materials, alpha, texture scale, transforms, and temporal functions remain unchanged;
- all inherited release tests continue to pass.

## 22. Definition of Complete

The comprehensive procedural primitive example set is complete when:

```text
catalog schema is stable
public directory hierarchy exists
14 Cylinder tutorials pass
16 Sphere tutorials pass
13 AxialProfile tutorials pass
7 comparison scenes pass
6 temporal scenes pass
6 gallery scenes pass
canonical option coverage is complete
alias equivalence is proven
negative diagnostic coverage is complete
all public examples generate zero diagnostics
preview and export agree
representative GUI smoke checks pass
documentation and contribution guidance are complete
the full release gate passes
the supervised representative gallery remains stable
```

The optional editor example browser is not required for P0 completion. It becomes a separate promotion once the catalog and files have demonstrated stable ownership and test coverage.

## 23. Highest-Gain Implementation Order

The critical path is:

```text
catalog contract
    ↓
catalog and grammar-to-scene harness
    ↓
Cylinder tutorials and fixtures
    ↓
Sphere tutorials and fixtures
    ↓
AxialProfile tutorials and fixtures
    ↓
comparisons and alias evidence
    ↓
temporal examples
    ↓
gallery scenes
    ↓
diagnostic completion
    ↓
release and supervised gallery audit
```

The strongest immediate coding milestone is **Milestone 1 plus the first three examples from each family**. That proves the catalog, authoring contract, and generic example harness before dozens of files are written. Once that vertical slice passes, the remaining example inventory can be added family by family without weakening verification discipline.
