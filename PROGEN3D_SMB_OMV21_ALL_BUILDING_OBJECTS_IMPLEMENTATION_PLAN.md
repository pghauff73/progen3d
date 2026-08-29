# ProGen3D SMB-OMv2.1 All Building Objects Implementation Plan

## Document Control

- **Date:** Monday, August 24, 2026
- **Status:** Implementation plan only. No new SMB runtime claim is promoted by this document.
- **Objective:** Extend the complete 124-object Small Modern Building with first-class classification, role, function, service-flow, requirement, state, scenario, and evidence models while preserving the accepted SMB-OMv1 and SMB-OMv2 behavior.
- **Object coverage authority:** `examples/SMB_OMv2_Instance_SMB_001/SMB_OMv2_Spatial_Object_Model.json`.
- **Generator authority:** `examples/SMB_OMv2_Instance_SMB_001/generate_smb_omv2.py` remains authoritative for the accepted SMB-OMv2 spatial fixture. SMB-OMv2.1 must enrich its output rather than duplicate or reinterpret the 124-object hierarchy.
- **Compatibility authority:** `examples/SMB_OMv1_Instance_SMB_001_FiveDeep_Positioned.p3d`, the accepted SMB-OMv2 grammar, existing primitive transforms, object identities, containment, geometry bindings, interfaces, connections, constraints, and evidence hashes remain unchanged until all SMB-OMv2.1 gates pass.
- **Release authority:** Deterministic generation, exact all-object coverage, native model tests, invalid-fixture tests, requirement-to-evidence audit, editor inspection, visual evidence, compatibility gates, and a collected supervisor-mode matrix.

## 1. Executive Decision

SMB-OMv2.1 should make every building object explainable in terms of what it is, what roles it performs, what functions it contributes to, what services it exchanges, what requirements apply to it, what state it occupies, and what evidence supports those claims.

It should be an additive knowledge model around the accepted spatial model:

```text
SmallModernBuildingModel
├── SpatialBuildingModel
├── BuildingClassificationModel
├── BuildingFunctionModel
├── BuildingServiceModel
├── BuildingRequirementModel
├── BuildingScenarioModel
└── BuildingEvidenceLedger
```

The existing `SpatialBuildingModel` remains the canonical owner of:

```text
stable object identity
containment
authored and resolved frames
geometry bindings
spatial interfaces
spatial connections
placement constraints
collision policy
resolution evidence
```

SMB-OMv2.1 adds canonical owners for:

```text
validated concept definitions
contextual object roles
building functions
functional allocations and dependencies
directed service ports and flows
requirements and applicability
requirement evaluations
orthogonal operational and condition states
scenarios and state snapshots
cross-model evidence and deterministic hashes
```

The core completion rule is:

> Every one of the 124 accepted SMB objects must have one deterministic semantic profile. Every profile must explicitly state which model dimensions are applicable, not applicable, or pending evidence. No missing field may be interpreted as an inferred fact.

## 2. Version and Compatibility Boundary

### 2.1 SMB-OMv1 Remains Frozen

SMB-OMv1 remains the authoritative compatibility fixture for:

```text
124 supplied object names
container decomposition
48 reusable L1-L5 template chains
physical leaf geometry
zero geometry for relationship and semantic leaves
fixed and collision-active primitive populations
configuration guards
legacy source transforms
```

SMB-OMv2.1 must not modify the SMB-OMv1 source grammar.

### 2.2 SMB-OMv2 Remains the Spatial Authority

SMB-OMv2 remains authoritative for:

```text
124 stable object IDs
123 containment relationships
143 primitive bindings
178 typed spatial interfaces
28 accepted connections
3 executable P0 placement constraints
3 immutable resolution records
CubeY placement semantics
authored and resolved transforms
spatial editor overlays
```

SMB-OMv2.1 must not silently change any of these counts or meanings.

### 2.3 SMB-OMv2.1 Is Additive

The initial SMB-OMv2.1 release should use a deterministic sidecar model rather than requiring new grammar syntax.

Canonical generated artifact:

```text
examples/SMB_OMv2_Instance_SMB_001/
    SMB_OMv21_Building_Knowledge_Model.json
```

Coverage audit artifact:

```text
examples/SMB_OMv2_Instance_SMB_001/
    SMB_OMv21_Object_Coverage.csv
```

Acceptance evidence:

```text
tests/evidence/smb_omv21_all_objects_acceptance_YYYY-MM-DD.json
tests/evidence/smb_omv21_all_objects_visual_YYYY-MM-DD.json
tests/evidence/smb_omv21_supervisor_matrix_YYYY-MM-DD.json
```

## 3. Current All-Object Baseline

The accepted spatial manifest contains exactly 124 objects:

| Root branch | Object count |
|---|---:|
| Building root | 1 |
| Site | 10 |
| Structure | 11 |
| Ground floor | 32 |
| Upper floor | 21 |
| Envelope | 7 |
| Roof zone | 5 |
| Services | 18 |
| External works | 5 |
| Relationships and semantic evidence | 14 |
| **Total** | **124** |

The implementation must preserve this inventory exactly. A newly inferred object, implicit subsystem, virtual connection, or aggregate requirement must receive its own model identity only if a later reviewed version explicitly adds it. SMB-OMv2.1 may add functions, roles, ports, flows, requirements, scenarios, and evidence without adding building object IDs.

## 4. Theory of the Complete Building Model

The complete model is:

```text
SMB = (O, H, G, I, C, K, F, R, T, E)
```

where:

```text
O = stable objects and classifications
H = containment hierarchy
G = geometry realizations and boundaries
I = spatial and service interfaces
C = physical and functional connections
K = placement and compatibility constraints
F = functions, allocations, dependencies, and service flows
R = requirements, applicability, evaluation, and compliance
T = operational state, condition, scenarios, and lifecycle snapshots
E = provenance, uncertainty, diagnostics, evidence, and hashes
```

### 4.1 Identity Is Not Classification

`SMB_001_Ground_Kitchen_Sink` is an object identity. `Sink` is a concept assignment. The identity must remain stable if classification terminology is refined.

### 4.2 Classification Is Not Role

A `Window` may perform several roles:

```text
WeatherBoundaryClosure
DaylightOpening
ViewOpening
VentilationOpening
FacadeAssemblyMember
```

Roles are contextual assignments, not subclasses.

### 4.3 Role Is Not Function

`PotableWaterConsumer` is a role. `ProvideWashingFacility` is a building function. The sink may perform both without merging the concepts.

### 4.4 Connection Is Not Flow

A physical or semantic connection identifies related interfaces. A service flow is directed and carries a declared medium from a source port to a target port.

### 4.5 Requirement Is Not State

`SinkMustReachWasteStack` is a requirement. `Passed` is an evaluation status. `Operational` is an object state. These must have separate canonical owners.

### 4.6 Authored Is Not Derived

Authored taxonomy, placement, and declared connections remain distinct from derived function coverage, service continuity, measured clearance, and scenario state.

### 4.7 Unknown Is Not False

The model must use explicit statuses:

```text
Applicable
NotApplicable
PendingEvidence
```

and:

```text
Passed
Failed
Unknown
NotEvaluated
NotApplicable
```

Missing data must never be promoted as a successful fact.

## 5. Completion Contract for Every Object

Every object in Appendix A must have one `BuildingObjectSemanticProfile` containing:

```text
object ID
concept ID
taxonomy path reference
role assignment IDs
function allocation IDs
service port IDs
requirement IDs
scenario applicability IDs
state facet applicability
source provenance
profile revision
deterministic profile hash
```

Every profile must provide an explicit applicability record for:

| Model dimension | Allowed applicability |
|---|---|
| Geometry | `Applicable`, `NotApplicable` |
| Spatial boundary | `Applicable`, `NotApplicable`, `PendingEvidence` |
| Spatial interfaces | `Applicable`, `NotApplicable`, `PendingEvidence` |
| Functional roles | `Applicable` |
| Building functions | `Applicable`, `NotApplicable` |
| Service ports | `Applicable`, `NotApplicable`, `PendingEvidence` |
| Requirements | `Applicable` |
| Operational state | `Applicable`, `NotApplicable` |
| Condition state | `Applicable`, `NotApplicable` |
| Scenario participation | `Applicable`, `NotApplicable`, `PendingEvidence` |

No profile may omit a model dimension.

