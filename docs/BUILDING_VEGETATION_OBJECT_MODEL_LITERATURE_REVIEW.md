# Building Vegetation Object Model Literature Review

Research snapshot: August 28, 2026.

## Review Question

What object-model concepts are required to represent trees, shrubs, grasses,
groundcovers, vines, container plants, and ecological plantings as both:

1. spatially aware building objects; and
2. biologically interpretable plant structures that can support calibrated
   growth, light, root, wind, maintenance, and renderer decisions?

The review focuses on primary peer-reviewed research and original technical
papers that define plant structure, reconstruct measurable architecture,
simulate root and shoot function, place vegetation in built environments, or
validate plant models. General landscape-design advice was excluded because it
does not provide a reproducible computational object contract.

## Main Finding

A vegetation object cannot be represented faithfully by a mesh, bounding box,
species label, or procedural seed alone. The minimum defensible model is a
composition of distinct authorities:

- identity and taxonomic resolution;
- multiscale architectural topology;
- geometry and authored extents;
- phenological and developmental state;
- root architecture and below-ground occupancy;
- canopy optical structure;
- biomechanical material behavior;
- versioned semantic anatomy and development annotations;
- functional traits with observation scope;
- hydraulic transport and drought-vulnerability traits;
- taxon- and site-specific size allometry;
- rootable substrate volume and physical soil requirements;
- environmental response and competition;
- building connection and maintenance interfaces;
- measurement, literature, generation, and validation provenance.

These concerns must remain separable. A visually convincing architectural
archetype is not automatically a calibrated species model, and a taxonomic name
does not supply missing root, optical, phenological, biomechanical, trait,
hydraulic, allometric, semantic, or substrate values.

## Literature Synthesis

### 1. Multiscale topology is the stable plant identity

Godin and Caraglio formalized plant architecture as a multiscale graph rather
than a flat list of geometry. Their model distinguishes decomposition between
scales from succession and branching within a scale. This is important because
the same plant fact may be observed as a whole plant, an axis, a growth unit, a
metamer, or an organ without creating competing topology authorities.

Functional-structural plant modelling extends that principle by coupling the
three-dimensional structure to processes such as development, resource capture,
transport, and allocation. Vos and colleagues emphasize that structure and
function influence each other over time. A static mesh therefore remains a
representation of one state, not the plant model itself.

**Object-model consequence:** `PlantArchitecturalTopologyProfile` uses the
ordered scales `WholePlant`, `Axis`, `GrowthUnit`, `Metamer`, `Organ`,
`GeometryRegion`, and `RepresentationCluster`. It records `Decomposition`,
`Succession`, `Branching`, and `Attachment` as distinct relationship meanings.
The renderer's region and meshlet levels are derived representation scales and
do not replace botanical topology.

### 2. Procedural growth needs environmental constraints

The space-colonization algorithm demonstrates that plausible branching emerges
from competition for space within an attraction volume. It is valuable for
building vegetation because crown envelopes, walls, roofs, screens, and nearby
objects can constrain growth. It does not, by itself, identify a botanical
species or validate physiological behavior.

Procedural Urban Forestry moves environmental placement into the urban context.
It models plausible vegetation locations from structural and functional zones
in city layouts and evaluates the results perceptually and through expert design
sessions. This supports explicit associations between a vegetation object and
its courtyard, planter, facade, pergola, circulation clearance, drainage zone,
or maintenance zone.

**Object-model consequence:** environment-responsive generation is represented
by `environmental_response_graph` and `vegetation_competition_graph`, while
building support and service facts remain in the existing support, attachment,
irrigation, stormwater, containment, and maintenance graphs. Procedural placement
is evidence for plausibility, not a substitute for collision or service rules.

### 3. Measured woody architecture requires quantitative structure evidence

TreeQSM reconstructs woody tree structure from terrestrial laser scanning as
connected geometric primitives. The method provides measurable branch topology,
diameter, length, surface area, and volume. Gonzalez de Tanago and colleagues
validated QSM-derived architecture against destructively measured tropical
trees, showing that branching order and large-branch geometry can be tested
against physical observations rather than judged only by appearance.

QSM evidence has limits. Occlusion, small branches, foliage, roots, and scanner
configuration affect what can be recovered. A QSM-calibrated trunk and branch
graph must not silently certify leaves, roots, phenology, or wind material
properties that were not measured.

**Object-model consequence:** branch order authority is either an explicit
generated growth graph or a measured QSM. Measurement evidence identifiers,
method, uncertainty, and specimen identity are mandatory before the object may
claim measured-specimen calibration.

### 4. Native exchange formats are semantic contracts

The Root System Markup Language defines an XML exchange contract with metadata,
scene, plant, nested root, mandatory polyline geometry, optional functions, and
annotations. Lobet and colleagues designed RSML to preserve root topology and
associated measurements across phenotyping and modelling tools. The official v1
schema makes x and y mandatory, z optional, and requires at least two points per
polyline. Its metadata defines geometry in image coordinates and uses
`resolution` to convert pixels to a declared unit.

