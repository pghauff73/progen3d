# ProGen3D MCP_OMv1 Modern Car Parametric Object Model Implementation Plan

## 1. Objective

Create `MCP_OMv1`, the Modern Car Parametric Object Model v1, by converting
the research and generator logic in:

```text
examples/MVPv2_5_Red_AWD_Hatchback/Modern_Car_MCSMv1/
```

into a typed, deterministic, provenance-bearing ProGen3D object model.

`MCP_OMv1` owns the parametric family definition and generated candidate
realizations. It does not replace the evidence-constrained automotive authority
already implemented by MVPv2.5. MVP2.6 composes both models:

```text
MVP2.6
  = preserved MVPv2.5 semantic authority
  + MCP_OMv1 parametric family source
  + explicit source-to-authority fitting evidence
```

The implementation must retain all MVPv1, MVPv2, and MVPv2.5 compatibility
gates.

## 2. Current Baseline

The MCSMv1 package currently provides:

- four vehicle variants: reference, track, aero, and crossover;
- package, wheel, style, and powertrain parameters;
- a Python implicit-field and marching-cubes generator;
- generated GLB, OBJ, and watertight body STL artifacts;
- reference-variant character curves and body sections;
- orthographic and isometric previews;
- validation and checksum records;
- a proposed grammar that is explicitly not compatible with the current
  ProGen3D parser.

The existing MVPv2.5 implementation already owns:

- calibrated reference observations;
- the authoritative vehicle package and reference frame;
- semantic landmarks, wireframe, and silhouette constraints;
- Class-A surface patches and highlight-flow validation;
- panel gaps, apertures, closures, glass motion, and seals;
- BIW, suspension, wheels, tyres, rims, brakes, and wheel houses;
- aero geometry, fitting residuals, and deterministic hashes.

MCP_OMv1 must therefore be additive. Generated MCSMv1 meshes are candidate
geometry, not a replacement for the MVPv2.5 semantic graph.

## 3. Compatibility Boundary

### 3.1 Current executable grammar

The current parser can execute:

- parameterized production rules;
- `Object`, `Interface`, `Position`, and `Joint` records;
- transforms such as `T`, `S`, and `A`;
- procedural shapes including `VehicleBodyShell`, `SurfaceLoft`,
  `ShellOffset`, `SweepDisk`, `Revolve`, `Extrude`, and `CompoundShape`;
- explicit instance material, alpha, and texture-scale arguments.

The current parser cannot execute MCSMv1 terms such as
`ImplicitSectionField`, `IsoSurface`, `StationInterpolation`, or
`GenerateCandidateRealization`.

Every `.p3d` file added during MCP_OMv1 P0-P5 must therefore use the current
parser. Future semantic syntax belongs in documentation until the corresponding
model, parser, evaluator, and geometry services exist.

### 3.2 Model authority

| Semantic fact | Canonical owner |
|---|---|
| Variant package dimensions | `VehiclePackageParameters` |
| Wheel radius, width, and tracks | `VehicleWheelParameters` |
| Style factors | `VehicleStyleParameters` |
| Powertrain intent | `VehiclePowertrainIntent` |
| Station interpolation knots | `StationFunction` |
| Lower-body field | `LowerBodyFieldDefinition` |
| Greenhouse field | `GreenhouseFieldDefinition` |
| Fender blending | `FenderFieldDefinition` and `ImplicitFieldBlendDefinition` |
| Wheelhouse subtraction | `WheelhouseDifferenceDefinition` |
| Generated mesh vertices and triangles | `GeneratedVehicleRealization` |
| Extracted curves and sections | generated evidence associated with a realization |
| Source files and hashes | `ParametricModelSourceManifest` |
| Final Class-A and functional authority | `VehicleMvp25Architecture` inside MVP2.6 |

No generated mesh, cache, grammar, or adapter may become a second owner of a
package or field parameter.

## 4. Coordinate Model

MCSMv1 uses:

```text
+x = vehicle front
+y = vehicle right
+z = vehicle up
```

MVP and ProGen3D vehicle models use:

```text
+X = vehicle right
+Y = vehicle up
+Z = vehicle forward
```

The authoritative conversion is:

```text
MVP.X = MCSM.y
MVP.Y = MCSM.z
MVP.Z = MCSM.x
```

`ModernCarCoordinateFrame` owns this relationship. Geometry services consume
the conversion; they must not copy or rediscover it.

The MCP_OMv1 local origin is the ground projection of the vehicle package
centre. Axle locations are therefore:

```text
front axle = (0, wheel radius, +wheelbase / 2)
rear axle  = (0, wheel radius, -wheelbase / 2)
```

## 5. UML-Readable Object Model

### 5.1 Root aggregate

```text
ModernCarParametricObjectModel
  *-- ModernCarCoordinateFrame
  *-- ModernCarFamilyDefinition
  *-- ParametricModelSourceManifest
  *-- ParametricModelGenerationPolicy
  o-- GeneratedVehicleRealization [0..*]
  o-- ParametricVehicleValidationReport [0..*]
```

`GeneratedVehicleRealization` is derived from a variant and generation policy.
It is associated with the root aggregate but does not own the variant source.

### 5.2 Family and variant composition

```text
ModernCarFamilyDefinition
  *-- ModernCarVariantDefinition [1..*]

ModernCarVariantDefinition
  *-- VehiclePackageParameters
  *-- VehicleWheelParameters
  *-- VehicleStyleParameters
  *-- VehiclePowertrainIntent
  *-- ParametricBodyDefinition
  *-- VehicleExteriorFeatureDefinition
```

### 5.3 Parametric body composition

```text
ParametricBodyDefinition
  *-- VehicleLongitudinalDomain
  *-- LowerBodyFieldDefinition
  *-- GreenhouseFieldDefinition
  *-- FenderFieldDefinition
  *-- WheelhouseDifferenceDefinition
  *-- ImplicitFieldBlendDefinition
  *-- ParametricCharacterCurveNetwork
  *-- ParametricBodySectionNetwork
```

### 5.4 Station functions

```text
StationFunction
  *-- StationFunctionKnot [2..*]
```

Each `StationFunction` has one purpose-revealing identifier, one interpolation
kind, and an ordered knot list. Examples include:

- `LowerBodyHalfWidthByStation`;
- `LowerBodyCentreHeightByStation`;
- `GreenhouseHalfWidthByStation`;
- `GreenhouseCentreHeightByStation`;
- `RoofScaleByStation`.

Do not store equivalent sampled tables beside the knot definition. Sampling is
a derived service result with source-hash provenance.

### 5.5 Generated realization

```text
GeneratedVehicleRealization
  *-- GeneratedBodyMesh
  *-- GeneratedCharacterCurveSet
  *-- GeneratedBodySectionSet
  *-- GeneratedObservationSet
  *-- GeneratedArtifactManifest
```

The realization records:

- source variant identifier;
- source manifest hash;
- generation-policy hash;
- coordinate-frame identifier;
- resolution preset;
- deterministic geometry hash;
- generated artifact paths;
- generation diagnostics.

## 6. Proposed C++ Class Layout

```text
include/vehicle/parametric/model/
  ModernCarParametricObjectModel.h
  ModernCarCoordinateFrame.h
  ModernCarFamilyDefinition.h
  ModernCarVariantDefinition.h
  VehiclePackageParameters.h
  VehicleWheelParameters.h
  VehicleStyleParameters.h
  VehiclePowertrainIntent.h
  ParametricBodyDefinition.h
  StationFunction.h
  LowerBodyFieldDefinition.h
  GreenhouseFieldDefinition.h
  FenderFieldDefinition.h
  WheelhouseDifferenceDefinition.h
  ImplicitFieldBlendDefinition.h
  ParametricCharacterCurveNetwork.h
  ParametricBodySectionNetwork.h
  VehicleExteriorFeatureDefinition.h
  GeneratedVehicleRealization.h
  ParametricModelSourceManifest.h
  ParametricModelGenerationPolicy.h
  ParametricVehicleValidationReport.h

include/vehicle/parametric/service/
  ModernCarParametricCatalogFactory.h
  ModernCarCoordinateConversionService.h
  ModernCarFieldEvaluationService.h
  ModernCarIsoSurfaceGenerationService.h
  ModernCarCharacterCurveExtractionService.h
  ModernCarBodySectionExtractionService.h
  ModernCarObservationGenerationService.h
  ModernCarParametricValidationService.h
  ModernCarParametricDeterministicHashService.h
  ModernCarParametricGrammarLoweringService.h
  Mvp26ParametricVehicleIntegrationService.h
```

Inheritance is not required for the initial field classes. They are distinct
composed concepts with different invariants. A shared abstract field category
should only be introduced later if all concrete fields genuinely satisfy one
stable evaluation contract.

## 7. Generator and Provenance Strategy

The existing Python generator remains the first source implementation. Do not
copy its variant constants into handwritten C++.