### 5.1 Container Objects

Container objects must:

- retain zero geometry unless the accepted SMB-OMv2 object already owns geometry;
- declare aggregate roles and functions;
- own containment completeness requirements;
- aggregate child requirement results without replacing child evidence;
- expose child coverage and unresolved-child counts;
- never receive placeholder geometry merely to make them selectable.

### 5.2 Physical Leaf Objects

Physical leaves must:

- retain their existing geometry bindings;
- declare physical and functional roles;
- declare boundary and interface applicability;
- receive support, clearance, containment, service, or collision requirements as appropriate;
- retain authored and resolved spatial evidence;
- participate in operational and condition state only where meaningful.

### 5.3 Space Objects

Rooms, storeys, entrance zones, halls, studies, bathrooms, kitchens, utility rooms, and bedrooms must:

- remain semantic/spatial containers rather than synthetic room solids unless exact space boundaries are authored later;
- declare occupancy or activity functions;
- declare containment and access requirements;
- aggregate installed fixtures and loose contents;
- expose explicit `PendingEvidence` for unsupported usable-area, headroom, circulation, daylight, ventilation, or egress claims.

### 5.4 System Objects

Structural, envelope, plumbing, electrical, HVAC, fire-safety, roof, landscape, lighting, and external-works systems must:

- declare system roles and functions;
- own system-level continuity or completeness requirements;
- aggregate component ports, flows, and state;
- avoid claiming analytical simulation not provided by the implementation.

### 5.5 Relationship and Semantic Objects

Relationship assertion objects must:

- retain zero geometry;
- reference exactly one canonical containment, connection, functional dependency, service flow, or monitoring relationship where the source meaning is executable;
- remain compatibility evidence rather than duplicate graph authority;
- report `PendingEvidence` when the legacy assertion is not executable;
- never create inferred endpoints from name similarity.

## 6. Target Aggregate Object Model

```cpp
class SmallModernBuildingModel
{
public:
    const SpatialBuildingModel &spatialModel() const;
    const BuildingClassificationModel &classificationModel() const;
    const BuildingFunctionModel &functionModel() const;
    const BuildingServiceModel &serviceModel() const;
    const BuildingRequirementModel &requirementModel() const;
    const BuildingScenarioModel &scenarioModel() const;
    const BuildingEvidenceLedger &evidenceLedger() const;
    std::uint64_t modelHash() const;
};
```

`SmallModernBuildingModel` composes purpose-specific models. It must not become a mutable property bag.

## 7. Classification and Role Model

### 7.1 Purpose Classes

```text
BuildingConceptId
BuildingConceptDefinition
BuildingClassificationModel
BuildingRoleId
BuildingRoleDefinition
BuildingObjectRoleAssignment
BuildingObjectSemanticProfile
BuildingModelApplicability
```

### 7.2 Building Concept Definition

```cpp
class BuildingConceptDefinition
{
public:
    const BuildingConceptId &conceptId() const;
    const std::string &canonicalName() const;
    const std::optional<BuildingConceptId> &broaderConceptId() const;
    const std::vector<std::string> &acceptedAliases() const;
    bool geometryExpected() const;
    bool containerExpected() const;
};
```

Concept inheritance is allowed only for real “is-a” relationships:

```text
BuildingObjectConcept
├── SpatialContainerConcept
├── PhysicalElementConcept
├── BuildingSystemConcept
├── BuildingSpaceConcept
└── SemanticEvidenceConcept
```

Concrete concepts such as `Window`, `Sink`, `StructuralSlab`, and `HVACEquipment` narrow those real categories.

### 7.3 Role Assignment

```cpp
class BuildingObjectRoleAssignment
{
public:
    const SpatialObjectId &objectId() const;
    const BuildingRoleId &roleId() const;
    const BuildingEvidenceReference &evidence() const;
};
```

Roles must not be encoded by multiplying object subclasses.

### 7.4 Classification Validation

`BuildingClassificationValidationService` must reject:

```text
undefined concept IDs
concept hierarchy cycles
duplicate canonical names
conflicting object concept assignments
geometry-bearing semantic-only concepts
container-only concepts assigned to physical leaves without explicit compatibility evidence
unresolved role IDs
duplicate role assignments
```

## 8. Function Model

### 8.1 Purpose Classes

```text
BuildingFunctionId
BuildingFunction
BuildingFunctionModel
BuildingFunctionAllocationRelationship
BuildingFunctionalDependencyRelationship
BuildingFunctionCoverageResult
```

### 8.2 Function Definition

```cpp
class BuildingFunction
{
public:
    const BuildingFunctionId &functionId() const;
    const std::string &purpose() const;
    BuildingFunctionCriticality criticality() const;
    const std::vector<BuildingFunctionId> &requiredDependencyIds() const;
};
```

Initial function vocabulary:

```text
DefineBuildingSite
DefinePropertyExtent
ProvideGroundSupport
ProvidePedestrianAccess
ProvideVehicleAccess
ProvideLandscapeAmenity
TransferBuildingLoads
ProvideHorizontalSupport
ProvideVerticalSupport
ProvideBuildingStability
ProvideWeatherProtection
ControlRainwater
ProvideDaylight
ProvideView
ProvideEntrance
SupportOccupancy
SupportLivingActivity
SupportCookingActivity
SupportDiningActivity
SupportSleepingActivity
SupportStudyActivity
SupportSanitationActivity
SupportUtilityActivity
ProvideVerticalCirculation
ProvideFurnitureSupport
ProvidePotableColdWater
ProvidePotableHotWater
RemoveWasteWater
DistributeElectricalPower
ProvideInteriorLighting
ProvideExteriorLighting
ConditionIndoorAir
ExtractBathroomAir
DetectSmoke
ProvidePortableFireSuppression
GenerateElectricalPower
ProvideOutdoorAmenity
DefineSiteSecurityBoundary
RecordSemanticRelationshipEvidence
```

### 8.3 Function Allocation Rules

- Every non-relationship object must have at least one applicable role.
- Every building space and system container must have at least one allocated function.
- A physical object may contribute to several functions.
- A function may be fulfilled by several objects.
- Relationship assertion objects may use `NotApplicable` for direct building function while performing `RecordSemanticRelationshipEvidence` as an evidence role.
- Every hard function must have at least one allocation before release.
- Function coverage does not prove performance. It proves only that declared model objects are assigned to the function.

## 9. Service Model

### 9.1 Purpose Classes

```text
BuildingServiceSystemId
BuildingServiceSystem
BuildingServicePortId
BuildingServicePort
BuildingServiceFlowId
BuildingServiceFlow
BuildingServiceMedium
BuildingServiceFlowDirection
BuildingServiceFlowGraph
BuildingServiceContinuityResult
```

### 9.2 Port Relationship to Spatial Interfaces

A `BuildingServicePort` references zero or one `SpatialInterface`:

```text
BuildingServicePort
├── object owner
├── service medium
├── direction
├── optional spatial interface reference
├── optional nominal capacity metadata
└── evidence reference
```

The service port does not duplicate the spatial interface frame. If no spatial interface exists, the port must report `PendingEvidence` for spatial localization.

### 9.3 Initial Service Media

```text
ElectricalPower
PotableColdWater
PotableHotWater
WasteWater
ConditionedAir
ExhaustAir
RefrigerantCircuit
SmokeMonitoringSignal
ControlSignal
Rainwater
```

### 9.4 P0 Service Authority

SMB-OMv2.1 P0 validates topology only:

```text
port compatibility
direction consistency
required source reachability
required sink reachability
missing endpoints
prohibited self-loops
declared cycle policy
orphan ports
duplicate flow IDs
deterministic graph traversal
```

It does not claim:

```text
hydraulic pressure
pipe sizing
electrical voltage drop
circuit protection
airflow rate
thermal load
refrigerant thermodynamics
fire-code certification
```

## 10. Requirement Model

### 10.1 Real Requirement Hierarchy

```text
BuildingRequirement
├── IdentityRequirement
├── ClassificationRequirement
├── ContainmentRequirement
├── ConnectionRequirement
├── ClearanceRequirement
├── CollisionAvoidanceRequirement
├── FunctionalCoverageRequirement
├── ServiceContinuityRequirement
├── BoundaryRequirement
├── StateRequirement
└── ScenarioRequirement
```

Inheritance is used because each concrete class is a real kind of building requirement with distinct evaluation data.