The official TreeQSM `save_model_text` 1.1.0 exporter is also a versioned
contract, not a generic delimited table. Its cylinder rows contain radius,
length, three start coordinates, three axis values, parent, extension, branch,
branch order, position, fit statistics, added-cylinder state, and unmodified
radius. Its thirteen header labels do not directly correspond to the seventeen
numeric values, and MATLAB format reuse can place physical line breaks inside a
logical record. Decoding therefore treats the post-header values as one ordered
token stream. Not every field has a canonical biological owner yet, so decoding
must preserve the raw payload and report deferred fields.

**Object-model consequence:** `VegetationMeasuredSourceSchemaRegistry` resolves
one exact media-type and schema-version pair to one decoder. Decoders produce a
new canonical artifact while retaining the raw payload SHA-256 in provenance.
Derived endpoints, averaged point diameters, inferred child attachment, unknown
functions, and unsupported geometry are explicit observations. Unknown versions
fail closed.

### 5. Root architecture is a separate spatial system

OpenSimRoot and CRootBox represent root systems as developing architectures
coupled to soil resources and plant function. Their work shows that root depth,
radial spread, branching order, segment diameter, insertion angle, growth rate,
tropism, and local soil conditions are not derivable from the visible crown.

For buildings, below-ground uncertainty affects footings, slabs, waterproofing,
drains, pipes, retaining walls, planters, root barriers, and neighboring plants.
A trunk contact point is therefore insufficient as the full root contract.

**Object-model consequence:** `PlantRootArchitectureProfile` distinguishes an
authored safety envelope from an evidence-backed explicit root graph. Until
calibrated depth, spread, order, diameter, branching angle, and occupancy density
are available, collision and clearance use the conservative safety envelope and
root simulation remains disabled.

### 6. Canopy optics needs area and porosity, not triangle count

Canopy appearance and light interception depend on leaf area, spatial leaf-area
density, leaf-angle distribution, projected foliage area, woody occlusion, and
gaps. Zhu and colleagues frame crown gap fraction as a volumetric porosity
problem and explicitly distinguish leaf and woody components in reconstructed
crowns.

This reinforces the existing renderer requirement that foliage simplification
preserve represented area and crown porosity. Two meshes with the same triangle
count may have very different projected area, gap fraction, shadow, and visual
density.

**Object-model consequence:** `PlantCanopyOpticalProfile` owns leaf area index,
leaf-area density distribution, leaf-angle distribution, crown gap fraction,
and projected foliage area by view. Missing values remain null. The existing
front/right/top fidelity gate is visual evidence; it becomes optical calibration
only when the observations come from measured or otherwise traceable data.

### 7. Wind motion needs material evidence

Wang, Zhao, and Barbic derive botanical material parameters from plant
biomechanics and use mass density, stiffness, and damping across tree components
to produce more plausible motion. The research demonstrates that branch and leaf
motion cannot be certified by a generic sway amplitude alone.

Wind behavior also depends on drag area, topology, taper, mass distribution,
attachment, and boundary conditions. Vines and grasses require different
articulation assumptions from woody trees, while facade and pergola vegetation
must transmit load through explicit host connections.

**Object-model consequence:** `PlantBiomechanicalProfile` records mass density,
elastic modulus, damping ratio, drag coefficient, and axis/organ articulation.
Physical wind simulation fails closed until those values have evidence. Authored
motion envelopes remain available for visual animation and collision clearance
but are labelled as uncalibrated.

### 8. Phenology and development are stateful

Functional-structural models represent development over time, while plant
phenotyping research provides observations that can calibrate organ appearance,
growth, and transitions. The rendered `MatureLeafOnDesignIntent` state is only
one reference state. Juvenile, flowering, fruiting, senescent, dormant, damaged,
pruned, and dead states may change geometry, material area, collision envelopes,
maintenance requirements, and optical metrics.

**Object-model consequence:** `PlantPhenologyProfile` separates the reference
render state from the set of supported states and from a calibrated seasonal
schedule. A category name cannot supply dates or transition thresholds. Those
remain unresolved until species, climate, and specimen evidence are attached.

### 9. Semantic exchange needs controlled plant terms

The Plant Ontology separates plant anatomical entities from plant-structure
development stages and assigns stable identifiers within explicit releases.
This is a stronger interoperability contract than free-text labels such as
"leaf", "shoot", or "mature", whose meaning can drift between grammars,
measurements, databases, and simulation services.

An ontology term is an annotation, not geometry ownership. A `plant organ`
identifier can classify a leaf mesh region or a root observation without
replacing the multiscale topology graph, authored object identifier, or
phenology schedule.

**Object-model consequence:** `PlantSemanticAnnotationProfile` owns the
ontology identifier, release identifier, anatomical-entity identifiers,
development-stage identifiers, and evidence identifiers. Partial crosswalks
fail closed because unversioned or mixed-vocabulary terms are not stable
calibration evidence.

### 10. Functional traits require observation scope

The TRY data paper demonstrates both the value and the incompleteness of plant
trait databases. Growth form has broad coverage, but continuous traits relevant
to vegetation modelling exhibit intraspecific variation and trait-environment
relationships. A species-level mean is therefore not equivalent to a measured
plant in a particular climate, substrate, or management regime.

