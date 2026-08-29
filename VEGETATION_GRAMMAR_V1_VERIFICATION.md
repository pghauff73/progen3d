# Progen3D Vegetation Grammar V1 Verification

## 1. Authority and completion boundary

This document audits the implementation against:

```text
/home/pamela/.codex/attachments/
3a05c73f-9624-4c7a-a6d4-20c6a147d3c7/pasted-text-1.txt
```

The originating specification defines three delivery bands:

1. the V0 topology, geometry, and organ-placement kernel;
2. V1 environmental reasoning, growth, vines, attachment, collision behavior,
   and scatter;
3. LOD and instancing as required architecture.

Section 42 and recommended implementation step 16 explicitly place implicit
junction blending, bark displacement, detailed veins, deformation, wind,
seasonality, root systems, fruit development, and resource competition after
the main architecture. Those features are therefore recorded as intentional
post-V1 work rather than silently treated as implemented.

The implemented grammar is intentionally executable rather than copying the
illustrative syntax in section 39 verbatim. `Plant(...)`, `Vine(...)`, and
`ScatterRegion(...)` evaluate checked expressions into immutable specifications,
then lower through deterministic services to `BranchGraph` and procedural mesh
families.

## 2. Architectural result

```text
Plant/Vine/Scatter grammar
    -> evaluated botanical specification
    -> BranchGraph or deterministic placement topology
    -> growth and environmental resolution
    -> organ attachment and shared instance arrays
    -> TaperedSweep / BranchJunction / LeafBlade / PetalBlade /
       Revolve / ShellLoft geometry
    -> deterministic mesh, bounds, surface tags, collision policy, and evidence
```

Primary ownership is separated into UML-readable model and service layers:

- `include/vegetation/model` owns botanical identity and immutable state.
- `include/vegetation/service` defines topology, growth, placement, and
  environmental operations.
- `src/vegetation/service` implements those operations.
- `include/geometry/model` and `src/geometry/service` own reusable mesh families.
- grammar parsing/evaluation remains in `src/grammar/service`.
- editor discovery remains in the procedural shape catalog and structured
  completion services.

## 3. Requirement-by-requirement audit