### 10.2 Common Requirement Contract

```cpp
class BuildingRequirement
{
public:
    virtual ~BuildingRequirement() = default;
    virtual const BuildingRequirementId &requirementId() const = 0;
    virtual BuildingRequirementKind kind() const = 0;
    virtual BuildingRequirementCriticality criticality() const = 0;
    virtual const BuildingRequirementTarget &target() const = 0;
    virtual const std::vector<BuildingRequirementId> &dependencyIds() const = 0;
};
```

### 10.3 Evaluation Record

```cpp
class BuildingRequirementEvaluationRecord
{
public:
    const BuildingRequirementId &requirementId() const;
    BuildingRequirementEvaluationStatus status() const;
    const BuildingMeasuredValue &measuredValue() const;
    const BuildingRequiredValue &requiredValue() const;
    float tolerance() const;
    const std::vector<BuildingEvidenceReference> &evidenceReferences() const;
    const std::vector<std::string> &diagnostics() const;
    std::uint64_t evidenceHash() const;
};
```

### 10.4 Criticality

```text
Hard
Soft
Informational
```

- A `Hard` requirement must evaluate `Passed` for release.
- A `Soft` requirement may remain `Unknown` only when the release report names it explicitly.
- An `Informational` requirement records evidence without gating release.
- `NotEvaluated` is never an acceptable final status for an applicable hard requirement.

### 10.5 Requirement Dependencies

The requirement dependency graph must be acyclic. Evaluation order is topological and source-order independent.

Examples:

```text
sink service continuity depends on port compatibility
cabinet wall clearance depends on resolved placement evidence
solar electrical contribution depends on the power-flow connection
space function completeness depends on child object function allocations
system completeness depends on required component profiles
```

## 11. State and Scenario Model

### 11.1 Orthogonal State Facets

Do not replace the existing spatial object state. Add purpose-specific facets:

```text
BuildingPlacementState
BuildingOperationalState
BuildingConditionState
BuildingComplianceState
BuildingServiceAvailabilityState
```

Example:

```text
SMB_001_HVAC_LivingIndoorUnit
    placement = ContactResolved
    operation = Available
    condition = Serviceable
    compliance = Passed
    service availability = Connected
```

### 11.2 Purpose Classes

```text
BuildingScenarioId
BuildingScenario
BuildingScenarioParticipation
BuildingStateSnapshot
BuildingStateTransitionRecord
BuildingScenarioStateTransitionService
```

### 11.3 Initial Scenarios

```text
AuthoredConfiguration
ResolvedPlacement
NormalOccupancy
BuildingUnoccupied
ElectricalPowerUnavailable
ColdWaterUnavailable
HotWaterUnavailable
HVACUnavailable
SmokeDetectedGroundFloor
SmokeDetectedUpperFloor
MaintenanceInspection
```

P0 scenario evaluation is discrete and deterministic. It does not run time-stepped physical simulation.

## 12. Evidence Model

### 12.1 Purpose Classes

```text
BuildingEvidenceId
BuildingEvidenceReference
BuildingEvidenceRecord
BuildingEvidenceLedger
BuildingEvidenceKind
BuildingEvidenceAuthority
BuildingEvidenceHashService
```

### 12.2 Evidence Kinds

```text
AuthoredSource
GeneratedManifest
SpatialResolution
GeometryBinding
InterfaceCompatibility
ConnectionGraph
ServiceTopology
RequirementEvaluation
ScenarioEvaluation
VisualInspection
CompatibilityGate
```

### 12.3 Evidence Authority

```text
Authoritative
Supporting
Documentary
Pending
```

Visual evidence is supporting. Source, graph, deterministic evaluation, collision, residual, and exact-hash evidence are authoritative where their declared capability applies.

### 12.4 Hashes

The acceptance artifact must include:

```text
concept catalog hash
object semantic profile hash
function allocation hash
functional dependency hash
service topology hash
requirement definition hash
requirement evaluation hash
scenario definition hash
scenario snapshot hash
aggregate SMB-OMv2.1 model hash
```

Repeated generation from identical SMB-OMv2 input must reproduce every hash exactly.

## 13. Independent Graphs and Relationships

SMB-OMv2.1 must preserve separate graph owners:

```text
SpatialContainmentTree
    answers: what contains this object?

SpatialConnectionGraph
    answers: what spatial interfaces are connected?

SpatialConstraintGraph
    answers: what placement dependencies determine transforms?

BuildingFunctionGraph
    answers: what functions depend on other functions?

BuildingServiceFlowGraph
    answers: what directed service medium flows between ports?

BuildingRequirementDependencyGraph
    answers: what evidence or prerequisite requirement must resolve first?
```

Do not create a universal relationship graph that erases these semantics.

## 14. All-Object Branch Implementation Program

### 14.1 Building Root — 1 Object

Objects:

```text
SMB_001
```

Required development:

- assign `SmallModernBuilding` as the canonical concept;
- allocate whole-building functions for shelter, occupancy support, services, safety, and external works;
- reference all nine root branches;
- require exactly 123 contained descendants and 124 total profiles;
- aggregate hard requirement status without replacing child records;
- expose the aggregate model hash and unresolved-object count;
- fail if any accepted object lacks a semantic profile.

Gate:

```text
root profile exists exactly once
all root branches resolve
all-object count equals 124
aggregate hard requirement status is Passed
```

### 14.2 Site Branch — 10 Objects

Required roles and functions:

```text
site definition
property extent
terrain support
pedestrian access
vehicle access
landscape organization
planting containment
outdoor amenity
```

Required requirements:

- property boundary contains authored site elements where exact boundaries permit evaluation;
- ground surface supports building and external-work objects through declared or pending interfaces;
- entry path and driveway remain inside the site boundary;
- planters and trees remain contained by the landscape system;
- geometry-bearing site leaves retain finite boundaries;
- no ecological, drainage, root-zone, or accessibility performance is inferred without evidence.

Gate:

```text
10/10 profiles
10/10 concept assignments
no orphan landscape child
all geometry applicability records agree with SMB-OMv2
site requirements have explicit statuses
```

### 14.3 Structure Branch — 11 Objects

Required roles and functions:

```text
foundation support
horizontal load transfer
vertical load transfer
building stability
storey support
roof support
core stability and circulation enclosure
```

Required structural topology:

```text
foundation -> ground slab
foundation -> structural columns
columns/core -> upper slab
columns/core -> roof slab
```

This topology is semantic evidence only in P0. It is not finite-element or code-compliance analysis.

Required requirements:

- all structural leaves have finite geometry bindings and boundaries;
- all four columns belong to the column system;
- ground, upper, and roof slabs belong to the structural system;
- the load-path graph reaches the foundation for every supported slab;
- structural objects reject service or furniture collision classifications;
- unresolved exact bearing surfaces remain explicit.

Gate:

```text
11/11 profiles
load-path topology connected
zero topology cycles
zero inferred structural capacities
```

### 14.4 Ground Floor Branch — 32 Objects

Required space functions:

```text
entrance
living
cooking
dining
sanitation
utility
vertical circulation
```

Required physical roles:

```text
floor finish
glazing
door closure
weather canopy
furniture
cabinetry
work surface
plumbing fixture
appliance
lighting
partition
sanitary fixture
stair flight
```

Required requirements:

- every room container owns the expected accepted child objects;
- all loose furniture retains explicit floor-support evidence or `PendingEvidence`;
- cabinet support and wall clearance use existing SMB-OMv2 resolution evidence;
- the sink is supported by the benchtop and connected to cold-water, hot-water, and waste service paths;
- sanitary fixtures declare water and waste applicability without inventing missing executable ports;
- room lights allocate interior-lighting function and later connect to ground lighting service topology;
- the water heater participates in hot-water supply topology;
- the stair flight belongs to the stair system and participates in vertical-circulation function;
- occupancy capacity, accessibility, appliance power, headroom, and egress remain pending unless exact evidence is added.

Gate:

```text
32/32 profiles
all seven ground-floor space functions allocated
existing cabinet and sink predicates reproduced by requirement evaluation
all loose contents have explicit support status
```

### 14.5 Upper Floor Branch — 21 Objects

Required space functions:

```text
upper circulation
sleeping
clothing storage
sanitation
study
daylight and view
```

Required requirements:

- both bedroom containers own floor, bed, wardrobe, and window profiles;
- the upper bathroom owns partition, vanity, toilet, and shower profiles;
- the study owns desk, chair, and window profiles;
- all three windows retain their accepted facade-opening connections;
- loose beds, wardrobes, desk, and chair set retain explicit floor-support evidence;
- upper floor lights represented through the electrical aggregate remain distinguishable from room contents;
- usable area, daylight, ventilation, privacy, and escape claims remain pending without exact evidence.

Gate:

```text
21/21 profiles
bedroom profile symmetry checked without forcing identical authored geometry
3/3 windows connected to authored opening interfaces
all loose upper-floor contents have support status
```

### 14.6 Envelope Branch — 7 Objects

Required roles and functions:

```text
weather boundary
facade enclosure
solar shading
rainwater collection and conveyance
opening host
```

Required requirements:

- four facade objects form the declared envelope set;
- external fins allocate shading role without claiming measured solar performance;
- downpipes use plumbing/rainwater service semantics rather than envelope collision classification;
- window opening interfaces remain semantic until exact surface boundaries exist;
- envelope continuity is a topological requirement in P0, not a watertight mesh claim;
- facade geometry remains finite and bound to the accepted objects.

Gate:

```text
7/7 profiles
4/4 facade members present
downpipe service medium is Rainwater
no Boolean opening or exact weather-tightness claim without evidence
```

### 14.7 Roof Zone Branch — 5 Objects

Required roles and functions:

```text
roof enclosure
edge protection representation
solar power generation
roof equipment support
outdoor HVAC service
```

Required requirements:

- roof system contains parapet, solar array, and outdoor HVAC profiles;
- photovoltaic module set remains one aggregate object owning repeated panel geometry;
- solar array reaches the electrical system through a directed power flow;
- outdoor HVAC remains supported by the roof slab using existing resolved placement evidence;
- parapet height compliance, photovoltaic capacity, wind loading, and equipment maintenance clearance remain pending unless measured evidence is added.

Gate:

```text
5/5 profiles
solar aggregate geometry count preserved
power-flow path reaches electrical system
outdoor HVAC support requirement passes
```

### 14.8 Services Branch — 18 Objects

Required system models:

```text
PlumbingSystem
ElectricalSystem
HVACSystem
FireSafetySystem
```

Required plumbing topology:

```text
cold-water riser -> cold-water consumers
water heater -> hot-water riser
hot-water riser -> hot-water consumers
waste-producing fixtures -> sanitary stack
```

Required electrical topology:

```text
solar array -> electrical system
main panel -> ground lighting circuit
main panel -> upper lighting circuit
lighting circuits -> luminaire sets
```

Required HVAC topology:

```text
outdoor HVAC unit -> living indoor unit
outdoor HVAC unit -> bedroom indoor unit
bathroom exhaust -> exhaust-air outlet pending exact endpoint
```

Required fire-safety topology:

```text
ground smoke alarm -> ground-floor monitoring zone
upper smoke alarm -> upper-floor monitoring zone
fire extinguisher -> portable suppression function
```

Required requirements:

- every service system and component has an explicit service-port applicability record;
- every executable service assertion maps to one directed flow or monitoring relationship;
- no capacity or regulatory compliance is inferred from connectivity;
- aggregate luminaire objects preserve repeated primitive ownership;
- unresolved fixture ports and external source/sink endpoints remain pending.

Gate:

```text
18/18 profiles
all executable service paths deterministic
zero orphan executable ports
zero incompatible service media
no unsupported analytical claims
```

### 14.9 External Works Branch — 5 Objects

Required roles and functions:

```text
outdoor amenity
site boundary/security representation
external illumination
external lighting aggregation
```

Required requirements:

- patio remains within the site and is supported by terrain where evaluable;
- perimeter fence references the property/site boundary relationship without inventing exact coincidence;
- external luminaires allocate exterior-lighting function and electrical service applicability;
- the external luminaire set retains four accepted primitive bindings under one stable object;
- lighting levels, security compliance, and accessibility remain pending without measurements.

Gate:

```text
5/5 profiles
external luminaire aggregation preserved
all external objects have site containment status
```

### 14.10 Relationship Branch — 14 Objects

Required development:

- retain the relationship system and semantic leaf as zero-geometry objects;
- map five containment assertions to the canonical containment tree;
- map sink cold-water, water-heater hot-water, two HVAC service, solar power, and two smoke-monitoring assertions to canonical connection or service relationships;
- retain exact source provenance for every assertion;
- ensure each assertion references one canonical fact and does not become a second authority;
- report any non-executable semantic leaf as documentary or pending.

Gate:

```text
14/14 profiles
12/12 assertion objects reference canonical relationships
zero invented geometry
zero duplicate graph authority
zero unresolved executable endpoints
```

## 15. Canonical Semantic Profile Schema

The generated knowledge model should use a versioned schema:

```json
{
  "schema": "ProGen3D-SMB-OMv2.1-BuildingKnowledgeModel-v1",
  "source_spatial_schema": "SMB-OMv2-SpatialObjectModel-v1",
  "source_object_count": 124,
  "model_revision": 1,
  "concept_catalog": [],
  "role_catalog": [],
  "functions": [],
  "functional_allocations": [],
  "functional_dependencies": [],
  "service_systems": [],
  "service_ports": [],
  "service_flows": [],
  "requirements": [],
  "requirement_dependencies": [],
  "object_profiles": [],
  "scenarios": [],
  "evidence_records": [],
  "deterministic_hashes": {}
}
```

### 15.1 Object Profile Record

```json
{
  "object_id": "SMB_001_Ground_Kitchen_Sink",
  "concept_id": "BuildingConcept.Sink",
  "role_ids": [
    "BuildingRole.WashingFixture",
    "BuildingRole.PotableWaterConsumer",
    "BuildingRole.WasteWaterProducer",
    "BuildingRole.BenchtopSupportedFixture"
  ],
  "function_allocation_ids": [
    "FunctionAllocation.KitchenSink.ProvideWashingFacility"
  ],
  "service_port_ids": [
    "ServicePort.KitchenSink.ColdWaterIn",
    "ServicePort.KitchenSink.HotWaterIn",
    "ServicePort.KitchenSink.WasteWaterOut"
  ],
  "requirement_ids": [
    "Requirement.KitchenSink.SupportedByBenchtop",
    "Requirement.KitchenSink.ColdWaterContinuity",
    "Requirement.KitchenSink.HotWaterContinuity",
    "Requirement.KitchenSink.WasteWaterContinuity"
  ],
  "applicability": {
    "geometry": "Applicable",
    "spatial_boundary": "Applicable",
    "spatial_interfaces": "Applicable",
    "building_functions": "Applicable",
    "service_ports": "Applicable",
    "requirements": "Applicable",
    "operational_state": "Applicable",
    "condition_state": "Applicable",
    "scenario_participation": "Applicable"
  },
  "profile_revision": 1,
  "profile_hash": 0
}
```

## 16. Generator Architecture

### 16.1 Preserve the Existing Generator

Do not duplicate the SMB-OMv2 hierarchy logic.

Add:

```text
examples/SMB_OMv2_Instance_SMB_001/
    generate_smb_omv21.py
    smb_omv21_concept_catalog.py
    smb_omv21_role_catalog.py
    smb_omv21_function_definitions.py
    smb_omv21_service_definitions.py
    smb_omv21_requirement_definitions.py
    smb_omv21_scenario_definitions.py
```

`generate_smb_omv21.py` should:

1. invoke or import the existing SMB-OMv2 generator;
2. load the generated 124-object spatial manifest;
3. verify the expected source schema and exact object ID set;
4. apply explicit semantic profiles keyed by stable object ID;
5. reject unknown, duplicate, or missing object profiles;
6. validate all catalog and graph references;
7. evaluate deterministic requirements supported at generation time;
8. emit the knowledge model and coverage CSV;
9. regenerate twice and prove byte-identical output in acceptance tests.

### 16.2 Generator Data Rules

- Use explicit object-ID keyed records, not name-fragment inference.
- Reusable profile templates may reduce repetition but must expand to explicit per-object records.
- A template may not assign a service port or requirement solely because an object name contains a matching word.
- Repeated aggregate objects such as `SMB_StairStep`, `SMB_SolarPanel`, `SMB_CeilingLight`, and `SMB_ExternalLight` retain one semantic profile each.
- Unresolved concepts remain explicit and fail closed.
- The coverage CSV is generated from the same in-memory records as the JSON model.