**Object-model consequence:** `PlantFunctionalTraitProfile` records specific
leaf area, leaf dry-matter content, leaf nitrogen content, maximum mature
height, and traceable evidence. Values remain null for architectural archetypes.
Calibration evidence bundles preserve taxon or specimen scope, location,
measurement protocol, units, uncertainty, source references, and exact payload
hash rather than importing a database mean as universal truth.

### 11. Hydraulics is a separate functional authority

Christoffersen and colleagues coupled measurable hydraulic traits to a
size-structured vegetation model. Their parameterization distinguishes maximum
conductance, capacitance, xylem vulnerability, stomatal vulnerability, and the
leaf-to-sapwood area ratio. These traits govern water transport and drought
response; they cannot be reconstructed from crown appearance or an irrigation
connection point.

**Object-model consequence:** `PlantHydraulicProfile` owns leaf-specific
conductance, hydraulic capacitance, xylem and stomatal water potentials at
fifty-percent loss, leaf-to-sapwood area ratio, and evidence. Irrigation and
stormwater graphs describe building services, while hydraulic state and
transport remain plant-function concerns.

### 12. Mature size needs calibrated allometry

Pretzsch and colleagues measured open-grown trees and derived species-specific
crown-radius to stem-diameter relationships plus crown extension types. The
study shows that urban growing form differs from a simple scaled forest tree
and that height, crown radius, projection area, and crown volume depend on
species, dimension, competition, and site context.

**Object-model consequence:** `PlantSizeAllometryProfile` owns reference height,
horizontal radius, supporting-axis diameter, a size-relationship model
identifier, and evidence. The authored visual or growth extent remains the
current spatial boundary; allometry may propose future extents only after its
own calibration and collision review.

### 13. Rootable substrate is a building constraint

Kopinga's street-tree study links restricted rootable soil volume to moisture
and nutrient deficits and shows that planting-hole dimensions must be reasoned
about through water and nutrient supply rather than a generic planter label.
The reported values are context-dependent rules and experiments, not universal
species constants.

**Object-model consequence:** `VegetationSubstrateRequirementProfile` owns
minimum rootable volume and depth, maximum bulk density, minimum air-filled
porosity, minimum available-water capacity, and evidence. It is evaluated
against a planter, tree pit, green roof, or landscape substrate object through
a suitability service; it does not duplicate the root graph or building
containment graph.

### 14. Validation must use multiple independent patterns

Pattern-oriented validation tests several observable patterns rather than
accepting one aggregate score. This aligns with the existing ProGen3D rule that
silhouette, projected foliage area, gap fraction, topology, material area, and
motion envelope must each pass in each required camera view.

The literature also supports a stronger distinction between kinds of validity:

- geometric validity: extents, silhouette, topology, branch dimensions;
- optical validity: leaf area, angle distribution, porosity, projected area;
- developmental validity: organ emergence and state transitions;
- functional validity: resource capture, transport, or allocation;
- mechanical validity: stiffness, damping, drag, and attachment response;
- semantic validity: versioned ontology terms and traceable crosswalks;
- trait validity: observation scope, units, provenance, and variability;
- hydraulic validity: conductance, capacitance, vulnerability, and water state;
- allometric validity: size relationship, taxon, site, and reference dimension;
- substrate validity: rootable volume, depth, density, porosity, and water supply;
- placement validity: building-zone, support, service, and clearance rules.

No one test certifies every validity domain.

## BVO-OMv2.1 Object Relationships

The enriched `BuildingVegetationObjectModel` composes:

- `PlantIdentityProfile`;
- `PlantArchitecturalTopologyProfile`;
- `PlantPhenologyProfile`;
- `PlantRootArchitectureProfile`;
- `PlantCanopyOpticalProfile`;
- `PlantBiomechanicalProfile`;
- `PlantEnvironmentalResponseProfile`;
- `PlantSemanticAnnotationProfile`;
- `PlantFunctionalTraitProfile`;
- `PlantHydraulicProfile`;
- `PlantSizeAllometryProfile`;
- `VegetationSubstrateRequirementProfile`;
- `VegetationEvidenceProfile`;
- `BuildingObjectSpatialProfile`;
- `VegetationRepresentationFidelityProfile`;
- `VegetationTriangleDistributionPolicy`.

In UML terms, `BuildingVegetationObjectModel` owns one spatial profile, one
biological profile, one visual-fidelity profile, and one triangle-distribution
policy. `VegetationBiologicalProfile` owns the twelve biological and
building-ecology concerns plus their evidence profile. The growth, root,
phenology, environment, competition, service, and evidence graphs are
associations between objects; they are not hidden inside the mesh.

## Calibration States

### Implemented readiness authorities

`VegetationBiologicalProfileValidationService` now validates identity,
architecture compatibility, multiscale topology, phenology, root measurements,
canopy optics, biomechanics, semantic annotations, traits, hydraulics,
allometry, substrate requirements, environmental drivers, and provenance as
one structural contract. Contradictory or partially calibrated records fail
closed.

