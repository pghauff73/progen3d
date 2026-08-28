# Improved Building Vegetation Object Model

`BVO-OMv2.1` enriches the existing building spatial object model without changing the frozen 124-object SMB inventory.

Research snapshot: August 28, 2026.

## Object Contract

Each vegetation object owns an authored extent, collision boundary, orientation frame, support or attachment interfaces, graph memberships, growth envelope, primitive/generation provenance, flattened front/right/top camera contract, and an explicit biological profile.

The biological profile separates identity, multiscale topology, phenology, root architecture, canopy optics, biomechanics, environmental response, semantic annotations, functional traits, hydraulics, size allometry, substrate requirements, and evidence provenance. Architectural archetypes do not invent species values: unresolved taxon, specimen, root, optical, seasonal, mechanical, trait, hydraulic, allometric, semantic, and substrate measurements remain null until evidence is attached.

## Taxonomy

- Tree: 10
- Shrub: 8
- Grass: 10
- Groundcover: 5
- Vine: 7
- Plant: 5
- EcologicalPlanting: 5

## Relationship Authorities

- `landscape_containment_graph`: site and planter ownership
- `landscape_support_graph`: root and base bearing relationships
- `vegetation_attachment_graph`: facade, screen, and pergola climbing hosts
- `irrigation_service_graph`: irrigation service connectivity
- `stormwater_service_graph`: rain-garden and bioswale drainage connectivity
- `vegetation_growth_graph`: generated branch, blade, leaf, petal, and vine topology
- `maintenance_access_graph`: inspection and maintenance access
- `vegetation_phenology_graph`: state and seasonal transitions
- `root_zone_constraint_graph`: below-ground occupancy and exclusion zones
- `environmental_response_graph`: light, water, temperature, wind, soil, space, and host inputs
- `vegetation_competition_graph`: crown, root, and resource competition
- `vegetation_evidence_dependency_graph`: literature, measurement, grammar, and validation provenance

## Biological Scales

The research-backed decomposition is `WholePlant -> Axis -> GrowthUnit -> Metamer -> Organ -> GeometryRegion -> RepresentationCluster`. Decomposition, succession, branching, and attachment remain distinct relationship meanings.

## Calibration Boundary

Root simulation requires measured depth, radial spread, root order, diameter, branching angle, and occupancy density. Light interception requires leaf area, leaf area density, leaf angle distribution, crown gap fraction, and projected foliage area. Physical wind simulation requires mass density, elastic modulus, damping, drag, and axis/organ articulation evidence. Water transport, mature size projection, trait inference, ontology exchange, and substrate suitability each require their own observations and cannot be inferred from an architectural label.

`VegetationBiologicalProfileValidationService` rejects contradictory identity, topology, phenology, root, optical, biomechanical, semantic, trait, hydraulic, allometric, substrate, environmental, and provenance records. `VegetationCalibrationReadinessEvaluationService` evaluates all fourteen calibration domains independently.

The current architectural-archetype suite is ready for authored building placement only. It is intentionally not marked ready for calibrated biological simulation or measured representation fidelity.

## Calibration Evidence Bundles

External calibration evidence uses `ProGen3D-VegetationCalibrationEvidenceBundle-v1`. The parser creates an immutable candidate, validation checks schema, subject identity, architecture, SI units, source hashes, canonical payload SHA-256, measurement domains, and evidence references, and binding creates a new biological profile without mutating the source object.

Binding is domain-atomic. Complete domains are accepted, incomplete but valid domains are deferred, and invalid bundles reject every measurement. The binding report records accepted, deferred, and rejected measurements plus before/after readiness domains.

## Measured Source Adapters

`VegetationMeasuredSourceArtifact` is the immutable boundary for native measured-source payloads. It owns source identity, citation, locator, media type, coordinate and unit systems, acquisition method, uncertainty, raw payload, and exact SHA-256. Adapter validation rejects stale hashes, unsupported media, malformed payloads, architecture mismatch, invalid graphs, unsupported units, and invalid observation series before evidence creation.

Five purpose-specific adapters convert QSM cylinder graphs, root architecture graphs, canopy optical observations, phenology observation series, and biomechanical material tests into the calibration evidence bundle contract. `VegetationMeasurementUnitNormalizationService` performs explicit supported conversions to SI, and `VegetationMeasuredSourceAdapterReport` preserves accepted, deferred, and rejected source records.

## Native Measured-Source Decoders

`VegetationMeasuredSourceSchemaRegistry` resolves exact source media type and schema version pairs. D2 supports the TreeQSM `save_model_text` 1.1.0 cylinder table, three-dimensional RSML v1 with metric resolution and polyline-domain diameter evidence, and three declared ProGen3D measurement tables. Unknown or ambiguous formats fail closed.

`VegetationMeasuredCoordinateReferenceTransform` owns a source-to-`LocalPlantXYZ-ZUp` affine transform, coordinate units, uncertainty, evidence identifier, and evidence SHA-256. Decoder output preserves raw payload identity and hash in canonical source provenance. Derived endpoints, averaged RSML diameters, nearest-axis attachment, deferred fields, and rejected rows remain explicit decode observations.

## Segmented Woody Branch Graphs

D3B2C1 separates explicit woody point segmentation from branch-graph reconstruction. `VegetationWoodyPointSegmentation` owns point-to-axis assignments, confidence, hierarchy, branch order, completeness for observed woody points, dataset identity, and exact source hash. Validation rejects missing or duplicate assignments, non-woody ownership, cycles, invalid order transitions, low confidence, and provenance drift.

`VegetationSegmentedWoodyBranchGraphReconstructionService` constructs purpose-specific axis, tapered-cylinder, and parent-child connection objects only from an admitted `ShootArchitecture` job and the exact `ProGen3D-SegmentedWoodyBranchGraph-v1` algorithm. Independent quality evidence records assignment coverage, principal variance, training and holdout surface residuals, holdout coverage, attachment gap, taper, branch order, and bounded component counts.

A quality-admitted graph can be exported as a deterministic canonical QSM artifact, but its scope is `CompleteForObservedWoodyEvidence`. Automated segmentation, parameter-stability evidence, unobserved biological branches, whole-tree volume, and biomass remain outside the claim.

D3B2C2 adds deterministic cover-cell partitioning, a bounded neighbour graph, a rooted minimum spanning tree, direction-continuity axis construction, refined centreline fitting, and geometry-derived assignment confidence. Multiple parameter candidates must each pass the explicit-segmentation graph gate before `VegetationWoodySegmentationSensitivityEvaluationService` compares assignment agreement, topology consensus, cylinder count, axis length, and derived woody-volume variation.

The selected candidate remains complete only for observed woody evidence. Cross-parameter agreement establishes reconstruction stability, not component-level biological correctness or calibrated whole-tree volume.

## Literature Review

See `docs/BUILDING_VEGETATION_OBJECT_MODEL_LITERATURE_REVIEW.md`. The generated model records thirty-four stable source identifiers (`BVO-LIT-001` through `BVO-LIT-034`) so every research-derived field can name its evidence domain.

## Acceptance Boundary

The grammar and deterministic camera render remain geometry authority. ImageGen references provide appearance intent constrained back toward the rendered silhouette, extents, orientation, and component placement before scoring.