## 17. Runtime Construction Architecture

### 17.1 Construction Context

```text
SmallModernBuildingModelConstructionContext
├── concept definitions
├── role definitions
├── object profiles
├── functions and allocations
├── service systems, ports, and flows
├── requirements and dependencies
├── scenarios
├── provisional evaluation records
└── diagnostics
```

### 17.2 Construction Service

```cpp
class SmallModernBuildingModelConstructionService
{
public:
    SmallModernBuildingModelConstructionResult construct(
        const SpatialBuildingModel &spatial_model,
        const SmallModernBuildingModelConstructionRequest &request) const;
};
```

Construction order:

```text
validate source spatial model
load catalogs
construct semantic profiles
validate object coverage
construct function graph
construct service-flow graph
construct requirement dependency graph
evaluate source-derived requirements
construct scenario catalog
hash immutable records
publish aggregate model atomically
```

No partial `SmallModernBuildingModel` is published on failure.

### 17.3 Snapshot Integration

The target snapshot relationship is:

```text
GeneratedSceneSnapshot
├── GrammarDocument
├── SceneGenerationContext
│   └── ScenePrimitiveInstance[]
├── SpatialBuildingModel
└── SmallModernBuildingModel
```

The knowledge model references stable spatial IDs and never owns primitive instances.

## 18. Requirement Evaluation Architecture

### 18.1 Evaluation Services

```text
BuildingIdentityRequirementEvaluationService
BuildingClassificationRequirementEvaluationService
BuildingContainmentRequirementEvaluationService
BuildingConnectionRequirementEvaluationService
BuildingClearanceRequirementEvaluationService
BuildingCollisionRequirementEvaluationService
BuildingFunctionCoverageEvaluationService
BuildingServiceContinuityEvaluationService
BuildingBoundaryRequirementEvaluationService
BuildingScenarioRequirementEvaluationService
```

### 18.2 Evaluation Context

```text
BuildingRequirementEvaluationContext
├── SpatialBuildingModel
├── BuildingClassificationModel
├── BuildingFunctionModel
├── BuildingServiceModel
├── current scenario
├── source evidence index
└── numerical tolerance policy
```

### 18.3 Reproduce Existing Priority Validity

The following current acceptance predicates must become first-class requirements while retaining equivalent acceptance behavior:

```text
cabinet supported and wall-cleared
sink supported and connected to cold, hot, and waste plumbing
water heater connected to hot-water riser
both indoor HVAC units served by outdoor HVAC unit
solar array connected to electrical system
both smoke alarms connected to monitored floors
three windows seated in authored facade openings
all loose furniture supported by the correct floor
```

The existing harness remains green while the new evaluator proves parity.

## 19. State Transition Architecture

### 19.1 Transition Authority

Public code must not assign arbitrary state values. State changes occur through:

```text
BuildingPlacementStateTransitionService
BuildingOperationalStateTransitionService
BuildingConditionStateTransitionService
BuildingComplianceStateTransitionService
BuildingServiceAvailabilityTransitionService
```

### 19.2 Scenario Transaction

A scenario evaluation must:

1. clone the prior immutable state snapshot;
2. apply declared input state changes;
3. evaluate affected service paths;
4. evaluate dependent functions;
5. evaluate dependent requirements;
6. publish a new snapshot only when all records are internally consistent;
7. retain the prior snapshot for rollback and comparison.

## 20. Editor Integration

### 20.1 Object Inspector

The selected object inspector should add:

```text
canonical concept
taxonomy path
roles
allocated functions
service ports
incoming and outgoing flows
applicable requirements
evaluation status
operational state
condition state
scenario participation
evidence references
profile hash
```

Geometry-free container and relationship objects must remain inspectable.

### 20.2 Building Views

Add purpose-specific views:

```text
Classification View
Function Allocation View
Service Flow View
Requirement Status View
Scenario State View
Evidence View
```

Do not overload the existing containment tree with unrelated edges.

### 20.3 Overlays

Optional overlays:

```text
service port markers
directed service-flow lines
requirement target highlights
failed requirement markers
pending evidence markers
scenario-unavailable objects
```

Visual overlays support inspection but do not replace graph and evidence gates.

## 21. Proposed Source Layout

```text
include/building/
    graph/
        BuildingFunctionGraph.h
        BuildingRequirementDependencyGraph.h
        BuildingServiceFlowGraph.h
    model/
        SmallModernBuildingModel.h
        BuildingClassificationModel.h
        BuildingConceptDefinition.h
        BuildingConceptId.h
        BuildingRoleDefinition.h
        BuildingRoleId.h
        BuildingObjectSemanticProfile.h
        BuildingModelApplicability.h
        BuildingFunction.h
        BuildingFunctionId.h
        BuildingFunctionModel.h
        BuildingFunctionCriticality.h
        BuildingServiceSystem.h
        BuildingServiceSystemId.h
        BuildingServicePort.h
        BuildingServicePortId.h
        BuildingServiceFlow.h
        BuildingServiceFlowId.h
        BuildingServiceMedium.h
        BuildingServiceFlowDirection.h
        BuildingRequirement.h
        BuildingRequirementId.h
        BuildingRequirementKind.h
        BuildingRequirementCriticality.h
        BuildingRequirementEvaluationRecord.h
        BuildingRequirementEvaluationStatus.h
        BuildingRequirementTarget.h
        BuildingMeasuredValue.h
        BuildingRequiredValue.h
        BuildingPlacementState.h
        BuildingOperationalState.h
        BuildingConditionState.h
        BuildingComplianceState.h
        BuildingServiceAvailabilityState.h
        BuildingScenario.h
        BuildingScenarioId.h
        BuildingStateSnapshot.h
        BuildingStateTransitionRecord.h
        BuildingEvidenceRecord.h
        BuildingEvidenceReference.h
        BuildingEvidenceLedger.h
    relationship/
        BuildingObjectRoleAssignment.h
        BuildingFunctionAllocationRelationship.h
        BuildingFunctionalDependencyRelationship.h
        BuildingRequirementApplicabilityRelationship.h
        BuildingScenarioParticipation.h
    service/
        SmallModernBuildingModelConstructionService.h
        SmallModernBuildingModelConstructionContext.h
        SmallModernBuildingModelValidationService.h
        SmallModernBuildingDeterministicHashService.h
        BuildingClassificationValidationService.h
        BuildingFunctionCoverageEvaluationService.h
        BuildingServiceTopologyValidationService.h
        BuildingRequirementEvaluationService.h
        BuildingScenarioEvaluationService.h
        BuildingScenarioStateTransitionService.h
        BuildingEvidenceHashService.h

src/building/
    graph/
    model/
    service/
```

Dedicated concrete requirement headers should be added only when their data or evaluation behavior differs materially:

```text
IdentityRequirement
ClassificationRequirement
ContainmentRequirement
ConnectionRequirement
ClearanceRequirement
CollisionAvoidanceRequirement
FunctionalCoverageRequirement
ServiceContinuityRequirement
BoundaryRequirement
StateRequirement
ScenarioRequirement
```

## 22. Implementation Milestones

### Milestone 0: Freeze Accepted Contracts

Tasks:

```text
record exact SMB-OMv1 source hash
record exact SMB-OMv2 generator hash
record exact spatial manifest hash
record exact 124-object ID set
record accepted object, interface, connection, constraint, and binding counts
record current native and visual evidence paths
```

Gate: existing SMB-OMv1 and SMB-OMv2 checks pass without modification.

### Milestone 1: Classification and Profile Kernel

Tasks:

```text
implement concept and role models
implement semantic profile and applicability models
implement classification validation
implement deterministic profile hashing
construct programmatic valid and invalid fixtures
```

Gate: duplicate concepts, hierarchy cycles, unknown roles, missing applicability, and conflicting classifications fail closed.

### Milestone 2: Function Kernel

Tasks:

```text
implement function model
implement allocation and dependency relationships
implement function coverage evaluation
implement deterministic function graph hashing
```

Gate: every required function has an allocation; dependency cycles and orphan allocations fail.

### Milestone 3: Service Kernel

Tasks:

```text
implement service systems, ports, media, directions, and flows
implement service-flow graph
implement topology validation
implement reachability evidence
```