`VegetationCalibrationReadinessEvaluationService` evaluates fourteen independent
domains: taxonomic identity, shoot topology, root architecture, canopy optics,
phenology, biomechanics, environmental response, semantic annotation,
functional traits, hydraulics, size allometry, substrate suitability, building
placement, and representation fidelity. The current 50 architectural archetypes
are ready for authored building placement only. Other domains remain not ready
until their measurements and evidence identifiers are attached.

### Implemented calibration evidence boundary

`VegetationCalibrationEvidenceBundle` and
`VegetationCalibrationMeasurement` provide the immutable exchange model between
external observations and the canonical biological profile. The bundle owns its
schema, taxon or specimen scope, target architecture, coordinate and unit
systems, acquisition and uncertainty statements, source references, typed
measurements, and exact canonical payload hash.

Parsing, canonical serialization, SHA-256 calculation, structural validation,
profile binding, and readiness evaluation are separate services. Binding is
domain-atomic: a structurally valid but incomplete domain is deferred, while a
complete domain can specialize a newly constructed profile. The architectural
archetype remains unchanged. This preserves the literature distinction between
procedural appearance, taxon-level evidence, and a measured specimen.

The implemented measured-source adapter layer accepts five hashed canonical JSON
schemas for QSM cylinders, root architecture graphs, canopy optical
observations, phenology series, and biomechanical tests. Purpose-specific
adapters validate source structure, normalize supported local coordinate and
unit conventions, preserve uncertainty and provenance, and emit the immutable
calibration evidence bundle.

The native decoder layer now implements the exact TreeQSM `save_model_text`
1.1.0 cylinder table, metric three-dimensional RSML v1, and three declared
ProGen3D measurement tables. A schema registry prevents format guessing.
Georeferenced records require a validated affine transform with source and
target frames, coordinate units, uncertainty, evidence identity, and evidence
SHA-256. Canonical output embeds the unchanged raw source hash and all transform
evidence before entering the adapter boundary.

This does not make heterogeneous vendor files interchangeable. Binary TreeQSM,
arbitrary CSV files, pixel-only RSML, and reconstruction quality assessment
remain separate future capabilities.

### Point-cloud intake and reconstruction admission

The August 28, 2026 research snapshot adds a separate point-cloud intake layer.
This separation is necessary because an interchange decoder establishes what
the source bytes mean, while a reconstruction algorithm estimates plant organs
and topology from incomplete samples.

PLY 1.0 is property-oriented and extensible. A vertex element can carry project
attributes beyond position, so the object model records property names and
defers unknown scalar values instead of pretending the core schema defines their
semantics. D3A established bounded ASCII scalar vertex records and metadata-only
binary headers. D3B1 now applies the same semantic mapping to exact little- and
big-endian signed and unsigned integers and IEEE-754 floating-point scalars.
List-valued vertex properties remain unsupported because their variable record
shape requires a separate bounded collection model.

LAS is a versioned binary exchange contract. Public-header point counts, point
record format and length, coordinate scales and offsets, and declared bounds are
authoritative. D3A supports uncompressed LAS 1.0 through 1.4 only, validates the
version-specific layouts, and maps integer coordinates through those exact
scales and offsets before applying the proven source-to-plant transform. ASPRS
marks LAS 1.5 Revision 00 as the latest release at this research snapshot. It
therefore receives an explicit unsupported capability rather than being misread
as LAS 1.4.

LAZ compression and ASTM E57's XML/binary packet structure require specialized
implementations. No audited LASzip, PDAL, or libE57Format dependency is present
in this repository, so these formats fail closed with the missing dependency
named. This is safer than writing partial compressed-format readers inside the
plant object model.

TreeQSM and SimpleTree demonstrate that quantitative woody architecture can be
estimated from terrestrial laser scanning, but their outputs depend on
segmentation, cover sets, cylinder fitting, occlusion, sampling density, and
algorithm parameters. Leaf/wood classification work likewise shows that organ
separation is an independent inference problem. ASPRS vegetation classes 3, 4,
and 5 are height categories and therefore cannot substitute for woody/foliage
labels.

The resulting object model has three explicit levels:

1. `VegetationPointCloudSourceMetadata` preserves source format, version,
   encoding, count, record layout, scales, offsets, bounds, coordinate frame,
   unit, and exact source hash.
2. `VegetationPointCloudDataset` owns canonical metric points and explicit
   attributes without asserting reconstructed organs.
3. `VegetationPointCloudReconstructionAdmissionReport` evaluates point count,
   spatial extent, duplicates, provenance, and target-specific organ evidence
   before `VegetationPointCloudReconstructionJob` can be created.

An admitted job is deterministic intent, not calibration evidence. A later
reconstruction slice must add independent quality reports for topology,
cylinder residuals, taper, coverage, leaf/wood classification, leaf area,
projected area, gap fraction, and withheld observations before derived QSM or
canopy artifacts can enter the existing measured-source adapters.

### Canopy occupancy reconstruction and quality

Voxel-based terrestrial laser-scanning methods provide a practical structural
representation of crown occupancy. Hosoi and Omasa use voxelized laser returns
as part of leaf-area-density estimation, while the VoxLAD work separates beam
transmission, attenuation, and non-random foliage assumptions from the voxel
grid itself. A separate sensitivity study shows that voxel size and scan setup
change canopy gap estimates.