Refactor it into importable responsibilities:

```text
McsM1VariantCatalog
McsM1FieldDefinitionFactory
McsM1VehicleGenerator
McsM1ArtifactWriter
McsM1ValidationReporter
```

Add `tools/generate_mcp_omv1_artifacts.py` as the canonical bridge. It imports
the refactored MCSMv1 module and writes:

- normalized `MCP_OMv1_FamilyDefinition.json`;
- normalized `MCP_OMv1_SourceManifest.json`;
- generated C++ catalog records used by `ModernCarParametricCatalogFactory`;
- per-variant curve, section, camera, and validation artifacts;
- current-parser `.p3d` preview lowerings;
- deterministic SHA-256 records.

The generated C++ records must embed the source JSON hash and must never be
edited manually. This avoids a runtime JSON dependency while retaining the
MCSMv1 source as the canonical owner.

Remove `__pycache__` and `.pyc` files from the canonical package and checksum
manifest. Record exact generator dependency versions in a lock file. Exclude
timestamps, temporary directories, and machine-specific paths from
deterministic hashes.

## 8. Service Pipeline

```text
MCSMv1 source catalog
  -> MCP_OMv1 normalized catalog generation
  -> ModernCarParametricCatalogFactory
  -> ModernCarParametricValidationService
  -> ModernCarFieldEvaluationService
  -> ModernCarIsoSurfaceGenerationService
  -> curve and section extraction services
  -> observation generation service
  -> GeneratedVehicleRealization
  -> grammar lowering or MVP2.6 integration
```

Every stage returns a result object containing value-or-diagnostics. No service
may silently clamp an invalid package, invent missing station knots, repair an
unknown coordinate frame, or substitute a mesh after a failed generation.

## 9. MVP2.6 Integration Model

```text
VehicleMvp26Architecture
  *-- VehicleMvp25Architecture
  *-- VehicleVariantDefinition
  *-- VehicleParametricShapePrior
  *-- VehicleParametricSourceEvidence
  *-- VehicleParametricFitReport
```

`VehicleMvp26Architecture` composes the accepted MVPv2.5 architecture. It must
not inherit from `VehicleMvp25Architecture`, because MVP2.6 is an aggregate
containing an authority model plus new source and fit evidence, not a narrower
kind of MVPv2.5 architecture.

`Mvp26ParametricVehicleIntegrationService` performs these explicit mappings:

1. MCP package parameters to `VehiclePackageEvidence` candidates;
2. MCP character curves to observed curve samples;
3. MCP sections to Class-A fitting seeds;
4. MCP generated views to `VehicleViewObservation` candidates;
5. MCP style values to soft `VehicleShapePrior` terms;
6. MCP powertrain intent to existing `VehiclePowertrainSpecification` inputs;
7. fit residuals back to `VehicleParametricFitReport`.

MVP package evidence remains hard authority unless a new variant is explicitly
selected. A variant selection creates a new package definition; it must not
silently mutate the accepted red hatchback baseline.

## 10. Implementation Work Packages

### P0 - Source normalization and hygiene

- refactor `modern_car_parametric_model.py` into importable responsibilities;
- remove generated Python bytecode from source and checksum ownership;
- pin the generator environment;
- regenerate all source artifacts twice and compare hashes;
- document the source coordinate frame and package-centre origin;
- fail if package length differs from front overhang plus wheelbase plus rear
  overhang beyond tolerance.

**Exit gate:** two clean regenerations produce identical normalized records,
curves, sections, meshes, and hashes.

### P1 - Typed MCP_OMv1 model

- implement the model classes in Section 6;
- use immutable value semantics after construction;
- separate model, service, relationship, and validation classes;
- implement explicit validation issue codes;
- implement deterministic object-model hashing;
- add a four-variant catalog factory from generated records.

**Exit gate:** the C++ model loads exactly four unique variants and reproduces
all canonical package and style values without handwritten duplicates.

### P2 - Field evaluation and candidate geometry

- port or bind the generalized superellipsoid field equations;
- implement lower-body and greenhouse field evaluation;
- implement smooth-minimum fender blending;
- implement wheelhouse difference evaluation;
- implement deterministic iso-surface generation with bounded resolution;
- record watertightness, finite geometry, bounds, and topology evidence.

**Exit gate:** each variant produces one finite candidate body whose normalized
bounds match its package and whose repeated geometry hash is stable.

### P3 - Curves, sections, and observations