Gate: wrong media, wrong direction, missing endpoints, orphan ports, prohibited cycles, and nondeterministic traversal fail.

### Milestone 4: Requirement Kernel

Tasks:

```text
implement requirement hierarchy
implement criticality and applicability
implement dependency graph
implement evaluation records and evidence hashes
implement source-order-independent evaluation
```

Gate: all hard requirements evaluate deterministically; unknown hard requirements block publication.

### Milestone 5: State and Scenario Kernel

Tasks:

```text
implement orthogonal state facets
implement legal transition services
implement scenarios and immutable snapshots
implement scenario transactions and rollback
```

Gate: invalid transitions and partial scenario publication fail closed.

### Milestone 6: All-Object Generator Skeleton

Tasks:

```text
add SMB-OMv2.1 generator wrapper
load exact spatial object inventory
emit one skeleton profile per object
emit explicit applicability for every dimension
emit coverage CSV
regenerate twice
```

Gate: exactly 124 unique profiles and byte-identical outputs.

### Milestone 7: Site, Structure, Envelope, Roof, and External Works

Tasks:

```text
populate 38 object profiles
add structural topology
add envelope and rainwater semantics
add roof solar and HVAC semantics
add external works functions and requirements
```

Gate: all five branch acceptance sections pass with no inferred analytical claims.

### Milestone 8: Ground and Upper Occupied Floors

Tasks:

```text
populate 53 object profiles
add space functions
add loose-object support requirements
add fixture roles
add window and opening requirements
add cabinet and sink requirement parity
```

Gate: 53/53 profiles pass and the current priority-validity behavior is preserved.

### Milestone 9: Building Services

Tasks:

```text
populate 18 service profiles
add plumbing topology
add electrical topology
add HVAC topology
add fire-safety monitoring topology
```

Gate: all executable service paths resolve with zero incompatible media or orphan executable ports.

### Milestone 10: Relationship Compatibility Migration

Tasks:

```text
populate 14 relationship profiles
map 12 assertions to canonical facts
retain semantic leaf documentary evidence
prove zero duplicate authority
```

Gate: relationship objects remain geometry-free and reference exact canonical relationships.

### Milestone 11: Runtime and Snapshot Integration

Tasks:

```text
construct aggregate runtime model
publish atomically with generated scene snapshot
add source and object associations
add deterministic aggregate hashes
```

Gate: invalid knowledge models do not publish and do not alter the accepted spatial scene.

### Milestone 12: Editor and Visual Evidence

Tasks:

```text
add object semantic inspector
add classification, function, service, requirement, scenario, and evidence views
add optional overlays
capture deterministic visual evidence twice
inspect retained capture directly
```

Gate: all 124 profiles are inspectable, geometry-free objects remain visible in the inspector, and two captures are byte-identical.

### Milestone 13: Full Acceptance and Compatibility Matrix

Tasks:

```text
run focused model harnesses
run all-object acceptance
run invalid fixtures
run SMB-OMv1 compatibility
run SMB-OMv2 compatibility
run visual evidence
run supervisor matrix
run full editor release checks
audit requirement-to-evidence coverage
```

Gate: every completion condition in Section 28 passes.

## 23. Test Plan

### 23.1 Focused Harnesses

```text
tests/building_classification_model_harness.cpp
tests/building_function_model_harness.cpp
tests/building_service_flow_model_harness.cpp
tests/building_requirement_model_harness.cpp
tests/building_scenario_model_harness.cpp
tests/building_evidence_hash_harness.cpp
tests/smb_omv21_all_objects_harness.cpp
tests/smb_omv21_editor_model_harness.cpp
```

### 23.2 Test Scripts

```text
tests/run_building_classification_checks.sh
tests/run_building_function_checks.sh
tests/run_building_service_flow_checks.sh
tests/run_building_requirement_checks.sh
tests/run_building_scenario_checks.sh
tests/run_smb_omv21_checks.sh
tests/run_smb_omv21_visual_checks.sh
tests/run_smb_omv21_supervisor_matrix.sh
```

### 23.3 Required Invalid Fixtures

```text
duplicate object semantic profile
missing accepted object profile
unknown object ID
undefined concept
concept hierarchy cycle
undefined role
function without allocation
functional dependency cycle
service port with unknown owner
service flow with incompatible medium
service flow with invalid direction
orphan required service port
requirement with unknown target
requirement dependency cycle
applicable hard requirement not evaluated
non-finite measured value
invalid state transition
scenario with unknown participant
relationship assertion with duplicate authority
semantic object with invented geometry
non-deterministic generation
```

### 23.4 All-Object Coverage Tests

The all-object harness must require:

```text
124 source spatial objects
124 semantic profiles
124 unique concept assignments
124 complete applicability records
zero unknown profile object IDs
zero missing profile object IDs
zero duplicate profile object IDs
all geometry applicability values match source has_geometry
all relationship and semantic objects have zero geometry bindings
all profile hashes are nonzero
all profile hashes repeat exactly
```

### 23.5 Branch Count Tests

```text
building root = 1
site = 10
structure = 11
ground floor = 32
upper floor = 21
envelope = 7
roof zone = 5
services = 18
external works = 5
relationships = 14
```

### 23.6 Requirement Coverage Tests

```text
every object has identity, classification, and containment requirements
every applicable physical object has boundary status
every applicable movable object has support status
every service port has continuity applicability
every hard function has allocation coverage
every hard requirement has one final evaluation record
every evaluation record cites evidence
every evidence hash is nonzero
zero Passed evaluations depend on Unknown prerequisites
```

### 23.7 Determinism

Run the full generator and runtime construction twice. Require exact equality for:

```text
knowledge-model JSON bytes
coverage CSV bytes
catalog hash
profile hash
function graph hash
service graph hash
requirement definition hash
requirement evaluation hash
scenario hash
aggregate model hash
visual capture bytes
```

### 23.8 Compatibility

Required compatibility commands remain:

```bash
./tests/run_smb_omv1_checks.sh
./tests/run_smb_omv2_checks.sh
./tests/run_smb_omv2_visual_checks.sh
```

The new matrix adds SMB-OMv2.1 checks without weakening those gates.

## 24. Acceptance Evidence Schema

```json
{
  "schema": "SMB-OMv2.1-AllObjectsAcceptanceEvidence-v1",
  "source_object_count": 124,
  "semantic_profile_count": 124,
  "classified_object_count": 124,
  "complete_applicability_count": 124,
  "function_count": 0,
  "function_allocation_count": 0,
  "service_system_count": 0,
  "service_port_count": 0,
  "service_flow_count": 0,
  "requirement_count": 0,
  "hard_requirement_count": 0,
  "passed_hard_requirement_count": 0,
  "failed_hard_requirement_count": 0,
  "unknown_hard_requirement_count": 0,
  "scenario_count": 0,
  "evidence_record_count": 0,
  "branch_coverage": {},
  "priority_requirement_parity": {},
  "compatibility_gates": {},
  "deterministic_hashes": {},
  "passed": false
}
```

Counts shown as zero are runtime-produced values, not expected final values. The final acceptance harness must compare them against explicit constants generated from the reviewed catalogs.

## 25. Safety and Performance

### 25.1 Safety Ceilings

Add limits for:

```text
maximum concept definitions
maximum role definitions
maximum functions
maximum allocations
maximum functional dependencies
maximum service systems
maximum service ports
maximum service flows
maximum requirements
maximum requirement dependency depth
maximum scenarios
maximum evidence records
maximum diagnostic count
maximum source string length
```

### 25.2 Fail-Closed Conditions

Abort publication for:

```text
profile coverage mismatch
duplicate IDs
unknown references
graph cycles where prohibited
non-finite values
invalid state transitions
failed hard requirements
unknown applicable hard requirements
hash mismatch
source schema mismatch
source object set mismatch
```

### 25.3 Performance Boundary

The 124-object fixture is small enough for deterministic full-graph validation. Prefer readable graph traversal and immutable records over premature optimization.

Required metrics:

```text
construction time
validation time
requirement evaluation time
hashing time
peak model record counts
```

Performance evidence must not replace correctness evidence.

## 26. Risks and Mitigations

### Taxonomy Depth Is Mistaken for Semantic Completeness

Add explicit roles, functions, services, and requirements. Do not add hierarchy levels merely to represent cross-cutting meaning.

### Classification Strings Become Unvalidated Authority

Resolve object concepts through a validated catalog with stable IDs.