These results argue against converting point count directly into leaf area or
optical gap fraction. D3B2A therefore reconstructs only foliage-supported
occupancy cells. Each cell retains its metric index and centre plus foliage,
woody, and unknown source-point counts. The source dataset, SHA-256, job,
algorithm, seed, voxel scale, and bounds remain explicit.

Quality evaluation is a separate service. A deterministic split withholds a
declared subset of foliage points and tests whether those points fall within a
bounded voxel neighborhood of training occupancy. The resulting holdout recall
is a project reconstruction-consistency metric, not a biological measurement.
Projected occupancy areas and projected gap proxies are likewise geometry
diagnostics. They cannot populate `leaf_area_index`, `crown_gap_fraction`, or
`mean_leaf_inclination_degrees` in the canopy observation adapter.

This creates a useful renderer and grammar artifact without crossing the
calibration boundary. Accepted occupancy can guide crown-cell mesh generation,
semantic meshlets, culling, and foliage LOD allocation. Optical calibration
still requires an independently validated method that accounts for sampling,
occlusion, beam geometry, attenuation, and foliage distribution.

### Primary woody-axis reconstruction and the QSM boundary

TreeQSM defines a quantitative structure model as a hierarchical collection of
cylinders and exposes point-to-model distance, surface coverage, taper
correction, minimum radius, and parent-radius correction as reconstruction or
quality concerns. SimpleTree likewise reconstructs topologically ordered
cylinders and reports that radius correction and parameter optimization matter
when the model is compared with destructively measured trees.

These methods do not justify treating one global cylinder fit as a complete
tree. A robust-cylinder study demonstrates that ordinary PCA can produce severe
radius bias for partial cylindrical surfaces with outliers. QSM ground-truth
validation further shows that occlusion and unresolved small branches affect
whole-tree volume. Therefore branch topology, local surface support, and volume
validation remain separate authorities.

D3B2B implements a deliberately narrower artifact. Woody-labelled points are
split deterministically into training and holdout evidence. Eigen3 covariance
analysis estimates the dominant axis. An initial radial-distance distribution
then supports a bounded median-absolute-deviation trim before a final axis fit.
This trim is an explicit contamination boundary, not a claim of robust PCA.

The final inliers form ordered `VegetationWoodyAxisRadiusStation` records at a
declared axial spacing. Consecutive stations form
`VegetationWoodyAxisCylinder` geometry. Radius varies between stations, so the
quality service can measure training and holdout surface RMSE against a tapered
profile instead of forcing one global radius.

The quality report also records woody-label fraction, excluded-outlier
fraction, principal variance fraction, holdout axial coverage, and taper
violations. These are reconstruction diagnostics. They are not field-measured
diameters, whole-tree woody volume, biomass, branch order, or branch topology.
The reconstruction is explicitly `PrimaryAxisOnly` and cannot enter the
quantitative-structure-model adapter.

### Explicit segmentation, branch graphs, and sensitivity admission

Raumonen and colleagues reconstruct tree structure through connected surface
patches, segment sets, branches, and fitted cylinders. These stages are related
but not interchangeable: organ classification says whether evidence is woody,
segmentation assigns woody evidence to axes, graph construction defines
parent-child topology, and cylinder fitting estimates local geometry.

Wilkes and colleagues compared terrestrial-laser-scanning reconstructions with
harvested branches. That component-level ground truth supports evaluating branch
architecture explicitly instead of relying on crown silhouette or aggregate
volume alone. It also reinforces the need to preserve branch-level geometry and
topology so errors remain attributable to a particular axis, cylinder, or
attachment.

Zhang and colleagues varied TreeQSM patch-size and relative-cylinder-length
parameters for apple trees and found material differences in reconstructed
branch attributes. This is direct evidence that one parameter setting is not a
general certificate. Parameter choice, point density, scanner configuration,
occlusion, and branch scale belong in the reconstruction evidence envelope.

**Object-model consequence:** D3B2C1 separates
`VegetationWoodyPointSegmentation` from
`VegetationWoodyBranchGraphReconstruction`. The segmentation owns exact source
identity, root and parent axes, branch order, point-to-axis assignments,
confidence, and completeness for observed woody points. A dedicated validation
service rejects identity drift, cycles, invalid order transitions, duplicate or
missing assignments, non-woody assignments, and low confidence.

The graph then owns `VegetationWoodyBranchAxis`,
`VegetationWoodyBranchCylinder`, and `VegetationWoodyBranchConnection`
collections. Independent quality evaluation retains assignment coverage,
principal variance, training and holdout surface residuals, holdout coverage,
attachment surface gap, taper, branch order, and bounded component counts. A
quality-admitted graph may enter the canonical QSM adapter, but its scope is
`CompleteForObservedWoodyEvidence`; it explicitly does not claim that occluded
or unobserved biological branches are complete.

The official TreeQSM implementation reinforces this staged design. Its
`cover_sets` stage partitions points into surface patches and neighbour
relations, `segments` classifies connected study-region components into
continuations and branches, `cylinders` preserves parent, extension, branch,
branch-order, point-distance, and surface-coverage evidence, and
`select_optimum` compares repeated models through point-model distances and
variation in volume, area, length, branch count, branch-order distributions,
and cylinder distributions.