- export character curves for all four variants, not only reference;
- export body sections for all four variants;
- use deterministic station identifiers and sample ordering;
- generate front, side, and top orthographic observations for every variant;
- generate rear and opposite-side views for MVP2.6 fitting;
- retain camera and projection metadata with every observation.

**Exit gate:** every variant has eight named character curves, a validated body
section network, and deterministic observation records.

### P4 - Current-parser grammar lowerings

- implement `ModernCarParametricGrammarLoweringService`;
- lower packages and coarse silhouettes to supported `VehicleBodyShell`,
  `Revolve`, `SweepDisk`, and `Extrude` shapes;
- emit one explicit material argument for every `I` or `!I` instance;
- emit `Start` and all production arrows;
- place vehicles upright with `+Y` up and `+Z` forward;
- limit preview detail so multi-variant scenes remain below renderer index and
  draw-list budgets.

**Exit gate:** family and adapter grammars parse, generate, render, export, and
produce deterministic captures without retaining an older scene snapshot.

### P5 - MVP2.6 adapter

- add `VehicleMvp26Architecture`;
- add `VehicleVariantDefinition`, `VehicleParametricShapePrior`,
  `VehicleParametricSourceEvidence`, and `VehicleParametricFitReport`;
- implement `Mvp26ParametricVehicleIntegrationService`;
- preserve the accepted MVPv2.5 architecture and hash as a composed member;
- create explicit builders for reference, track, aero, and crossover variants;
- keep `RedAwdHatchbackMvp25Builder` unchanged as the compatibility builder.

**Exit gate:** the reference variant preserves the accepted MVPv2.5 result,
while the other variants produce distinct packages and source-evidence hashes.

### P6 - Fit and silhouette validation

- compare MCP-generated observations with ProGen3D candidate views;
- use silhouette intersection-over-union as the primary coarse metric;
- require at least `0.80` IoU independently for front, side, and top views;
- retain the full five-view MVPv2.5 landmark, character-curve, Class-A,
  closure, chassis, wheel, rim, brake, and fit validations;
- classify package constraints as hard and style resemblance as soft;
- fail closed when a view, camera, source hash, or mapping is missing.

**Exit gate:** every variant passes the three-view `0.80` silhouette floor;
the reference variant also passes the complete MVP2.6 evidence-constrained
validation suite.

### P7 - Documentation and acceptance evidence

- add a UML relationship diagram;
- publish source, generated, grammar, native, and visual evidence hashes;
- add a requirement-to-test matrix;
- document known concept-geometry limits such as absent CFD, crash,
  manufacturing, suspension sweep, and homologation validation;
- rerun MVPv1, MVPv2, MVPv2.5, MCP_OMv1, and MVP2.6 compatibility gates.

**Exit gate:** every requirement has one named owner, one validation route, and
one evidence artifact.

## 11. Required Invariants

### Package

- all dimensions are finite and positive;
- `length = frontOverhang + wheelbase + rearOverhang` within tolerance;
- front and rear tracks are smaller than overall width;
- wheel radius and width are positive;
- ground clearance is nonnegative;
- axle centres agree with wheelbase and the package-centre origin.

### Station functions

- at least two knots;
- finite knot positions and values;
- strictly increasing normalized positions in `[0, 1]`;
- unique purpose identifier;
- explicit interpolation kind;
- no extrapolation unless the generation policy explicitly permits it.

### Powertrain

- AWD intent names both front and rear driven axles;
- EV variants require an energy-storage envelope;
- EV variants have zero exhaust outlets;
- combustion variants cannot claim a battery-only energy source;
- powertrain intent remains separate from generated exterior detail geometry.

### Generated realizations

- source and generation-policy hashes are present;
- all positions are converted to the declared MCP coordinate frame;
- all mesh values are finite;
- package-normalized bounds satisfy tolerance;
- body watertightness is reported explicitly;
- curves and sections reference stable semantic identifiers;
- repeated generation produces identical deterministic hashes.

## 12. Test and Evidence Matrix