### Roles Become Artificial Subclasses

Use `BuildingObjectRoleAssignment`. Reserve inheritance for real conceptual categories.

### Spatial Connections and Service Flows Are Conflated

Retain the spatial graph and add a separate directed service-flow graph.

### Requirements Remain Hidden in Tests

Promote accepted validity predicates to first-class requirement definitions and retain test parity.

### Every Object Is Forced Through One State Machine

Use orthogonal placement, operational, condition, compliance, and service-availability states.

### Missing Evidence Is Treated as Success

Use explicit applicability and evaluation statuses. Unknown applicable hard requirements block release.

### Semantic Assertions Duplicate Graph Authority

Assertion objects reference canonical graph facts and remain evidence objects.

### Generator Data Is Inferred From Names

Use explicit stable object-ID keyed records and fail on unknown IDs.

### All-Object Conversion Hides Kernel Errors

Complete programmatic kernel fixtures before populating all 124 profiles.

### Visual Success Hides Model Failure

Keep graph, requirement, evidence, and deterministic hash gates authoritative.

### New Knowledge Model Changes Spatial Output

Run exact SMB-OMv1 and SMB-OMv2 compatibility gates and compare accepted counts and hashes.

## 27. Non-Goals for the First SMB-OMv2.1 Release

```text
structural finite-element analysis
building-code certification
hydraulic pressure simulation
pipe sizing
electrical load flow
protective-device coordination
thermal-zone simulation
computational fluid dynamics
daylight simulation
energy-rating certification
fire and smoke propagation
occupant evacuation simulation
automatic service-port discovery
automatic requirement inference from object names
automatic geometry repair
automatic regulatory compliance claims
nonzero pose covariance propagation
deformable building objects
silent conflict relaxation
```

Unimplemented capabilities must report `PendingEvidence`, `Unknown`, or an explicit unsupported diagnostic.

## 28. Definition of Complete

SMB-OMv2.1 all-building-object implementation is complete only when:

```text
SMB-OMv1 source remains unchanged
SMB-OMv1 native checks pass
SMB-OMv2 generated spatial output remains compatible
SMB-OMv2 native and visual checks pass
exactly 124 semantic profiles exist
every accepted object ID appears exactly once
every object has one validated concept assignment
every object has a complete applicability record
every applicable object has explicit role assignments
every required building function has allocation coverage
functional dependencies are cycle-free
all executable service ports and flows are valid
all hard requirements evaluate Passed
no Passed requirement depends on Unknown evidence
existing priority validity is reproduced as first-class requirements
relationship assertions reference canonical facts
semantic and relationship objects remain geometry-free
all state transitions are legal and service-controlled
scenario snapshots are immutable and deterministic
all evidence records cite source or derived authority
all required hashes are nonzero and repeat exactly
all invalid fixtures fail with exact diagnostics
all 124 profiles are inspectable in the editor
visual overlays and captures are deterministic
visual evidence remains supporting rather than authoritative
the requirement-to-evidence audit has zero uncovered hard requirements
the supervisor matrix passes
the full editor release gate passes
```

No narrower fixture, green visual capture, or partial branch implementation justifies an all-object completion claim.

## 29. Recommended First Implementation Slice

Implement the classification, function, requirement, and evidence kernel for this existing assembly without changing geometry or grammar:

```text
SMB_001_Ground_Kitchen_01
├── SMB_001_Ground_Kitchen_CabinetRun
├── SMB_001_Ground_Kitchen_Benchtop
├── SMB_001_Ground_Kitchen_Sink
├── SMB_001_Plumbing_ColdWaterRiser
├── SMB_001_Plumbing_HotWaterRiser
└── SMB_001_Plumbing_SanitaryStack
```

Implement:

```text
BuildingConceptDefinition
BuildingRoleDefinition
BuildingObjectSemanticProfile
BuildingFunction
BuildingFunctionAllocationRelationship
BuildingServicePort
BuildingServiceFlow
BuildingRequirement
BuildingRequirementEvaluationRecord
BuildingEvidenceRecord
BuildingClassificationValidationService
BuildingFunctionCoverageEvaluationService
BuildingServiceTopologyValidationService
BuildingRequirementEvaluationService
BuildingEvidenceHashService
```

Prove:

```text
kitchen supports cooking activity
cabinet is supported and wall-cleared
benchtop supports the sink
sink receives cold water
sink receives hot water
sink reaches the sanitary stack
all requirement records cite accepted spatial or connection evidence
all outputs are deterministic
an injected missing hot-water flow rolls back the candidate model
```

After the kernel passes, populate the remaining 118 profiles branch by branch according to Milestones 7 through 10.

## Appendix A. Authoritative 124-Object Inventory

Each object below must receive exactly one `BuildingObjectSemanticProfile`.

### A.1 Building Root — 1

- `SMB_001` — `SmallModernBuilding` — geometry: no

### A.2 Site — 10

- `SMB_001_Site` — `BuildingSite` — geometry: no
- `SMB_001_Site_PropertyBoundary` — `PropertyBoundary` — geometry: yes
- `SMB_001_Site_GroundSurface` — `GroundSurface` — geometry: yes
- `SMB_001_Site_EntryPath` — `PavedPath` — geometry: yes
- `SMB_001_Site_Driveway` — `PavedPath` — geometry: yes
- `SMB_001_Site_Landscape` — `LandscapeSystem` — geometry: no
- `SMB_001_Site_Planter_Left` — `Planter` — geometry: yes
- `SMB_001_Site_Planter_Right` — `Planter` — geometry: yes
- `SMB_001_Site_Tree_Left` — `Tree` — geometry: yes
- `SMB_001_Site_Tree_Right` — `Tree` — geometry: yes

### A.3 Structure — 11

- `SMB_001_Structure` — `StructuralSystem` — geometry: no
- `SMB_001_Structure_Foundation` — `Foundation` — geometry: yes
- `SMB_001_Structure_GroundSlab` — `StructuralSlab` — geometry: yes
- `SMB_001_Structure_UpperSlab` — `StructuralSlab` — geometry: yes
- `SMB_001_Structure_RoofSlab` — `StructuralSlab` — geometry: yes
- `SMB_001_Structure_Columns` — `ColumnSystem` — geometry: no
- `SMB_001_Structure_Column_FL` — `StructuralColumn` — geometry: yes
- `SMB_001_Structure_Column_FR` — `StructuralColumn` — geometry: yes
- `SMB_001_Structure_Column_RL` — `StructuralColumn` — geometry: yes
- `SMB_001_Structure_Column_RR` — `StructuralColumn` — geometry: yes
- `SMB_001_Structure_Core` — `BuildingCore` — geometry: yes

### A.4 Ground Floor — 32

- `SMB_001_GroundFloor` — `GroundStorey` — geometry: no
- `SMB_001_Ground_Entry_01` — `EntranceZone` — geometry: no
- `SMB_001_Ground_Entry_Door_01` — `Door` — geometry: yes
- `SMB_001_Ground_Entry_Canopy_01` — `Canopy` — geometry: yes
- `SMB_001_Ground_LivingRoom_01` — `LivingRoom` — geometry: no
- `SMB_001_Ground_Living_FloorFinish` — `FloorFinish` — geometry: yes
- `SMB_001_Ground_Living_Glazing` — `Glazing` — geometry: yes
- `SMB_001_Ground_Living_Sofa` — `Furniture` — geometry: yes
- `SMB_001_Ground_Living_MediaUnit` — `Furniture` — geometry: yes
- `SMB_001_Ground_Living_Light` — `Luminaire` — geometry: yes
- `SMB_001_Ground_Kitchen_01` — `Kitchen` — geometry: no
- `SMB_001_Ground_Kitchen_CabinetRun` — `Cabinet` — geometry: yes
- `SMB_001_Ground_Kitchen_Benchtop` — `Benchtop` — geometry: yes
- `SMB_001_Ground_Kitchen_Island` — `Cabinet` — geometry: yes
- `SMB_001_Ground_Kitchen_Sink` — `Sink` — geometry: yes
- `SMB_001_Ground_Kitchen_Oven` — `Appliance` — geometry: yes
- `SMB_001_Ground_Kitchen_Light` — `Luminaire` — geometry: yes
- `SMB_001_Ground_DiningRoom_01` — `DiningRoom` — geometry: no
- `SMB_001_Ground_Dining_Table` — `Table` — geometry: yes
- `SMB_001_Ground_Dining_Chairs` — `ChairSet` — geometry: yes
- `SMB_001_Ground_Dining_Light` — `Luminaire` — geometry: yes
- `SMB_001_Ground_Bathroom_01` — `Bathroom` — geometry: no
- `SMB_001_Ground_Bathroom_Partitions` — `Partition` — geometry: yes
- `SMB_001_Ground_Bathroom_Vanity` — `Vanity` — geometry: yes
- `SMB_001_Ground_Bathroom_Toilet` — `Toilet` — geometry: yes
- `SMB_001_Ground_Bathroom_Shower` — `Shower` — geometry: yes
- `SMB_001_Ground_UtilityRoom_01` — `UtilityRoom` — geometry: no
- `SMB_001_Ground_Utility_Partition` — `Partition` — geometry: yes
- `SMB_001_Ground_Utility_Washer` — `Appliance` — geometry: yes
- `SMB_001_Ground_Utility_WaterHeater` — `WaterHeater` — geometry: yes
- `SMB_001_Ground_Stair_01` — `StairSystem` — geometry: no
- `SMB_StairStep` — `StairFlight` — geometry: yes