**D3B2C2 consequence:** `VegetationWoodyCoverSetSegmentationService` now infers
candidate branch identity without reading PLY or LAS classification values.
Canonical woody points are partitioned into deterministic cover cells, joined
through a bounded neighbour graph and rooted minimum spanning tree, and divided
into axes through connectivity and direction-continuity changes. Provisional
assignments are refined through per-axis covariance and parent-child line
attachment before final geometric confidence is recorded.

Every successful candidate enters the unchanged D3B2C1 graph authority.
`VegetationWoodySegmentationSensitivityEvaluationService` then compares
accepted-candidate fraction, selected assignment agreement, axis-count and
branch-order consensus, cylinder-count variation, total-axis-length variation,
and derived observed-woody-volume variation. A selected graph therefore has
evidence of deterministic parameter stability as well as point-to-model
quality.

The deterministic fixture still contains project-specific PLY classification
values for the earlier explicit-segmentation test, but source gates prove the
automated service never reads them. Sensitivity agreement does not replace
harvested-branch or equivalent component-level ground truth. D3B2C3 must compare
branch detection, attachment, diameter, length, taper, and volume before
observed woody volume can become calibrated evidence.

### Architectural archetype

The current 50-object suite is at this level. It has deterministic grammar,
authored extents, spatial interfaces, growth methods, visual requirements, and
camera evidence. Botanical taxon and quantitative biological measurements are
unknown.

### Taxon calibrated

Requires a scientific taxon or cultivar and traceable source evidence. It may
define plausible parameter ranges but does not establish a measured specimen.

### Specimen calibrated

Requires specimen identity and acquisition evidence such as TLS, image-based
reconstruction, manual measurement, root observation, phenotyping, or material
testing. Uncertainty and missing domains remain explicit.

### Simulation ready

Simulation readiness is domain-specific. A specimen can be ready for geometric
LOD validation but not for root growth, light interception, phenology, or wind.
Each domain has its own gate and evidence dependencies.

## Required Evidence by Domain

| Domain | Minimum evidence before calibrated use |
|---|---|
| Identity | Scientific name or cultivar, resolution level, source identifier |
| Shoot topology | Rooted graph, parent relation, branch order, lengths, diameters, uncertainty |
| Root architecture | Depth, spread, root order, diameters, angles, occupancy density |
| Canopy optics | Leaf area, density distribution, angle distribution, gap fraction, projected area |
| Phenology | State definitions, transition observations, climate/location context |
| Biomechanics | Density, stiffness, damping, drag, articulation and boundary conditions |
| Semantic annotation | Ontology and release identifiers, anatomy and development terms, crosswalk evidence |
| Functional traits | Trait values, units, taxon/specimen scope, protocol, location, uncertainty |
| Hydraulics | Conductance, capacitance, vulnerability thresholds, area ratio, water-state evidence |
| Size allometry | Reference dimensions, relationship model, taxon, site, management context |
| Substrate suitability | Rootable volume and depth, bulk density, porosity, water capacity, substrate evidence |
| Environmental response | Driver-response observations, climate and site context, calibrated curves |
| Building placement | Host zone, support, collision, clearance, service and maintenance constraints |
| Representation fidelity | Independent view metrics, source images/data, camera contract, evidence hash |

## Renderer and Grammar Implications

1. Grammar remains the deterministic geometry authority for the current
   architectural-archetype suite.
2. Biological profile fields constrain generation only after their calibration
   gate passes.
3. Root safety envelopes participate in collision even when roots are not
   rendered.
4. LOD reduction preserves botanical topology, projected foliage area, and gap
   fraction rather than optimizing triangle count alone.
5. Meshlets remain local to botanical regions and crown cells so that wind,
   culling, and LOD decisions preserve semantic ownership.
6. Camera comparison remains flattened and object-local, but visual matching
   cannot certify roots, phenology, traits, hydraulics, substrate, physiology,
   or mechanics.
7. ImageGen remains appearance intent and may not invent taxonomic or measured
   biological facts.

## Research Limitations

- The reviewed methods are not a universal parameter catalogue for all species.
- Trees are better covered than grasses, groundcovers, vines, and mixed
  architectural plantings.
- Above-ground scanning does not normally resolve full root architecture.
- QSM accuracy varies with occlusion, point density, reconstruction settings,
  and branch size.
- Explicit point-to-axis segmentation can validate graph construction without
  validating the segmentation algorithm that produced it.
- A graph complete for observed woody points is not necessarily a complete
  biological tree; occluded and unresolved branches remain outside its scope.
- One parameter setting cannot establish QSM stability. D3B2C2 now evaluates a
  bounded deterministic parameter ensemble, but component-level biological
  comparison remains a separate authority.
- The evidence bundle boundary does not itself assess whether a native QSM,
  point cloud, root graph, or laboratory file was reconstructed correctly;
  source-specific adapters and their validation fixtures remain required.