| Spec section | Requirement | Authoritative implementation evidence | Verification | Status |
|---:|---|---|---|---|
| 1 | Separate topology, geometry, and development primitives | `include/vegetation/model`, `include/vegetation/service`, `include/geometry/model` | Full source organization and grammar-scene gate | Implemented |
| 2 | `BranchGraph` with root, nodes, edges, buds, organ attachments, and growth points | `BranchGraph`, `BranchNode`, `BranchSegment`, `VegetationBud`, `OrganAttachment`, `VegetationGrowthTip` | `run_vegetation_topology_checks.sh` | Implemented |
| 3 | Continuous-taper branch geometry | `TaperedSweepShapeSpecification`, validator, parallel-transport mesh generator | `run_vegetation_tapered_sweep_checks.sh` | Implemented |
| 4 | Parent/child branch-radius conservation | `BranchRadiusConservationService` and species gamma/tolerance data | Topology and rule-branching gates | Implemented |
| 5 | Branch-junction transition geometry | `BranchJunctionShapeSpecification`, `BranchJunctionMeshGenerator` | `run_vegetation_branch_junction_checks.sh` and LOD geometry gate | Implemented as compound P0 junction; implicit watertight blend deferred |
| 6 | Branch angle, azimuth, roll/internode-style spacing, order, and count controls | `PlantBranchingSpecification`, rule branching, deterministic variation, turtle angle rules | `run_vegetation_rule_branching_checks.sh`, `run_vegetation_lsystem_checks.sh` | Implemented |
| 7 | Explicit Internode and Stem semantics | `PlantInternode`, `PlantStem`, `PlantTopologyDecomposition`, `PlantTopologyDecompositionService` | Topology gate proves two primary internodes plus a lateral stem | Implemented as derived topology |
| 8 | Bud lifecycle and meaningful node attachment | `VegetationBud`, bud states, `PlantOrganAttachmentService`, node-referenced attachments | Topology and growth gates | Implemented |
| 9 | Parametric `LeafBlade` | `BotanicalBladeShapeSpecification`, `LeafBladeShapeSpecification`, blade generator | `run_vegetation_botanical_blade_checks.sh` | Implemented |
| 10 | Canonical leaf profile variants | `BotanicalBladeProfile`: Linear, Lanceolate, Elliptic, Ovate, Obovate, Cordate, Palmate, Needle | Blade, grammar-scene, and catalog completion gates | Implemented as parameterizations |
| 11 | Compound leaf as rachis plus leaflet array | `PlantCompoundLeafSpecification`, `CompoundLeafPlacementService`, instanced rachis and leaflet parts | Placement, geometry assembly, grammar-scene gates | Implemented for alternate/opposite P0 patterns |
| 12 | Semantically distinct Petiole using tapered sweep | `PlantPetioleSpecification`, `petiole(...)`, `node_surface -> petiole_base` semantic roles | Geometry assembly and grammar-scene gates | Implemented |
| 13 | Alternate, Opposite, Decussate, Whorled, Spiral, and Rosette phyllotaxis | `PhyllotaxisMode`, `PhyllotaxisSpecification`, `OrganPlacementService` | Placement gate | Implemented |
| 14 | General `OrganArray` with count, spacing, azimuth, falloff, orientation, and variation | `PlantOrganArraySpecification`, `OrganArrayPlacementService` for Bud, Leaf, Petal, Flower, Fruit, Thorn and Stem, Branch, FlowerHead, Surface hosts | Placement and geometry assembly gates | Implemented |
| 15 | Parametric `PetalBlade` | Shared botanical blade model with petal specialization | Botanical blade and primitive visual gates | Implemented |
| 16 | Whorl placement | `WhorlSpecification`, deterministic radial transforms | Placement and geometry assembly gates | Implemented |
| 17 | Golden-angle flower-head packing | `FlowerHeadSpecification`, `FlowerHeadPlacementService` using radial square-root packing | Placement, grammar-scene, plant visual gates | Implemented |
| 18 | Raceme, Spike, Panicle, Umbel, Corymb, and Head inflorescences | `InflorescenceKind`, specification, placement service | Placement gate exercises all six kinds; shrub grammar uses Panicle | Implemented |
| 19 | Weighted tropism vectors | `TropismInfluence`, `TropismSpecification`, `TropismDirectionService` | Placement, growth, rule branching, space colonization, and vine gates | Implemented |
| 20 | Space-colonization tree generation | crown sampling, seed graph, validation, resolution evidence, `SpaceColonizationService` | `run_vegetation_space_colonization_checks.sh`, grammar-scene and environment visual gates | Implemented |
| 21 | Crown volumes including sphere, ellipsoid, cone, inverse cone, cylinder, dome, lobed, and custom | `CrownVolumeKind`, `CrownVolumeSpecification`, containment and sampling services | Space-colonization gate and showcase | Implemented |
| 22 | `VinePath` as topology over a guided path | `VineGrowthSpecification`, `VineGrowthService`, `VinePathState` | Vine-growth, grammar-scene, environment visual gates | Implemented |
| 23 | Surface attachment with offset and projection semantics | `SurfaceAttachmentSpecification`, `SurfaceAttachmentService`, target boundaries | Vine-growth and environment visual gates | Implemented |
| 24 | Free, wall-climbing, hanging, twining, and surface-creeping growth | `VineGrowthMode` and showcase records | Vine-growth and grammar-scene gates | Implemented |
| 25 | Tendril geometry and twining | tendril mode/path construction lowered through TaperedSweep | Vine-growth gate exercises tendril twining | Implemented |
| 26 | Avoid, Seek, and PermitIntersection environmental collision behavior | `VegetationCollisionBehavior`, obstacle boundaries and deterministic evidence | Vine-growth, scatter, and grammar-scene gates | Implemented |
| 27 | Time/age-driven Growth | `PlantGrowthSpecification`, channels, curves, requests, service, evidence | `run_vegetation_growth_checks.sh` | Implemented |
| 28 | Seed, Bud, Shoot, Juvenile, Mature, Flowering, Fruiting, Senescent, Dormant, Dead states | `PlantDevelopmentState`, growth filtering and organ visibility | Growth and grammar-scene gates | Implemented |
| 29 | Bounded L-system rewriting lowered to BranchGraph | symbol/rule/specification models, validation, turtle lowering, topology evidence | `run_vegetation_lsystem_checks.sh`, grammar-scene and L-system visual gates | Implemented with explicit P0 alphabet |
| 30 | Multiple generation algorithms | `PlantTopologyGenerationMethod`: RuleBranching, SpaceColonization, LSystem; guided vine growth remains separate | Rule branching, space colonization, L-system, and vine gates | Implemented |
| 31 | Root systems using shared BranchGraph/TaperedSweep/Tropism | The specification says roots should “eventually” exist and lists root systems in V2 | Completion boundary | Deferred by specification |
| 32 | Bark displacement at close LOD | Listed in section 42 V2 realism and step 16 | Completion boundary | Deferred by specification |
| 33 | Midrib/vein detail | Listed in section 42 V2 realism and step 16 | Completion boundary | Deferred by specification |
| 34 | Generative flower hierarchy | PetalBlade, Whorl, OrganArray, FlowerHead, Inflorescence, TaperedSweep, and Revolve provide the proposed compact kernel | Placement, fruit shell, geometry assembly, plant visual gates | Core hierarchy implemented; detailed reproductive organs and flower development remain V2 |
| 35 | Shrubs and grasses without separate kernels | `Shrub` and `GrassClump` species use BranchGraph/LeafBlade/OrganArray | Plant grammar-scene and visual gates | Implemented |
| 36 | Groundcover and vegetation regions | `ScatterRegionSpecification`, service, mesh generator, collision masks | Scatter-region, grammar-scene, environment visual gates | Implemented |
| 37 | Deterministic variation | hierarchical `DeterministicPlantVariationService`, explicit seeds, stable graph and mesh hashes | Rule branching, growth, scatter, grammar-scene repetition | Implemented |
| 38 | Species definition as data | `PlantSpeciesSpecification`, five-entry `PlantSpeciesCatalog`, grammar overrides | Rule branching, grammar-scene and plant visual gates | Implemented |
| 39 | User-facing vegetation grammar | `Plant(...)`, `Vine(...)`, `ScatterRegion(...)`, repeated options, checked expressions, catalog completion | Editor and grammar-scene gates | Implemented with production syntax; pasted syntax remains illustrative |
| 40 | V0 geometry: TaperedSweep, LeafBlade, PetalBlade, Revolve, ShellLoft, BranchJunction | botanical geometry plus existing procedural Revolve/ShellLoft kernel | Vegetation geometry gates, FruitShell gate, `run_profile_extrusion_checks.sh` | Implemented/reused |
| 40 | V0 grammar: BranchGraph, OrganAttach, OrganArray, Phyllotaxis, Whorl, Tropism | BranchGraph is the internal authority reached by Plant/LSystem/SpaceColonization; public options expose placement and tropism | Topology, placement, grammar-scene gates | Implemented |
| 41 | V1 environmental reasoning | Growth, SpaceColonization, SurfaceAttachment, collision behavior, ScatterRegion | Full V1 source and environment visual matrix | Implemented |
| 42 | V2 realism | Explicitly later work | Completion boundary | Deferred as specified |
| 43 | LOD0 through LOD5 | `GeometryDetailLevel`, detail ranges, progressively richer vegetation parts | Geometry assembly gate proves triangle/part progression across six levels | Implemented |
| 44 | Shared instancing for leaves, petals, flowers, and fruit | `InstanceArrayShapeSpecification`, shared organ geometry sources | Geometry assembly proves typed arrays and 10,000 leaf transforms sharing one `LeafBlade` source | Implemented |
| 45 | Spatially aware plant collision positioning | Avoid/Seek/PermitIntersection, target layers/masks, attachment resolution and evidence | Vine-growth, scatter, environment visual gates | Implemented for promoted V1 query/growth scope |