### A.5 Upper Floor — 21

- `SMB_001_UpperFloor` — `UpperStorey` — geometry: no
- `SMB_001_Upper_Hall_01` — `UpperHall` — geometry: yes
- `SMB_001_Upper_Bedroom_01` — `Bedroom` — geometry: no
- `SMB_001_Upper_Bedroom_01_Floor` — `FloorFinish` — geometry: yes
- `SMB_001_Upper_Bedroom_01_Bed` — `Bed` — geometry: yes
- `SMB_001_Upper_Bedroom_01_Wardrobe` — `Wardrobe` — geometry: yes
- `SMB_001_Upper_Bedroom_01_Window` — `Window` — geometry: yes
- `SMB_001_Upper_Bedroom_02` — `Bedroom` — geometry: no
- `SMB_001_Upper_Bedroom_02_Floor` — `FloorFinish` — geometry: yes
- `SMB_001_Upper_Bedroom_02_Bed` — `Bed` — geometry: yes
- `SMB_001_Upper_Bedroom_02_Wardrobe` — `Wardrobe` — geometry: yes
- `SMB_001_Upper_Bedroom_02_Window` — `Window` — geometry: yes
- `SMB_001_Upper_Bathroom_01` — `Bathroom` — geometry: no
- `SMB_001_Upper_Bathroom_Partition` — `Partition` — geometry: yes
- `SMB_001_Upper_Bathroom_Vanity` — `Vanity` — geometry: yes
- `SMB_001_Upper_Bathroom_Toilet` — `Toilet` — geometry: yes
- `SMB_001_Upper_Bathroom_Shower` — `Shower` — geometry: yes
- `SMB_001_Upper_Study_01` — `Study` — geometry: no
- `SMB_001_Upper_Study_Desk` — `Desk` — geometry: yes
- `SMB_001_Upper_Study_Chair` — `ChairSet` — geometry: yes
- `SMB_001_Upper_Study_Window` — `Window` — geometry: yes

### A.6 Envelope — 7

- `SMB_001_Envelope` — `EnvelopeSystem` — geometry: no
- `SMB_001_Envelope_Front` — `Facade` — geometry: yes
- `SMB_001_Envelope_Rear` — `Facade` — geometry: yes
- `SMB_001_Envelope_Left` — `Facade` — geometry: yes
- `SMB_001_Envelope_Right` — `Facade` — geometry: yes
- `SMB_001_Envelope_ExternalFins` — `FacadeFin` — geometry: yes
- `SMB_001_Envelope_Downpipes` — `PipeRun` — geometry: yes

### A.7 Roof Zone — 5

- `SMB_001_RoofZone` — `RoofSystem` — geometry: no
- `SMB_001_Roof_Parapet` — `Parapet` — geometry: yes
- `SMB_001_Roof_SolarArray` — `PhotovoltaicArray` — geometry: no
- `SMB_SolarPanel` — `PhotovoltaicModuleSet` — geometry: yes
- `SMB_001_Roof_HVACOutdoorUnit` — `HVACEquipment` — geometry: yes

### A.8 Services — 18

- `SMB_001_Services` — `BuildingServices` — geometry: no
- `SMB_001_Plumbing` — `PlumbingSystem` — geometry: no
- `SMB_001_Plumbing_ColdWaterRiser` — `PipeRun` — geometry: yes
- `SMB_001_Plumbing_HotWaterRiser` — `PipeRun` — geometry: yes
- `SMB_001_Plumbing_SanitaryStack` — `PipeRun` — geometry: yes
- `SMB_001_Electrical` — `ElectricalSystem` — geometry: no
- `SMB_001_Electrical_MainPanel` — `ElectricalPanel` — geometry: yes
- `SMB_001_Electrical_GroundLights` — `GroundLightingCircuit` — geometry: no
- `SMB_001_Electrical_UpperLights` — `UpperLightingCircuit` — geometry: no
- `SMB_CeilingLight` — `CeilingLuminaireSet` — geometry: yes
- `SMB_001_HVAC` — `HVACSystem` — geometry: no
- `SMB_001_HVAC_LivingIndoorUnit` — `HVACEquipment` — geometry: yes
- `SMB_001_HVAC_BedroomIndoorUnit` — `HVACEquipment` — geometry: yes
- `SMB_001_HVAC_BathroomExhaust` — `HVACEquipment` — geometry: yes
- `SMB_001_FireSafety` — `FireSafetySystem` — geometry: no
- `SMB_001_FireSafety_SmokeAlarm_Ground` — `SmokeAlarm` — geometry: yes
- `SMB_001_FireSafety_SmokeAlarm_Upper` — `SmokeAlarm` — geometry: yes
- `SMB_001_FireSafety_Extinguisher` — `FireExtinguisher` — geometry: yes

### A.9 External Works — 5

- `SMB_001_ExternalWorks` — `ExternalWorksSystem` — geometry: no
- `SMB_001_External_Patio` — `Patio` — geometry: yes
- `SMB_001_External_PerimeterFence` — `Fence` — geometry: yes
- `SMB_001_External_Lights` — `ExternalLightingSystem` — geometry: no
- `SMB_ExternalLight` — `ExternalLuminaireSet` — geometry: yes

### A.10 Relationships and Semantic Evidence — 14

- `SMB_SemanticLeaf` — `SemanticEvidence` — geometry: no
- `SMB_001_Relationships` — `RelationshipSystem` — geometry: no
- `SMB_REL_GroundFloor_CONTAINS_LivingRoom` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_GroundFloor_CONTAINS_Kitchen` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_GroundFloor_CONTAINS_DiningRoom` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_UpperFloor_CONTAINS_Bedroom01` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_UpperFloor_CONTAINS_Bedroom02` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_KitchenSink_CONNECTED_TO_ColdWater` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_WaterHeater_CONNECTED_TO_HotWater` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_LivingIndoorUnit_SERVED_BY_HVACOutdoor` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_BedroomIndoorUnit_SERVED_BY_HVACOutdoor` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_SolarArray_POWERS_ElectricalSystem` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_SmokeAlarmGround_MONITORS_GroundFloor` — `SpatialRelationshipAssertion` — geometry: no
- `SMB_REL_SmokeAlarmUpper_MONITORS_UpperFloor` — `SpatialRelationshipAssertion` — geometry: no

## Appendix B. Mandatory All-Object Audit Columns

`SMB_OMv21_Object_Coverage.csv` must contain one row per object and these columns:

```text
object_id
parent_object_id
root_branch_id
concept_id
taxonomy_path
has_geometry
geometry_applicability
boundary_applicability
spatial_interface_applicability
role_assignment_count
function_allocation_count
service_port_applicability
service_port_count
requirement_count
hard_requirement_count
passed_hard_requirement_count
failed_hard_requirement_count
unknown_hard_requirement_count
operational_state_applicability
condition_state_applicability
scenario_applicability
evidence_record_count
profile_revision
profile_hash
coverage_passed
```

Release requires:

```text
row count = 124
unique object_id count = 124
coverage_passed = true for every row
failed_hard_requirement_count = 0 for every row
unknown_hard_requirement_count = 0 for every row
profile_hash != 0 for every row
```