- Native measured-source decoding is intentionally limited to one pinned
  TreeQSM text exporter, metric 3D RSML v1, and three declared ProGen3D tables.
  Point-cloud intake separately supports PLY 1.0 ASCII, little-endian binary,
  and big-endian binary scalar vertices plus uncompressed LAS 1.0-1.4. Binary
  QSM, PLY vertex lists, LAS 1.5, LAZ, E57, arbitrary vendor tables, pixel-only
  RSML, leaf-surface reconstruction, component-level branch ground-truth
  admission, complete biological-tree reconstruction, calibrated observed
  woody volume, biomass, and optical canopy reconstruction remain unsupported.
  Geometry-only foliage occupancy, primary woody-axis geometry, an explicit
  quality-admitted branch graph, and classification-independent automated
  segmentation with cross-parameter sensitivity admission are supported after
  independent deterministic holdout quality evaluation.
- RSML point-domain diameters require adjacent averaging for segment radii, and
  nested-root attachment requires nearest-parent-segment derivation. Both are
  reported as non-measured conversion observations.
- Functional-structural models can be highly parameterized and require domain
  calibration before prediction.
- Procedural plausibility and image similarity are weaker claims than measured
  biological equivalence.
- The current model does not yet execute coupled water, carbon, temperature,
  or root-soil simulation; it provides explicit ownership and readiness gates
  for future services.
- TRY coverage does not justify silently filling missing continuous traits;
  intraspecific variation and site context remain part of the evidence scope.
- Plant Ontology identifiers require a recorded release and an explicit
  project crosswalk; names alone are not stable enough for exchange.

## Primary Sources

1. Godin, C., and Caraglio, Y. (1998). *A multiscale model of plant topological
   structures*. Journal of Theoretical Biology. https://doi.org/10.1006/jtbi.1997.0561
2. Vos, J., Evers, J. B., Buck-Sorlin, G. H., Andrieu, B., Chelle, M., and de
   Visser, P. H. B. (2010). *Functional-structural plant modelling: a new
   versatile tool in crop science*. Journal of Experimental Botany.
   https://doi.org/10.1093/jxb/erp345
3. Runions, A., Lane, B., and Prusinkiewicz, P. (2007). *Modeling Trees with a
   Space Colonization Algorithm*. Eurographics Workshop on Natural Phenomena.
   https://doi.org/10.2312/NPH/NPH07/063-070
4. Raumonen, P., Kaasalainen, M., Akerblom, M., Kaasalainen, S., Kaartinen, H.,
   Vastaranta, M., Holopainen, M., Disney, M., and Lewis, P. (2013). *Fast
   Automatic Precision Tree Models from Terrestrial Laser Scanner Data*.
   Remote Sensing. https://doi.org/10.3390/rs5020491
5. Gonzalez de Tanago, J. et al. (2018). *Quantifying branch architecture of
   tropical trees using terrestrial LiDAR and 3D modelling*. Trees.
   https://doi.org/10.1007/s00468-018-1704-1
6. Postma, J. A. et al. (2017). *OpenSimRoot: widening the scope and application
   of root architectural models*. New Phytologist.
   https://doi.org/10.1111/nph.14641
7. Schnepf, A. et al. (2018). *CRootBox: a structural-functional modelling
   framework for root systems*. Annals of Botany.
   https://doi.org/10.1093/aob/mcx221
8. Niese, T., Pirk, S., Albrecht, M., Benes, B., and Deussen, O. (2022).
   *Procedural Urban Forestry*. ACM Transactions on Graphics.
   https://doi.org/10.1145/3502220
9. Zhu, Y. et al. (2023). *A reinterpretation of the gap fraction of tree crowns
   from the perspectives of computer graphics and porous media theory*.
   Frontiers in Plant Science. https://doi.org/10.3389/fpls.2023.1109443
10. Wang, B., Zhao, Y., and Barbic, J. (2017). *Botanical materials based on
    biomechanics*. ACM Transactions on Graphics.
    https://doi.org/10.1145/3072959.3073655
11. Louarn, G., and Song, Y. (2020). *Two decades of functional-structural plant
    modelling: now addressing fundamental questions in systems biology and
    predictive ecology*. Annals of Botany. https://doi.org/10.1093/aob/mcaa143
12. Wang, M. et al. (2018). *Pattern-oriented modelling as a novel way to verify
    and validate functional-structural plant models: a demonstration with the
    annual growth module of avocado*. Annals of Botany.
    https://doi.org/10.1093/aob/mcx187
13. Cooper, L. et al. (2013). *The Plant Ontology as a Tool for Comparative
    Plant Anatomy and Genomic Analyses*. Plant and Cell Physiology.
    https://doi.org/10.1093/pcp/pcs163
14. Kattge, J. et al. (2020). *TRY plant trait database - enhanced coverage and
    open access*. Global Change Biology. https://doi.org/10.1111/gcb.14904
15. Christoffersen, B. O. et al. (2016). *Linking hydraulic traits to tropical
    forest function in a size-structured and trait-driven model (TFS v.1-Hydro)*.
    Geoscientific Model Development.
    https://doi.org/10.5194/gmd-9-4227-2016