## 4. Recommended implementation-order audit

| Step | Deliverable | Evidence | Result |
|---:|---|---|---|
| 1 | BranchGraph | Topology model/service/gate | Complete |
| 2 | TaperedSweep | Shape family/generator/gate | Complete |
| 3 | BranchJunction | Shape family/generator/gate | Complete at P0 compound level |
| 4 | LeafBlade | Shared blade kernel/gate | Complete |
| 5 | OrganAttach | Attachment model/service/gates | Complete |
| 6 | OrganArray | General organ/host placement/gates | Complete |
| 7 | Phyllotaxis | Six modes/placement gate | Complete |
| 8 | PetalBlade + Whorl | Shared blade and whorl placement/gates | Complete |
| 9 | Tropism | Weighted resolution service/gates | Complete |
| 10 | Growth(age,t) | Growth channels/states/evidence | Complete |
| 11 | SpaceColonization | Crown sampling and topology service | Complete |
| 12 | SurfaceAttachment | Projection/offset service | Complete |
| 13 | VinePath | Guided vine and tendril topology | Complete |
| 14 | ScatterRegion | Deterministic regional placement | Complete |
| 15 | LOD + instancing | Six LOD levels and shared arrays | Complete |
| 16 | Bark, veins, wind, seasons, roots | Explicit post-V1 realism work | Deferred by source specification |