| Requirement | Focused test or artifact |
|---|---|
| Four variants and unique identifiers | `tests/mcp_omv1_catalog_harness.cpp` |
| Package equations and bounds | `ModernCarParametricValidationService` tests |
| Coordinate conversion | `tests/mcp_omv1_coordinate_conversion_harness.cpp` |
| Field samples | `tests/mcp_omv1_field_evaluation_harness.cpp` |
| Deterministic generation | two-run hash comparison |
| Curves and sections for every variant | catalog artifact audit |
| Executable grammar syntax | `tests/run_mcp_omv1_grammar_checks.sh` |
| Three-view silhouette IoU | `tests/run_mcp_omv1_visual_checks.sh` |
| MVP2.6 source adapter | `tests/mvp26_parametric_integration_harness.cpp` |
| MVPv2.5 compatibility | existing MVPv2.5 native and visual scripts |
| MVPv2 and MVPv1 compatibility | existing compatibility scripts |
| Source hygiene and provenance | checksum and manifest audit |

## 13. Planned Artifacts

```text
examples/MVPv2_5_Red_AWD_Hatchback/Modern_Car_MCP_OMv1/
  README.md
  MCP_OMv1_FamilyDefinition.json
  MCP_OMv1_SourceManifest.json
  MCP_OMv1_Validation.json
  MCP_OMv1_Executable_Family_Preview.p3d
  MCP_OMv1_MVP26_Reference_Adapter.p3d
  generated/reference/
  generated/track/
  generated/aero/
  generated/crossover/
```

The two example `.p3d` files are current-parser demonstrations. They expose
package, frame, axle, and candidate-geometry concepts, but they are not the
canonical MCP_OMv1 data store.

## 14. Current-Parser Grammar Example

The executable family example follows this pattern:

```p3d
Start ->
    MCPOMv1Ground
    MCPOMv1Reference
    MCPOMv1Track
    MCPOMv1Aero
    MCPOMv1Crossover

MCPOMv1Reference ->
[
    T(-2.4 0 3.4)
    Object(
        id(MCPOMV1_REFERENCE)
        name(ResearchMedianAwdHotHatch)
        class(ModernCarParametricVariant)
        taxonomy(Vehicle ParametricCandidate Hatchback Reference)
        layer(Equipment)
        mask(Equipment Terrain Temporary)
    )
    [
        Interface(id(packageCentre) type(InspectionInterface)
                  origin(0 0 0) normal(0 1 0) tangent(0 0 1)
                  region(Point) tolerance(0.001))
        MCPOMv1VehicleCandidate(4.353 1.816 1.46812 2.6289
                               1.56825 1.56799 0.13 0.34 0.235)
    ]
]
```

Every instance includes its material inside the same `I(...)` or `!I(...)`
call. Descriptor-only constructs are kept inside `Object` and `Interface`
records rather than emitted as top-level unwrapped text.

## 15. Future Semantic Grammar Contract

The following is a design target, not current executable syntax:

```text
Start -> ModernCarParametricObjectModel(MCP_OMv1)

ModernCarParametricObjectModel(modelId) ->
    CoordinateFrame(MCPVehicleFrame)
    SourceManifest(MCSMv1SourceManifest)
    FamilyDefinition(ModernCarFamilyV1)
    GenerationPolicy(MediumDeterministicPolicy)
    GenerateFamilyRealizations(ModernCarFamilyV1)

VariantDefinition(reference) ->
    VehiclePackageParameters(...)
    VehicleWheelParameters(...)
    VehicleStyleParameters(...)
    VehiclePowertrainIntent(AWD Combustion)
    ParametricBodyDefinition(
        LowerBodyField(...)
        GreenhouseField(...)
        FenderField(...)
        WheelhouseDifference(...)
        Blend(SmoothMinimum 7.0)
    )

GenerateVariantRealization(reference) ->
    EvaluateImplicitField(reference)
    ExtractIsoSurface(0)
    ExtractCharacterCurves()
    ExtractBodySections()
    GenerateObservations(front side top rear oppositeSide)
    ValidateParametricRealization()
    AdaptToMvp26()
```

This syntax may only become a `.p3d` grammar after corresponding typed model
objects, parser productions, semantic validation, evaluation services, and
deterministic tests are implemented.

## 16. Completion Standard

MCP_OMv1 is complete only when:

- MCSMv1 is refactored and remains the generator source rather than being
  duplicated;
- four typed variants exist with one canonical owner for every parameter;
- coordinate conversion and package-centre placement are explicit;
- all variants generate deterministic body, curve, section, and view evidence;
- current-parser family and adapter grammars execute without diagnostics;
- every variant exceeds `0.80` silhouette IoU in front, side, and top views;
- the reference variant integrates into MVP2.6 without weakening any accepted
  MVPv2.5 validation;
- MVPv1, MVPv2, MVPv2.5, MCP_OMv1, and MVP2.6 gates pass;
- provenance, limitations, hashes, and evidence remain reviewable.
