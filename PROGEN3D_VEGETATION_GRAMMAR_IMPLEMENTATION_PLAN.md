# Progen3D Vegetation Grammar Implementation Plan

**Verification authority:** `VEGETATION_GRAMMAR_V1_VERIFICATION.md` maps every
originating requirement to current source, deterministic tests, and supervisor
visual evidence. The release command composes the subsystem through
`run_vegetation_checks.sh` and `run_vegetation_visual_matrix.sh`.

## 1. Objective

Implement vegetation as a first-class Progen3D subsystem whose canonical flow is:

```text
PlantSpecies + Development State + Environment
    -> BranchGraph
    -> Organ Placements
    -> Procedural Geometry + Shared Instances
    -> Spatial Validation + Resolution Evidence
```

The subsystem must not encode a tree as unrelated cylinders and spheres. Botanical
topology, geometry, development, environmental reasoning, and rendering detail remain
separate object-model responsibilities.

## 2. Architectural boundaries

```text
vegetation/model
    BranchGraph, BranchNode, BranchSegment, PlantInternode, PlantStem
    Bud, GrowthTip, FlowerHeadSpecification, InflorescenceSpecification
    OrganAttachment, PhyllotaxisSpecification, TropismSpecification
    PlantSpeciesSpecification, PlantDevelopmentState

vegetation/service
    BranchGraphValidationService
    BranchRadiusConservationService
    RuleBranchingGenerationService
    SpaceColonizationService
    TropismDirectionService
    OrganPlacementService
    PlantTopologyDecompositionService
    FlowerHeadPlacementService
    InflorescencePlacementService
    SurfaceAttachmentService
    VegetationGeometryAssemblyService
    VegetationDeterministicHashService

geometry/model + geometry/service
    TaperedSweepShapeSpecification + generator
    BotanicalBladeShapeSpecification + generator
    BranchJunction specification + builder
```

`BranchGraph` is the authoritative botanical structure. Geometry builders consume the
graph but never mutate it. Random variation is evaluated before topology generation and
is represented by an explicit deterministic seed and hierarchical variation record.

## 3. Phase V0A - topology kernel

Implement:

1. `BranchNode` with stable identifier, position, radius, age, branch order, and state.
2. `BranchSegment` with one parent and one child node.
3. `Bud`, `GrowthTip`, and `OrganAttachment` as semantic records.
4. `BranchGraph` with one root, unique node identifiers, directed parent-child edges,
   organ attachments, buds, and growth tips.
5. `BranchGraphValidationService` enforcing finite values, positive radii, a valid root,
   one parent per non-root node, reachability, acyclicity, increasing branch order, and
   attachment references.
6. `BranchRadiusConservationService` checking
   `parentRadius^gamma ~= sum(childRadius^gamma)` with species tolerance.

### V0A gate

- Valid trunk/branch/twig graph passes.
- Duplicate identifiers, cycles, disconnected nodes, invalid radii, and invalid
  attachment references fail with stable diagnostics.
- Traversal order is deterministic.

## 4. Phase V0B - vegetation geometry kernel

### TaperedSweep

Add a procedural shape family containing a path, one radius per path sample, an up hint,
radial tessellation, cap policy, and detail level. Generate parallel-transport frames,
continuous taper, hard end caps, deterministic UVs, semantic surface tags, collision
acceleration, and a canonical cache key.

### BranchJunction

P0 constructs a deterministic local transition assembly from parent and child sweep
sections plus a junction bulge. The result is explicitly classified as compound geometry
until a watertight implicit/SDF blend is implemented.

### LeafBlade and PetalBlade

Use a shared botanical blade abstraction with concrete leaf and petal specifications.
The mesh is generated from a longitudinal centreline, width law, camber, twist, base/tip
shape, thickness, and deterministic tessellation. Thickness greater than zero must create
a closed mesh; a zero-thickness blade is an intentional surface.

### V0B gate

- Closed tapered sweeps and thick blades are watertight and non-degenerate.
- Repeated specifications produce identical keys, vertex order, triangle order, and
  topology hashes.
- Bounds, normals, UVs, volume evidence, and collision policy are valid.