## 5. Grammar and editor surface

The promoted grammar families are:

```p3d
Plant(
    species(...)
    age(...)
    state(...)
    seed(...)
    branching(...)
    leaf(...)
    petiole(...)
    leafArray(...)
    organArray(...)
    flower(...)
    whorl(...)
    flowerHead(...)
    inflorescence(...)
    fruit(...)
    tropism(...)
    crown(...)
    spaceColonization(...)
    lAxiom(...)
    lRule(...)
    lSystem(...)
    detail(LOD0|LOD1|LOD2|LOD3|LOD4|LOD5)
)

Vine(
    species(...)
    start(...)
    direction(...)
    mode(...)
    collision(Avoid|Seek|PermitIntersection)
    attachment(...)
    target(...)
    detail(...)
)

ScatterRegion(
    region(...)
    species(...)
    seed(...)
    surface(...)
    density(...)
    separation(...)
    scale(...)
    orientation(...)
    layer(...)
    mask(...)
    detail(...)
)
```

`run_vegetation_editor_checks.sh` proves that these options are exposed through
the structured shape catalog and completion system rather than being parser-only
features.

## 6. Deterministic nonvisual evidence

All commands below passed on Saturday, August 22, 2026:

The repository release gate invokes the complete matrix through
`tests/run_vegetation_checks.sh`.

| Command | Principal evidence |
|---|---|
| `run_vegetation_topology_checks.sh` | 4 nodes, 3 segments, 2 stems, 3 internodes |
| `run_vegetation_tapered_sweep_checks.sh` | 216 vertices, 72 triangles, stable topology hash |
| `run_vegetation_branch_junction_checks.sh` | 870 vertices, 896 triangles, stable topology hash |
| `run_vegetation_botanical_blade_checks.sh` | deterministic leaf and petal hashes |
| `run_vegetation_growth_checks.sh` | growth channels and deterministic evidence hash |
| `run_vegetation_placement_checks.sh` | all placement modes, 8-floret head, six inflorescence kinds |
| `run_vegetation_rule_branching_checks.sh` | 140 nodes, 139 segments, 202 leaves |
| `run_vegetation_fruit_shell_checks.sh` | closed 528-triangle Revolve shell with positive volume |
| `run_vegetation_space_colonization_checks.sh` | 216 nodes, 20 iterations, obstacle rejection evidence |
| `run_vegetation_vine_growth_checks.sh` | attachment, seeking, twining, avoidance, deterministic evidence |
| `run_vegetation_scatter_region_checks.sh` | deterministic density, spacing, masks, scale and orientation |
| `run_vegetation_geometry_assembly_checks.sh` | LOD triangle progression plus 10,000 shared leaf instances |
| `run_vegetation_editor_checks.sh` | structured completion and editor integration |
| `run_vegetation_lsystem_checks.sh` | 126 nodes, 125 segments, 63 tips, bounded expansion |
| `run_vegetation_grammar_scene_checks.sh` | deterministic primitive, plant, vine, scatter, space-colonization, and L-system scenes |
| `run_profile_extrusion_checks.sh` | shared Revolve and ShellLoft geometry kernel |

