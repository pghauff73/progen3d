# Building Vegetation Object Model: Triangle Distribution Research

Research snapshot: August 28, 2026.

This renderer-focused study is now a companion to
`docs/BUILDING_VEGETATION_OBJECT_MODEL_LITERATURE_REVIEW.md`, which defines the
broader biological, spatial, calibration, and provenance object model.

## Objective

The Building Vegetation Object Model must represent vegetation as a spatially
aware building object and as a biological structure. Triangle count is not the
primary truth. Triangle placement must preserve the visual and structural facts
that define the plant:

- root and ground contact;
- primary stem and branch hierarchy;
- branch junction continuity;
- crown silhouette;
- projected foliage area;
- crown gap fraction and visible porosity;
- flowers, fruit, and material transitions;
- motion and growth envelopes;
- support, attachment, irrigation, stormwater, and maintenance interfaces.

The implementation therefore separates biological topology, geometric
representation, triangle allocation, spatial placement, and camera-view
fidelity. A lower triangle representation is accepted only when it remains a
faithful representation of the authored object.

## Research Findings

### Biological topology precedes polygonization

Runions, Lane, and Prusinkiewicz model tree crowns through competition for
space. Attraction points influence nearby growth nodes, producing a branching
structure adapted to the available crown volume and obstacles. This supports a
Building Vegetation Object Model in which branch topology, tropism, obstacle
response, and crown volume exist before triangle generation.

Terrestrial-laser-scanning and quantitative-structure-model research provides
an external validation pattern. TreeQSM-style reconstruction measures branch
length, diameter, order, volume, and topology. ProGen3D does not need to adopt
TreeQSM as its generator, but these measurements are useful reference truths
for captured or calibrated species models.

### Foliage simplification must preserve represented area

Foliage is aggregate geometry composed of many disconnected leaves or blades.
When simplification removes complete organs, the crown becomes visibly thin.
Epic's Nanite guidance addresses this by redistributing lost area to surviving
open boundaries. Symmetric leaves are effectively enlarged; ribbon-like grass
blades become thicker.

ProGen3D records an `organLinearAreaPreservationScale` for foliage, flower, and
fruit regions. The value is a representation directive for organ-removal LODs,
not permission to enlarge trunks or structural supports.

### Triangle clusters should remain spatially and semantically local

Mesh-shader pipelines process small mesh clusters. Khronos mesh shaders expose
task and mesh workgroups that generate compact vertex and primitive outputs.
The meshoptimizer reference implementation recommends meshlets that balance
vertex reuse and culling efficiency and provides a flexible builder when
spatial locality is more important than filling every meshlet.

Vegetation should not combine distant leaves merely to fill a cluster. Meshlets
should be built inside semantic regions and crown cells so that frustum,
occlusion, wind, and LOD decisions can reject or replace local groups.

### Billboard clouds remain useful as a far representation

Billboard-cloud research approximates complex geometry using a set of textured
planes selected to represent the model from many directions. It remains useful
for distant crowns, but it is not a near-field geometry substitute. ProGen3D
uses billboard clouds only after full geometry, area-preserving organ geometry,
and clustered organ geometry exceed their projected-error limits.

### Masked foliage can trade triangles for overdraw

Reducing leaf geometry to large transparent cards may lower triangle count but
increase masked-pixel work and produce poor depth, shadow, and ray behavior.
Epic's current foliage guidance recommends geometry-based leaves for Nanite
foliage and notes that masked-card foliage can be expensive. ProGen3D therefore
tracks triangle cost and coverage/overdraw cost separately. Triangle reduction
is not considered an optimization when it produces substantially more rejected
or transparent fragment work.

## Object Model

`BuildingVegetationObjectModel` composes:

- `BuildingObjectSpatialProfile` for extents, collision, orientation,
  placement, interfaces, and graph memberships;
- `PlantArchitecture` for tree, shrub, herb, grass, or vine architecture;
- `VegetationRepresentationFidelityProfile` for independent acceptance
  thresholds;
- `VegetationTriangleDistributionPolicy` for target budget, meshlet size,
  semantic minimums, area preservation, and importance weights.

The generated 50-object suite additionally records category-specific reference
budgets, protected semantic regions, projected-error LOD contracts, and the
front/right/top fidelity contract.

## Semantic Triangle Regions

Triangles are distributed among explicit plant roles:

1. `RootFlare`
2. `PrimaryStem`
3. `BranchJunction`
4. `BranchSegment`
5. `FoliageSilhouette`
6. `FoliageInterior`
7. `FlowerOrFruit`
8. `GroundContact`
9. `SupportStructure`

Every region declares:

- source triangle count;
- non-negotiable minimum triangle count;
- represented biological or material area;
- normalized silhouette contribution;
- projected-area contribution;
- curvature contribution;
- topology contribution;
- motion contribution;
- material-boundary contribution;
- visibility frequency;
- biological-area contribution.

## Triangle Allocation

The default normalized importance score is:

```text
0.25 silhouette
+ 0.20 projected area
+ 0.12 curvature
+ 0.13 topology
+ 0.08 motion
+ 0.05 material boundary
+ 0.07 visibility
+ 0.10 biological area
```

Allocation proceeds in two stages:

1. Assign every semantic minimum.
2. Distribute the remaining budget proportionally by importance, respecting
   each region's source capacity and resolving fractional triangles through a
   deterministic largest-remainder order.

The result is independent of input region order and records a deterministic
evidence hash. Invalid budgets, duplicate region IDs, impossible semantic
minimums, non-finite values, and targets above source geometry fail closed.

## Representation Levels

Representation selection is based on measured projected error rather than a
fixed camera distance.

| Representation | Error limit | Reference budget | Purpose |
|---|---:|---:|---|
| Full semantic geometry | 0.35 px | 100% | Editing, inspection, near camera views |
| Area-preserving organ geometry | 0.85 px | 55% | Near-to-mid rendering |
| Clustered organ geometry | 1.75 px | 25% | Mid-distance rendering and dense scenes |
| Billboard cloud | 4.00 px | 8% | Distant crowns and scene overview |

These are initial policy values, not certified universal thresholds. They must
be calibrated against target display resolution, field of view, motion, species
complexity, lighting, and renderer cost.

## True-Representation Gate

`VegetationRepresentationFidelityEvaluationService` requires exactly one front,
right, and top observation. Every view independently evaluates:

- silhouette match;
- projected foliage-area match;
- crown gap-fraction match;
- branch-topology match;
- material-area match;
- motion-envelope match.

The generated object suite requires at least 95 percent for every metric in
every view and targets 97 percent. Composite averaging cannot hide a failed
metric or view.

ImageGen remains appearance evidence. The deterministic grammar and camera
render remain geometry authority. If an ImageGen result changes branch count,
silhouette, crown porosity, planter geometry, or support topology, the result is
rejected or constrained back to the camera geometry; correct grammar must not
be rewritten to match invented structure.

## Renderer Integration

The next renderer slice should:

1. Partition `VegetationAssemblyGeometry` by semantic role and crown cell.
2. Measure front/right/top projected contribution for every region.
3. Ask `VegetationTriangleDistributionService` for each representation budget.
4. Simplify structural regions with locked junction and silhouette boundaries.
5. Reduce foliage by deterministic organ selection before simplifying surviving
   blade meshes.
6. Apply area-preservation scale only to surviving disconnected foliage organs.
7. Build spatially local meshlets inside semantic regions.
8. Record triangle count, meshlet count, projected error, overdraw estimate, and
   fidelity observations in render evidence.
9. Select LOD with hysteresis to avoid visible representation oscillation.
10. Fall back to the previous validated representation if any fidelity gate
    fails.

## Primary Sources

- Runions, Lane, and Prusinkiewicz, *Modeling Trees with a Space Colonization
  Algorithm*, Eurographics Workshop on Natural Phenomena, 2007:
  https://diglib.eg.org/items/b5d756ee-0ab3-436e-a5cb-617d50df78fb
- Gonzalez de Tanago et al., *Quantifying branch architecture of tropical trees
  using terrestrial LiDAR and 3D modelling*, Trees, 2018:
  https://doi.org/10.1007/s00468-018-1704-1
- Décoret et al., *Billboard Clouds for Extreme Model Simplification*, ACM
  Transactions on Graphics, 2003:
  https://www-sop.inria.fr/reves/Basilic/2003/DDDS03/
- Epic Games, *Working with Nanite-Enabled Content*, foliage guidance:
  https://dev.epicgames.com/documentation/unreal-engine/working-with-naniteenabled-content
- Khronos Group, `VK_EXT_mesh_shader` reference:
  https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_mesh_shader.html
- meshoptimizer reference implementation and documentation:
  https://github.com/zeux/meshoptimizer