## 5. Phase V0C - organ placement

Implement:

1. `PhyllotaxisSpecification` modes: Alternate, Opposite, Decussate, Whorled, Spiral,
   and Rosette.
2. `OrganPlacementService` for deterministic placement along branch paths.
3. `OrganArray` for leaves, buds, flowers, fruit, and thorns.
4. `WhorlSpecification` for radial flower-organ placement.
5. Shared mesh instancing for leaf, petal, flower, and fruit geometry.
6. Semantic petiole-to-node and organ-to-host interfaces.

### V0C gate

- Golden-angle spiral placement is deterministic.
- Opposite, decussate, and whorled arrangements produce the documented counts and
  angular phases.
- Ten thousand repeated leaves share one source mesh specification.

## 6. Phase V0D - species and generated plants

Implement `PlantSpeciesSpecification` as data containing branching, internode, taper,
crown, phyllotaxis, leaf, flower, fruit, tropism, and growth parameters. Add a controlled
rule-branching generator that creates a `BranchGraph` from species, age, and seed.

Provide acceptance species for:

- a small deciduous tree;
- a shrub;
- a flowering herb;
- a grass clump.

### V0D gate

- Species data, not C++ branching conditionals, distinguishes acceptance plants.
- Generated topology and geometry hashes are stable for a fixed seed.
- Different plant-instance seeds remain related to the same species parameter ranges.

## 7. Phase V1A - environmental direction and growth

Implement:

- `TropismSpecification` and weighted environmental vector composition;
- phototropism, gravitropism, support attraction, moisture attraction, and obstacle
  avoidance;
- `GrowthSpecification` for length, radius, and organ-scale curves over age/time;
- developmental states from Seed through Dead;
- collision-aware `Avoid`, `Seek`, and `PermitIntersection` policies.

Growth changes must produce new immutable graph/state snapshots and deterministic
resolution evidence rather than mutating the last valid plant in place.

## 8. Phase V1B - space colonization and crowns

Implement crown volumes (sphere, ellipsoid, cone, inverse cone, cylinder, dome, lobed,
and custom sampled volume) and deterministic space colonization with influence radius,
kill radius, step length, iteration ceiling, and attraction-point ceiling.

### V1B gate

- Every generated node remains inside the selected crown contract.
- Attraction-point removal and nearest-node selection are deterministic.
- Building obstacle boundaries alter growth without creating forbidden penetration.

## 9. Phase V1C - vines and surface attachment

Implement `VinePath`, `SurfaceAttachmentSpecification`, tendril paths, and modes for free,
wall, trellis, ground, hanging, twining, and tendril climbing. Reuse spatial proximity,
collision, interface, and resolution-evidence services.

### V1C gate

- An ivy acceptance fixture seeks and attaches to a wall with recorded evidence.
- A tendril seeks a trellis and transitions from free to attached state.
- Invalid target geometry or unsupported collision pairs fail closed.

## 10. Phase V1D - scatter regions

Implement deterministic surface scatter with density, minimum separation, scale range,
orientation rule, collision layer/mask, and placement ceiling. Use it for grass clumps,
flowers, shrubs, fallen leaves, and rocks.

## 11. LOD and instancing contract

```text
LOD0 Bounds              crown/root proxy
LOD1 CoarseShape         trunk and crown clusters
LOD2 Assembly            major branches and leaf clusters/cards
LOD3 Component           full branch graph and instanced leaves
LOD4 ConstructionDetail  leaf thickness, flowers, fruit
LOD5 FastenersAndSeals   veins, bark microgeometry, attachment detail
```

The selected detail level participates in mesh-cache identity. Object identity and world
transform do not. Leaf, petal, flower, and fruit source meshes are cached independently
from instance transforms.

## 12. Grammar integration

Add dedicated nested parsers and evaluated syntax nodes rather than passing vegetation
constructors through the arithmetic-expression parser.

Initial canonical forms:

```p3d
I(Plant(species(Oak) age(18) seed(42) crown(Ellipsoid(5 8 5))
        branching(SpaceColonization(influence(1.2) kill(0.35) step(0.30)))
        trunk(taper(0.45 0.04))
        leaves(Leaf(profile(Ovate) length(0.11) width(0.055))
               Phyllotaxis(alternate divergence(137.5))))
  material(barkroughwood) 1 1)
```

```p3d
I(Vine(start(-3 0 2) grow(toward(light) seek(Wall01))
       attach(Wall01 distance(0.015))
       branch(probability(0.18))
       leaves(profile(Palmate) spacing(0.09)))
  material(leafgreenmattepaint) 1 1)
```

Expressions are retained during parsing and evaluated during grammar expansion. Mesh
generation consumes evaluated values and performs no random sampling.

## 13. Editor integration

Add syntax highlighting, context-sensitive completion, tooltips, scene inspection, and
debug overlays for branch order, node identifiers, growth tips, organ indices, tropism
vectors, crown volume, surface attachment, collision evidence, and selected LOD.

## 14. Safety ceilings

Initial configurable ceilings:

```text
branch nodes per plant          100000
branch segments per plant       99999
growth iterations                4096
attraction points              250000
organs per plant               250000
radial segments per branch         64
blade longitudinal segments       256
generated vertices             2000000
generated triangles            4000000
```

Reject pathological requests before allocation. An empty or invalid plant must produce a
diagnostic, not an empty successful mesh.

## 15. Verification corpus

Create fixtures for:

- minimal trunk;
- tapered polyline branch;
- fork with radius conservation;
- invalid cycle;
- ovate leaf;
- curved petal;
- alternate/opposite/decussate/whorled/spiral placement;
- five-petal flower;
- deterministic deciduous tree;
- shrub;
- grass clump;
- space-colonized crown;
- wall-climbing vine;
- surface scatter;
- all six LOD levels;
- time samples at seed, juvenile, mature, flowering, and senescent states.

Each fixture records topology hash, bounds, counts, diagnostics, and rendered evidence.

## 16. Completion definition

The vegetation subsystem is complete only when:

- V0 topology, geometry, organ placement, species, LOD, and instancing are implemented;
- V1 growth, tropism, space colonization, collision reasoning, vines, attachment, and
  scatter are implemented;
- grammar syntax and editor support are live;
- examples render in supervisor mode;
- deterministic source, geometry, topology, export, and visual gates pass;
- completion is audited requirement-by-requirement against the originating specification.

V2 bark displacement, veins, wind, seasons, roots, fruit development, and resource
competition remain explicit post-completion realism extensions unless separately promoted.

## 17. Current implementation status — 2026-08-22