The geometry-assembly and profile-extrusion scripts were corrected to link all
current source dependencies. The shared SMB-OMv2 gate was likewise corrected to
link the vegetation source manifest after `ShapeSpecificationEvaluator` gained
vegetation families.

## 7. Supervisor visual evidence

Every visual fixture is rendered twice. A gate passes only when captures are
byte-identical, all GUI smoke/render/FOV/capture markers are present, and the
expected silhouette or color regions are non-collapsed.

The repository release gate invokes all four visual suites through
`tests/run_vegetation_visual_matrix.sh`.

| Fixture | Evidence file | Capture SHA-256 |
|---|---|---|
| TaperedSweep, LeafBlade, PetalBlade primitives | `tests/evidence/vegetation_visual_2026-08-22.json` | `6e7dfe5817d2ff7ef5fddfe96047d1a81a03c5614280d61a4108ec9206d6ce0a` |
| DeciduousTree, Shrub, FloweringHerb, GrassClump | `tests/evidence/vegetation_plant_visual_2026-08-22.json` | `cbb32f220af78407d20f516182a2730ec066ec991f4d08a0d6f0598c68865d17` |
| L-system tree | `tests/evidence/vegetation_lsystem_visual_2026-08-22.json` | `c41539d635d8962abe57be53d8db9c0d7f8f99b7dc8b74521e063b958d088057` |
| Vine environment | `tests/evidence/vegetation_environment_visual_2026-08-22.json` | `ff0d61d374604a9ca4d1f100b29a5184610fc22c84352754c942b81f6b6368df` |
| ScatterRegion environment | `tests/evidence/vegetation_environment_visual_2026-08-22.json` | `5df19d71940fd57c4c79808de7c728dae5eb5abdb570b7934f1c3a22289152b7` |
| SpaceColonization environment | `tests/evidence/vegetation_environment_visual_2026-08-22.json` | `2a71b2991bd60d4ffa9dd909881f44897fccc8def7dc41b2dbf044e4bb80e7a3` |

The environment captures were also inspected directly. They show attached/climbing
vine paths, separated regional vegetation instances, and a non-collapsed irregular
space-colonized crown.

## 8. Safety and determinism

`VegetationComplexityLimits` bounds nodes, segments, organs, growth iterations,
attraction points, and generated topology. Validators reject non-finite values,
invalid references, cycles, unsupported states, invalid tessellation, excessive
arrays, and pathological geometry before allocation or mesh generation.

Determinism is proven through repeated:

- traversal and topology hashes;
- expression-evaluated shape keys;
- vertex and triangle ordering;
- growth/scatter/colonization resolution evidence;
- grammar-to-scene topology and bounds;
- byte-identical visual captures.

Geometry generators never draw random values internally. Randomness is reduced to
explicit deterministic seeds and stable hierarchical variation before topology or
mesh construction.

## 9. Known warnings and explicit limitations

The focused C++ builds report pre-existing warnings in `include/grammar.h`,
`include/Solution.h`, and `src/Mesh.cpp` concerning signed-size comparisons and
unused declarations. They do not fail the vegetation gates and were not changed as
part of this subsystem.

The following are not claimed as V1 features:

- implicit/SDF watertight branch-junction blending;
- bark displacement and peeling geometry;
- detailed midrib and vein geometry;
- wind and leaf deformation;
- flower and fruit developmental morphing;
- seasonal state transitions beyond explicit development-state selection;
- root-system generation;
- resource competition.

These match the originating specification's V2/step-16 boundary.

## 10. Completion decision

Every non-deferred V0, V1, LOD, instancing, grammar, editor, deterministic,
collision/environmental, and supervisor-visual requirement has direct current-state
evidence. The implementation plan and this verification record preserve V2 work as
explicit future scope rather than treating it as silently complete.

The Progen3D vegetation grammar V1 promotion is therefore complete.