16. Pretzsch, H. et al. (2015). *Crown size and growing space requirement of
    common tree species in urban centres, parks, and forests*. Urban Forestry &
    Urban Greening. https://doi.org/10.1016/j.ufug.2015.04.006
17. Kopinga, J. (1991). *The Effects of Restricted Volumes of Soil on the
    Growth and Development of Street Trees*. Arboriculture & Urban Forestry.
    https://doi.org/10.48044/jauf.1991.016
18. Lobet, G. et al. (2015). *Root System Markup Language: Toward a Unified Root
    Architecture Description Language*. Plant Physiology.
    https://doi.org/10.1104/pp.114.253625
19. RootSystemML. *RSML v1 XML schema*, `rsml.xsd`, commit
    `0b47b2af172805619309c19650ed92fe3b89512c`.
    https://raw.githubusercontent.com/RootSystemML/RSMLValidator/0b47b2af172805619309c19650ed92fe3b89512c/rsml.xsd
20. Raumonen, P. (2020). *TreeQSM `save_model_text` 1.1.0 exporter*, commit
    `a65ec03ec2646fcc14981e95e632b0c48e579817`.
    https://raw.githubusercontent.com/InverseTampere/TreeQSM/a65ec03ec2646fcc14981e95e632b0c48e579817/src/tools/save_model_text.m
21. Stanford 3D Scanning Repository. *The PLY Polygon File Format
    specification*. https://graphics.stanford.edu/data/3Dscanrep/ply.html
22. American Society for Photogrammetry and Remote Sensing. (2024). *LAS
    Specification 1.4 Revision 16*.
    https://github.com/ASPRSorg/LAS/releases/tag/1.4.R16
23. American Society for Photogrammetry and Remote Sensing. *LAS
    Specification 1.5 Revision 00*.
    https://github.com/ASPRSorg/LAS/releases/tag/1.5.R00
24. Maloney, A., and libE57Format contributors. *libE57Format: C++ read and
    write support for the ASTM E57 3D imaging data format*.
    https://github.com/asmaloney/libE57Format
25. Hackenberg, J., Spiecker, H., Calders, K., Disney, M., and Raumonen, P.
    (2015). *SimpleTree - An Efficient Open Source Tool to Build Tree Models
    from TLS Clouds*. Forests. https://doi.org/10.3390/f6114245
26. Vicari, M. B., Disney, M., Wilkes, P., Burt, A., Calders, K., and Woodgate,
    W. (2019). *Leaf and wood classification framework for terrestrial LiDAR
    point clouds*. Methods in Ecology and Evolution.
    https://doi.org/10.1111/2041-210X.13144
27. Hosoi, F., and Omasa, K. (2006). *Voxel-Based 3-D Modeling of Individual
    Trees for Estimating Leaf Area Density Using High-Resolution Portable
    Scanning Lidar*. IEEE Transactions on Geoscience and Remote Sensing.
    https://doi.org/10.1109/TGRS.2006.881743
28. Béland, M., Widlowski, J.-L., Fournier, R. A., Côté, J.-F., and Verstraete,
    M. M. (2014). *A model for deriving voxel-level tree leaf area density
    estimates from ground-based LiDAR*. Environmental Modelling & Software.
    https://doi.org/10.1016/j.envsoft.2014.09.034
29. Fernández-Sarría, A. et al. (2014). *Effects of voxel size and sampling
    setup on the estimation of forest canopy gap fraction from terrestrial
    laser scanning data*. Agricultural and Forest Meteorology.
    https://doi.org/10.1016/j.agrformet.2014.05.013
30. Nurunnabi, A., Sadahiro, Y., and Lindenbergh, R. (2017). *Robust Cylinder
    Fitting in Three-Dimensional Point Cloud Data*. The International Archives
    of the Photogrammetry, Remote Sensing and Spatial Information Sciences.
    https://doi.org/10.5194/isprs-archives-XLII-1-W1-63-2017
31. Demol, M. et al. (2021). *Forest above-ground volume assessments with
    terrestrial laser scanning: a ground-truth validation experiment in
    temperate, managed forests*. Annals of Botany.
    https://doi.org/10.1093/aob/mcab110
32. Wilkes, P., Shenkin, A., Disney, M., Malhi, Y., Bentley, L. P., Vicari,
    M. B., et al. (2021). *Terrestrial laser scanning to reconstruct branch
    architecture from harvested branches*. Methods in Ecology and Evolution.
    https://doi.org/10.1111/2041-210X.13709
33. Zhang, C., Yang, G., Jiang, Y., Xu, B., Li, X., Zhu, Y., et al. (2020).
    *Apple Tree Branch Information Extraction from Terrestrial Laser Scanning
    and Backpack-LiDAR*. Remote Sensing.
    https://doi.org/10.3390/rs12213592
34. TreeQSM contributors. *TreeQSM cover-set, branch-segmentation,
    cylinder-fitting, and optimum-selection implementation*, commit
    `6630bbf516f8b53adb7d60a2cccbd21e6fe51226`.
    https://github.com/InverseTampere/TreeQSM/tree/6630bbf516f8b53adb7d60a2cccbd21e6fe51226/src