| Requirement | Current evidence | Status |
|---|---|---|
| BranchGraph, BranchNode, BranchSegment, buds, growth tips | `include/vegetation/model`, topology and branching source gates | Implemented |
| Internode and Stem semantics | `PlantInternode`, `PlantStem`, `PlantTopologyDecompositionService`, deterministic chain-decomposition assertions | Implemented as derived BranchGraph topology, not duplicate geometry |
| Radius conservation | `BranchRadiusConservationService` and fork/rule tests | Implemented |
| TaperedSweep | Canonical grammar family, mesh generator, topology tests | Implemented |
| BranchJunction | Canonical grammar family and generated fork parts | Implemented as deterministic compound P0 geometry |
| LeafBlade and PetalBlade | Shared blade model, grammar, closed/surface mesh tests | Implemented |
| Phyllotaxis and Whorl | Alternate, Opposite, Decussate, Whorled, Spiral, Rosette plus flower whorls | Implemented |
| FlowerHead | `FlowerHeadSpecification`, golden-angle `FlowerHeadPlacementService`, grammar, editor completion, LOD4 plant fixture | Implemented with deterministic `r = c*sqrt(n)` packing |
| Inflorescence | Raceme, Spike, Panicle, Umbel, Corymb, and Head placement topology through `InflorescencePlacementService` | Implemented as topology over shared flower geometry |
| Petioles and attachment interfaces | `PlantPetioleSpecification`, `petiole(...)`, shared TaperedSweep array, `node_surface` to `petiole_base` evidence | Implemented |
| Compound leaves / LeafArray | `PlantCompoundLeafSpecification`, `leafArray(...)`, dedicated placement service, shared rachis sweeps and leaflet instances | Implemented for alternate/opposite single-rachis leaves |
| Shared leaf and petal instancing | `InstanceArrayShapeSpecification` in vegetation assembly | Implemented |
| Rule branching and species data | Five catalog species with deterministic hierarchical variation | Implemented |
| Growth and developmental states | Growth channels, age/state grammar, time-sampled tests | Implemented |
| Tropism | Weighted directional influences in growth services and grammar | Implemented |
| Space colonization and crown volumes | Sphere, ellipsoid, cone, inverse cone, cylinder, dome, lobed, custom sampled crowns | Implemented |
| Vines, tendrils, attachment, Avoid/Seek/PermitIntersection | Vine grammar, surface attachment, collision behavior and fixtures | Implemented |
| ScatterRegion | Deterministic surface placement and fixture | Implemented |
| LOD0–LOD5 | Plant assembly detail ranges and six-level geometry gate | Implemented |
| Editor completion and inspection | Shape completion, catalog tooltips, core vegetation inspector fields | Implemented; detailed growth/crown overlays remain partial |
| Supervisor examples | Primitive, plant, vine, scatter, space-colonization, and L-system fixtures | Implemented; rerun after each geometry promotion |
| General OrganArray for buds, leaves, petals, flowers, fruit, and thorns | `PlantOrganArraySpecification`, repeatable `organArray(...)`, deterministic host-path placement, shared source meshes, and typed orientation | Implemented for Stem, Branch, FlowerHead, and Surface hosts |
| FruitShell and fruit instancing | `PlantFruitSpecification`, `fruit(...)`, closed Revolve-backed `FruitShellGeometryFactory`, fruit surface tags, and shared OrganArray source | Implemented |
| L-system input to BranchGraph | Symbol/rule/specification model, bounded deterministic rewriting, turtle lowering, tropism, radius reconciliation, organ attachment, grammar, editor completion, and scene fixture | Implemented as a deliberately small P0 alphabet |
| Roots, bark displacement, veins, wind, seasons, resource competition | Explicit V2 realism work | Deferred |

### Petiole promotion evidence

The petiole implementation keeps topology and geometry separate:

```text
OrganAttachment(node_surface -> petiole_base)
    + PlantPetioleSpecification
    -> one canonical TaperedSweep mesh
    -> one InstanceArray transform per leaf
    -> leaf blade translated to the petiole tip
```

Validation gates:

```text
./tests/run_vegetation_topology_checks.sh
./tests/run_vegetation_rule_branching_checks.sh
./tests/run_vegetation_geometry_assembly_checks.sh
./tests/run_vegetation_editor_checks.sh
./tests/run_vegetation_grammar_scene_checks.sh
./tests/run_curved_shape_model_checks.sh
```

These gates remain part of the regression matrix. Compound-leaf, generalized
organ-array, FruitShell, and L-system lowering have since been promoted and are
covered by the evidence sections below.

### Compound-leaf promotion evidence

The compound-leaf implementation preserves the botanical assembly hierarchy:

```text
OrganAttachment(node_surface -> petiole_base)
    -> PlantPetioleSpecification / TaperedSweep
    -> PlantCompoundLeafSpecification
        -> one instanced rachis TaperedSweep per leaf attachment
        -> LeafArray along the rachis via OrganPlacementService
        -> one shared LeafBlade source instanced as leaflets
```

Canonical grammar:

```p3d
leafArray(3 0.08 0.0018 0.0008 Opposite 180 0.12 0.75)
```

P0 accepts `Alternate` and `Opposite` leaflet patterns and rejects unsupported
patterns, fewer than two rachis nodes, invalid taper, negative orientation bias,
and non-positive leaflet scale. The placement gate proves repeatable transforms;
the geometry gate proves separate petiole, rachis, and leaflet instance arrays;
the grammar gate proves symbolic parsing, species overrides, diagnostics, and the
four-species showcase below the established mesh ceilings.

### General OrganArray promotion evidence

`PlantOrganArraySpecification` separates the requested organ from its host path and
placement policy:

```text
organ type + host + count + spacing + azimuth progression
    + scale falloff + orientation + deterministic jitter + minimum order
    -> OrganArrayPlacementService
    -> one canonical organ source mesh
    -> InstanceArray transforms
```

Canonical grammar:

```p3d
organArray(Fruit Branch 4 0.16 137.50776 0.04 0.92 Outward 0.08 1)
```

Supported organ sources are Bud, Leaf, Petal, Flower, Fruit, and Thorn. Supported
hosts are Stem, Branch, FlowerHead, and Surface; Surface requires the
`SurfaceNormal` orientation contract. Placement and assembly tests prove stable
ordering, scale falloff, bounded jitter, typed semantic roles, and shared geometry.

### Internode, Stem, FlowerHead, and Inflorescence promotion evidence

`PlantTopologyDecompositionService` derives botanical objects without changing the
authoritative `BranchGraph`:

```text
BranchSegment + endpoint BranchNodes
    -> PlantInternode
Continuation chain at one branch order
    -> PlantStem
```

The topology gate proves a two-internode primary stem plus a separate lateral stem,
stable source-segment identity, node endpoints, length, radius, and branch order.

Flower topology similarly remains separate from geometry:

```text
FlowerHeadSpecification
    -> golden-angle floret sites

InflorescenceSpecification
    -> Raceme | Spike | Panicle | Umbel | Corymb | Head sites

flower site + WhorlSpecification
    -> shared PetalBlade InstanceArray
```

Explicit flower heads and inflorescences attach to the deterministic principal active
growth point, preventing accidental multiplication across every terminal branch. The
placement gate proves all six inflorescence kinds, golden-angle radial packing, normalized
orientations, scale falloff, and byte-stable transforms. The grammar scene gate proves a
Panicle shrub and a FlowerHead herb within the configured mesh safety ceiling.

### FruitShell promotion evidence

`FruitShellGeometryFactory` lowers `PlantFruitSpecification` to one closed profile
of revolution:

```text
fruit(height radius shoulder fullness radialSegments profileSegments)
    -> deterministic axial fruit profile
    -> Revolve
    -> BotanicalFruitOuter surface tags
    -> shared OrganArray source mesh
```

The standalone gate verifies a stable shape key, vertex order, face order, topology
hash, positive signed volume, watertight two-manifold topology, no degenerate
triangles, and fail-closed invalid dimensions and tessellation.

```text
./tests/run_vegetation_fruit_shell_checks.sh
```

### L-system promotion evidence

The P0 L-system alphabet is intentionally explicit and bounded:

```text
F, Plus, Minus, Push, Pop
```

Grammar keeps symbolic sentences outside numeric expression validation while
`lSystem(...)` parameters retain checked expression evaluation. Repeatable
`lRule(...)` declarations are parsed as production rules, validated for unique
predecessors and balanced turtle state, expanded within a symbol ceiling, then
lowered through:

```text
PlantLSystemSpecification
    -> LSystemBranchGraphGenerationService
    -> BranchGraph
    -> radius conservation + tropism
    -> ordinary organ attachment
    -> ordinary vegetation geometry assembly
```

The canonical fixture uses the standard branching production
`F -> F[+F]F[-F]F`. Three iterations produce 126 nodes, 125 segments, 63 terminal
tips, a stable topology hash, and a deterministic 77,880-triangle LOD3 scene.

```text
./tests/run_vegetation_lsystem_checks.sh
./tests/run_vegetation_grammar_scene_checks.sh
./tests/run_vegetation_lsystem_visual_checks.sh
./tests/run_vegetation_environment_visual_checks.sh
```

The environment visual gate covers the V1 vine, attachment, scatter, and
space-colonization showcases with two byte-identical supervisor captures per
fixture. This closes the visual-evidence gap between grammar-scene validation
and the full promoted V1 environmental surface.

### Remaining post-completion realism work

The architectural V0/V1 subsystem is complete. Explicit V2 extensions remain:
watertight implicit branch-junction blending, bark displacement, leaf veins and
deformation, wind, seasonal transitions, root systems, fruit development, and
resource competition.
