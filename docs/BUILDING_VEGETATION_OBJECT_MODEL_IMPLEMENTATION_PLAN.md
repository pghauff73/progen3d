# Building Vegetation Object Model Implementation Plan

Plan snapshot: August 28, 2026.

## Objective

Evolve the Building Vegetation Object Model from deterministic architectural
vegetation geometry into an evidence-bound, domain-calibrated object model
without changing the frozen 124-object Simple Modern Building inventory or
inventing species and specimen measurements.

## Governing Rules

- Grammar and authored extents remain current geometry authority.
- Biological domains have separate model owners and readiness gates.
- Unknown measurements remain null.
- One ready domain cannot certify another domain.
- Calibration evidence must identify its source, units, acquisition method,
  uncertainty, specimen or taxon scope, and exact content hash.
- Imported evidence may specialize an architectural archetype but may not
  silently replace its object identity, spatial interfaces, or geometry.
- Validation, binding, readiness evaluation, and simulation remain separate
  service responsibilities.

## Slice A: Research-Backed Biological Profiles

Status: completed and validated.

Implemented:

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
- `VegetationBiologicalProfile` composition in
  `BuildingVegetationObjectModel`;
- BVO-OMv2.1 records for all 50 vegetation objects;
- a seventeen-source primary literature review and stable source identifiers.

Acceptance evidence:

- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- all 50 generated objects carry every biological profile;
- uncalibrated values remain null;
- generated grammar hashes remain the geometry-provenance authority.

## Slice B: Validation and Calibration Readiness

Status: completed and validated.

Implemented:

- `VegetationBiologicalProfileValidationService`;
- `VegetationCalibrationReadinessEvaluationService`;
- fourteen independent calibration domains;
- calibrated, uncalibrated, contradictory, duplicate-domain, and fail-closed
  coverage;
- generated readiness records for all 50 objects;
- Makefile ownership and full editor linkage.

The current generated suite is ready for authored `BuildingPlacement`. It is
not yet ready for calibrated identity, shoot topology, roots, canopy optics,
phenology, biomechanics, environmental response, semantic annotation,
functional traits, hydraulics, size allometry, substrate suitability, or
measured representation fidelity.

Issue adjustment:

The complete vegetation matrix initially failed because its grammar-scene gate
still referenced showcase files at `examples/`. The canonical files had moved
to `examples/SimpleModernBuilding/`. The gate now uses the canonical paths and
the complete nonvisual vegetation matrix passes.

Acceptance evidence:

- `make -j2 progen3d-editor-gui` passes;
- `./tests/run_vegetation_grammar_scene_checks.sh` passes;
- `./tests/run_vegetation_checks.sh` passes;
- `git diff --check` passes for the changed implementation surface.

## Slice C: Evidence Bundle Import and Binding

Status: completed and validated.

Implemented an immutable calibration evidence boundary:

1. `VegetationCalibrationEvidenceBundle` owns bundle identity, schema version,
   taxon or specimen scope, coordinate and unit systems, acquisition method,
   uncertainty statement, source references, and exact payload hash.
2. `VegetationCalibrationMeasurement` owns one typed measurement, value, unit,
   domain, observation scope, and uncertainty.
3. `VegetationCalibrationEvidenceBundleParsingService` parses JSON without
   mutating canonical object state.
4. `VegetationCalibrationEvidenceBundleValidationService` rejects unknown
   domains, unsupported units, duplicate measurements, non-finite values,
   architecture mismatch, missing evidence hashes, and contradictory identity.
5. `VegetationCalibrationEvidenceBindingService` creates a new specialized
   `VegetationBiologicalProfile`; it never mutates the input profile.
6. `VegetationCalibrationEvidenceBindingReport` records accepted, deferred, and
   rejected measurements and the before/after readiness domains.
7. Deterministic tests cover exact round trips, order independence, duplicate
   rejection, partial bundles, mismatched specimen identity, stale hashes, and
   domain readiness transitions.

Supporting model corrections:

- `PlantIdentityProfile` now owns an explicit specimen identifier so measured
  specimen evidence cannot be represented by a taxon name alone;
- `PlantArchitecturalTopologyProfile` now owns maximum branch order and
  topology evidence identifiers;
- `PlantRootArchitectureProfile` now owns an explicit root-graph identifier and
  root evidence identifiers;
- the generated BVO-OMv2.1 schema exposes those fields and records the evidence
  bundle authority, services, hash policy, and immutable domain-atomic binding
  policy for every vegetation object.

Issue adjustments:

1. Exact payload hashes and exact JSON round trips required one canonical owner,
   so `VegetationCalibrationEvidenceBundleSerializationService` was added rather
   than duplicating JSON ordering rules in the parser, validator, and tests.
2. The original identity model could distinguish taxon and cultivar evidence but
   could not prove measured-specimen identity. An explicit specimen identifier is
   now required at that resolution.
3. Shoot and root readiness originally lacked the minimum graph-completeness
   fields needed by the bundle contract. Maximum branch order and root-graph
   identity are now explicit rather than inferred from geometry.
4. The first focused pass exposed generated-schema drift after those C++ model
   changes. The generator now emits the same fields and evidence authority as the
   canonical object model.
5. Final semantic review found that accepted text measurements could be copied
   into `VegetationEvidenceProfile.sourceIdentifiers`. Binding now records only
   bundle and referenced source identifiers; taxon names and artifact identifiers
   remain owned by their purpose-specific profiles.

Initial supported measurements:

- identity: scientific name, cultivar, specimen identifier;
- shoot topology: topology artifact identifier and maximum branch order;
- roots: graph identifier, depth, radial spread, maximum order;
- canopy optics: leaf area index, gap fraction, mean leaf inclination;
- phenology: seasonal schedule artifact identifier;
- biomechanics: density, elastic modulus, damping ratio, drag coefficient;
- environmental response: response-model artifact identifier;
- semantic annotation: ontology, release, anatomy and development term identifiers;
- functional traits: measured leaf traits, mature height, scope and uncertainty;
- hydraulics: conductance, capacitance, vulnerability thresholds and area ratio;
- size allometry: reference dimensions and relationship model identifier;
- substrate suitability: rootable volume, depth, density, porosity and water capacity;
- representation fidelity: grammar and camera evidence identifiers.

Acceptance evidence:

- canonical serialize-parse-serialize output is byte-identical;
- source and measurement ordering does not change the payload hash;
- duplicate, stale-hash, unknown-domain, unsupported-unit, non-finite,
  architecture-mismatch, missing-source-hash, and identity-conflict bundles are
  rejected;
- partial domains are deferred without falsely changing readiness;
- a complete valid bundle produces a new profile and transitions the calibrated
  domains without mutating the architectural archetype;
- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- `make -j2 progen3d-editor-gui` passes with the JsonCpp and SHA-256 boundary
  linked into the complete editor;
- `./tests/run_vegetation_checks.sh` passes.

## Slice D1: Canonical Measured-Source Adapters

Status: completed and validated.

Implemented evidence-producing adapters that never mutate a canonical profile:

1. `VegetationMeasuredSourceArtifact` owns immutable source identity, citation,
   locator, media type, coordinate and unit systems, acquisition metadata,
   uncertainty, raw payload, and exact payload SHA-256.
2. `VegetationQuantitativeStructureModelAdapter` validates QSM cylinder identity,
   parent connectivity, branch order, positive dimensions, architecture, units,
   and source hash before emitting shoot-topology evidence.
3. `VegetationRootArchitectureGraphAdapter` validates root segment connectivity
   and computes normalized maximum depth, radial spread, and root order relative
   to the declared root origin.
4. `VegetationCanopyObservationAdapter` converts optical observations into leaf
   area index, crown gap fraction, and mean leaf inclination measurements while
   explicitly deferring point-cloud identifiers not represented by bundle v1.
5. `VegetationPhenologyObservationSeriesAdapter` requires valid, unique,
   chronologically ordered ISO dates and at least two development stages.
6. `VegetationBiomechanicalMaterialTestAdapter` converts measured density,
   elastic modulus, damping, and drag into the existing biomechanical domain.
7. `VegetationMeasurementUnitNormalizationService` supports explicit length,
   density, pressure, fraction, angle, and dimensionless conversions.
8. `VegetationMeasuredCoordinateNormalizationService` converts declared local
   Y-up and Z-up coordinates into canonical `LocalPlantXYZ-ZUp`; coordinate
   frames requiring an undeclared world-to-local transform fail closed.
9. `VegetationMeasuredSourceAdapterReport` preserves accepted, deferred, and
   rejected source records, while `VegetationMeasuredEvidenceBundleFactory`
   creates a canonically hashed Slice C evidence bundle.

Issue adjustments:

1. QSM, root, phenology, point-cloud, and laboratory tools do not share one
   stable vendor-native file contract. Slice D was split so the validated
   canonical JSON interchange boundary is established before format-specific
   decoders are added.
2. Preserving a source coordinate-system label was insufficient after scalar
   geometry measurements had been normalized. Evidence bundles now declare the
   canonical local Z-up frame, while the immutable source artifact preserves the
   original frame.
3. A world-coordinate label cannot establish a specimen-local transform by
   itself. Global frames are rejected until an exact transform, units, axis
   convention, and provenance are supplied.
4. Point-cloud identifiers currently have no dedicated measurement kind in
   evidence bundle v1. The canopy adapter records them as deferred rather than
   silently treating them as calibrated optical values.

Acceptance evidence:

- five frozen measured-source fixtures cover QSM, roots, canopy optics,
  phenology, and biomechanics;
- repeated adaptation of the same hashed artifact produces byte-identical
  canonical evidence JSON;
- centimetres, millimetres, density, pressure, percent, and radians are converted
  to the exact SI units required by Slice C;
- local Y-up coordinates convert deterministically to local Z-up coordinates;
- stale hashes, unsupported coordinate systems and units, architecture mismatch,
  invalid parent references, incomplete graphs, and unordered observations are
  rejected;
- adapter uncertainty and source identifiers propagate to every output
  measurement;
- sequential adapter bundles transition shoot topology, roots, canopy optics,
  phenology, and biomechanics to ready without mutating the source archetype;
- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- `make -j2 progen3d-editor-gui` passes;
- `./tests/run_vegetation_checks.sh` passes.

## Slice D2: Native Decoders and Georeferencing

Status: completed and validated on August 28, 2026.

Implemented source decoders outside the validated adapter boundary:

1. `VegetationMeasuredSourceSchemaRegistry` maps an exact media type and schema
   version to one decoder; unknown or ambiguous variants fail closed.
2. QSM decoders convert declared cylinder-table and supported TreeQSM exports into
   `ProGen3D-QuantitativeStructureModel-v1` without changing source values.
3. Root decoders convert declared root-system graph exports into
   `ProGen3D-RootArchitectureGraph-v1` with explicit origin and axis semantics.
4. Canopy decoders read declared optical-result tables and point-cloud metadata;
   raw point-cloud processing remains a separate reconstruction service.
5. Phenology and biomechanical table decoders preserve row-level source
   provenance, missing fields, and declared uncertainty.
6. `VegetationMeasuredCoordinateReferenceTransform` owns the exact affine
   world-to-specimen transform, source and target frames, units, uncertainty, and
   evidence hash required before georeferenced sources can enter D1.
7. Decoder reports distinguish unsupported source format, lossy conversion,
   deferred fields, and records rejected before canonical adaptation.

Implementation details:

1. `VegetationMeasuredSourceDecodeContext` composes the exact source schema,
   target plant architecture, canonical record identifier, and optional proven
   coordinate reference transform.
2. `VegetationMeasuredSourceDecodeReport` owns fatal decode issues, raw source
   identity and hash, explicit decoded, derived, lossy, deferred, and rejected
   observations, and the optional canonical artifact.
3. `VegetationMeasuredSourceSchemaRegistry` registers five exact source
   contracts. `VegetationMeasuredSourceDecodingService` refuses unknown schema
   versions, unknown media types, and ambiguous registrations.
4. `VegetationTreeQsmCylinderTableDecoder` implements the official TreeQSM
   `save_model_text` 1.1.0 cylinder export. It preserves the raw seventeen-value
   rows, derives endpoints from start, axis, and length, and defers auxiliary
   fields not represented by canonical QSM v1.
5. `VegetationRootSystemMarkupLanguageDecoder` implements metric,
   three-dimensional RSML v1. It applies metadata resolution, traverses nested
   roots, emits one segment per polyline interval, derives child attachment to
   the nearest containing-root segment, and converts polyline diameter samples
   to segment radii.
6. Three declared ProGen3D table decoders preserve canopy, phenology, and
   biomechanical values and units for validation by the completed D1 adapters.
7. `VegetationMeasuredCoordinateReferenceTransform` and its service validate an
   invertible affine matrix, map source points into `LocalPlantXYZ-ZUp`, map
   target points back for round-trip tests, and retain transform uncertainty,
   evidence identity, and evidence SHA-256.
8. `VegetationDecodedMeasuredSourceArtifactFactory` embeds raw source media,
   schema, identity, payload hash, decoder identity, and optional transform
   evidence in deterministic canonical JSON before attaching a new exact hash.

Issues discovered and adjustments:

1. TreeQSM `save_model_text` 1.1.0 writes thirteen header labels for seventeen
   numeric cylinder values because `start_point` and `axis_direction` each
   represent three columns. MATLAB reuses the thirteen-field numeric format
   across the transposed cylinder matrix, so physical line breaks do not
   necessarily coincide with logical cylinders. The decoder validates the exact
   header and regroups the numeric token stream into seventeen-value records.
2. RSML geometry is defined in image coordinates. Metadata `resolution`
   converts pixels to the declared unit, while `z` is optional. The building
   vegetation decoder therefore accepts only metric units and explicit 3D
   points; pixel-only or 2D files are rejected until independent calibration is
   supplied.
3. RSML nesting identifies the containing root but does not identify one
   canonical parent segment. Nearest-segment attachment is therefore marked as
   derived geometry, never silently presented as a measured parent relation.
4. One diameter sample per point requires averaging adjacent values to obtain a
   segment diameter. This is marked as lossy conversion. One sample per segment
   is preserved directly.
5. Circular radii cannot survive a non-uniform affine transform without becoming
   elliptical. QSM and RSML decoders therefore require rigid or uniformly scaled
   transforms and reject non-uniform scale.
6. Vendor binary QSM files, raw point clouds, arbitrary CSV files, and
   pixel-only RSML remain outside the supported source registry.

Acceptance evidence:

- exact decode output from frozen native-format fixtures;
- source schema and version mismatch rejection;
- world-to-local transform round trips and axis-convention fixtures;
- unchanged raw-source hash and source-to-canonical provenance;
- decoder output accepted by every completed D1 adapter;
- `tests/vegetation_native_source_decoder_harness.cpp` passes under
  `-Wall -Wextra -Wpedantic -Werror`;
- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- `make -j2 progen3d-editor-gui` passes;
- `./tests/run_vegetation_checks.sh` passes.

## Slice D3A: Point-Cloud Intake and Reconstruction Admission

Status: completed and validated on August 28, 2026.

Implemented prerequisite boundary:

1. `VegetationPointCloudFormatCapabilityCatalog` declares exact point-record,
   metadata-only, and unsupported combinations without format guessing.
2. `VegetationPlyPointCloudDecoder` reads PLY 1.0 headers, decodes bounded ASCII
   scalar vertex records, preserves unknown properties as deferred evidence, and
   exposes binary little- and big-endian files as metadata-only.
3. `VegetationLasPointCloudDecoder` reads uncompressed little-endian LAS 1.0
   through 1.4 public headers and point data records. It applies authoritative
   integer scales and offsets, validates version-specific header and record
   lengths, maps source coordinates into canonical plant space, and compares
   decoded extent with transformed public-header bounds.
4. LAS 1.5 is explicitly represented but rejected because it is newer than the
   frozen D3A decoder contract. LAZ and E57 fail closed with named LASzip/PDAL
   and libE57Format dependency requirements.
5. `VegetationPointCloudDataset` composes immutable source metadata, exact source
   payload SHA-256, canonical point records, optional colour/intensity/class and
   return attributes, explicit woody/foliage/unknown organ labels, and observed
   metric bounds.
6. `VegetationPointCloudReconstructionAdmissionService` evaluates minimum point
   count, three-axis extent, exact duplicate fraction, source identity, and
   target-specific woody/foliage label requirements.
7. `VegetationPointCloudReconstructionJobFactory` creates deterministic bounded
   job intent only when the admission report names the same dataset and source
   hash. It does not create inferred cylinders, leaves, or optical parameters.

Research-driven adjustments:

1. PLY's extensible property model means unknown scalar properties are deferred,
   not treated as decoder errors or silently discarded.
2. LAS public-header counts, scales, offsets, record lengths, and bounds remain
   the source authority. The decoder does not infer them from point records.
3. ASPRS low, medium, and high vegetation classification codes describe return
   classes by height, not botanical organs. They remain `Unknown`; canopy jobs
   requiring leaf/wood separation must use independent organ evidence.
4. QSM and leaf-surface construction are reconstruction algorithms with
   sampling, occlusion, segmentation, and parameter uncertainty. Successful
   point ingestion therefore creates an admissible dataset, not a calibrated
   biological profile.
5. LAS 1.5 became the current ASPRS specification before the August 28, 2026
   snapshot. Compatibility is versioned explicitly instead of silently treating
   current LAS as LAS 1.4.

Focused evidence:

- frozen PLY 1.0 ASCII fixture with woody and foliage labels, colour,
  intensity, returns, classifications, and a deferred extension property;
- frozen text-encoded LAS 1.4 point-format 6 binary fixture with exact scales,
  offsets, counts, attributes, and declared bounds;
- successful deterministic PLY and LAS ingestion with unchanged source hashes;
- metadata-only binary PLY behavior;
- stale hashes, duplicates, point limits, truncation, LAS 1.5, LAZ, E57, and
  unknown media fail closed;
- canopy admission succeeds only with explicit woody and foliage coverage;
- shoot reconstruction intent is provenance-bound and deterministic;
- `tests/vegetation_point_cloud_ingestion_harness.cpp` passes under
  `-Wall -Wextra -Wpedantic -Werror`;
- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- `make -j2 progen3d-editor-gui` passes;
- `./tests/run_vegetation_checks.sh` passes.

## Slice D3B1: Binary PLY Point Records

Status: completed and validated on August 28, 2026.

Implemented extension:

1. `VegetationPlyPointCloudDecoder` now decodes PLY 1.0
   `binary_little_endian` and `binary_big_endian` scalar vertex records instead
   of stopping at metadata.
2. Exact readers cover signed and unsigned 8-, 16-, and 32-bit integers plus
   IEEE-754 32- and 64-bit floating-point properties in both byte orders.
3. Existing semantic mapping remains shared across ASCII and binary records, so
   coordinate canonicalization, colour, intensity, class, return, organ labels,
   duplicate policy, source hashing, deferred properties, bounds, and
   reconstruction admission do not diverge by encoding.
4. Header-declared scalar byte widths own binary record length. Payload size and
   multiplication overflow are checked before any point record is admitted.
5. Frozen equivalent little- and big-endian fixtures exercise every admitted
   scalar family and prove identical canonical positions and attributes.

Focused evidence:

- equivalent 42-byte little- and big-endian vertex records decode successfully;
- signed extension values, unsigned 32-bit values, and double precision values
  are consumed and preserved through source provenance as deferred properties;
- truncated binary records fail closed;
- both endian variants retain exact independent source hashes;
- the D3A and D3B1 point-cloud harness passes under
  `-Wall -Wextra -Wpedantic -Werror`;
- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- `make -j2 progen3d-editor-gui` passes;
- `./tests/run_vegetation_checks.sh` passes.

## Slice D3B2A: Canopy Occupancy Reconstruction Quality

Status: completed and validated on August 28, 2026.

Implemented reconstruction boundary:

1. `VegetationCanopyVoxelIndex` and `VegetationCanopyOccupancyCell` provide an
   explicit metric voxel topology with foliage, woody, and unknown point counts.
2. `VegetationCanopyOccupancyReconstructionPolicy` owns voxel edge length,
   minimum foliage support, occupied-cell limit, deterministic holdout split,
   neighborhood radius, holdout recall threshold, and organ-label threshold.
3. `VegetationCanopyOccupancyReconstructionService` requires an admitted
   `CanopyOptics` point-cloud job whose dataset identifier and source hash match
   the immutable dataset and admission report.
4. Foliage points are voxelized relative to the canonical dataset minimum.
   Woody and unknown points remain explicit cell evidence but cannot make a cell
   foliage-occupied.
5. The job seed and source point index produce a deterministic training/holdout
   split without arithmetic overflow. Cell ordering is deterministic.
6. `VegetationCanopyOccupancyQualityEvaluationService` independently evaluates
   organ-label fraction, holdout neighborhood recall, projected occupancy areas,
   and projected gap proxies on the final occupancy cells.
7. Reconstruction succeeds only after the quality report accepts geometry use.
   Failed reports remain inspectable but cannot produce a reconstruction.
8. Projected gap values are named proxies. The service does not emit leaf area
   index, optical gap fraction, leaf angle, or a canonical canopy observation,
   so D1 canopy calibration remains fail closed.

Research-driven adjustments:

1. Voxel-based TLS literature supports explicit three-dimensional occupancy but
   shows that voxel size, occlusion, beam sampling, and attenuation correction
   affect inferred leaf-area and gap metrics.
2. D3B2A therefore reconstructs bounded geometry occupancy only. It does not
   apply a point-count-to-leaf-area conversion or reinterpret an occupancy gap
   proxy as an optical measurement.
3. Holdout neighborhood recall is a ProGen3D reconstruction consistency metric,
   not a biological trait. It tests whether withheld foliage samples are near
   occupied training cells under the declared voxel scale.
4. Maximum occupied cells are bounded by both the reconstruction policy and the
   point-cloud job's maximum output primitive count.

Focused evidence:

- the frozen mixed woody/foliage PLY fixture reconstructs seven deterministic
  occupied cells at one-metre resolution;
- the seeded split produces five training cells and two withheld foliage points;
- holdout neighborhood recall is one and organ-label fraction is one;
- XY projected occupancy is six square metres with an explicit one-third gap
  proxy;
- repeated reconstruction produces identical ordered cells and counts;
- low holdout recall, occupied-cell overflow, and unsupported algorithms fail
  closed;
- source payload SHA-256 propagates unchanged into quality and reconstruction
  evidence;
- `tests/vegetation_point_cloud_ingestion_harness.cpp` passes under
  `-Wall -Wextra -Wpedantic -Werror`;
- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- `make -j2 progen3d-editor-gui` passes;
- `./tests/run_vegetation_checks.sh` passes.

## Slice D3B2B: Primary Woody Axis Reconstruction Quality

Status: completed and validated on Friday, August 28, 2026.

Implemented reconstruction boundary:

1. `VegetationWoodyAxisReconstructionPolicy` owns axial station spacing,
   minimum woody support, training and station support, cylinder limits,
   deterministic holdout selection, bounded radial trimming, label coverage,
   principal variance, surface residual, axial coverage, taper, and minimum
   observed-radius thresholds.
2. `VegetationWoodyAxisRadiusStation` represents one measured cross-section
   estimate along the dominant woody axis. It retains axial projection, metric
   centre, radius, ordering, and supporting point count.
3. `VegetationWoodyAxisCylinder` connects two consecutive radius stations for
   primary-axis geometry. It contains no parent branch, branch order, or
   complete-tree volume claim.
4. `VegetationPrimaryWoodyAxisReconstructionService` accepts only a
   provenance-bound admitted `ShootArchitecture` job whose exact algorithm is
   `ProGen3D-PrimaryWoodyAxis-v1`.
5. The job seed and source point index produce a deterministic woody
   training/holdout split. An initial Eigen3 covariance solution establishes a
   candidate axis; a bounded median-absolute-deviation radial trim records and
   limits obvious contamination before the final covariance solution.
6. Training points are assigned to fixed-spacing axial stations. Every station
   must retain the declared point support, preserve a measured radius above the
   policy floor, and participate in a contiguous cylinder chain.
7. `VegetationWoodyAxisQualityEvaluationService` independently evaluates woody
   label fraction, excluded-outlier fraction, principal variance fraction,
   training and holdout cylinder-surface RMSE, holdout axial coverage, and
   taper-violation fraction.
8. A failed quality report remains inspectable but cannot emit geometry.
   Output primitive count is bounded by both policy and reconstruction job.
9. `VegetationWoodyAxisReconstructionScope::PrimaryAxisOnly` and
   `representsCompleteBranchTopology() == false` keep the artifact outside the
   canonical quantitative-structure-model adapter.

Research-driven adjustments:

1. TreeQSM and SimpleTree model woody trees as ordered cylinder collections,
   but their full methods also require segmentation, branching topology,
   parameter selection, and radius correction.
2. Robust-cylinder research shows ordinary PCA can be badly biased by partial
   cylinders and outliers. D3B2B therefore records a bounded deterministic
   radial trim, while explicitly avoiding any claim that this is equivalent to
   robust PCA or a complete local-cylinder estimator.
3. Ground-truth QSM studies show that volume and small-branch accuracy require
   separate validation. D3B2B therefore admits primary-axis geometry only and
   emits no branch graph, whole-tree volume, biomass, or D1 QSM evidence.
4. Training and holdout surface residuals are measured against linearly
   interpolated station radii, so taper is evaluated without silently forcing a
   constant-radius trunk.

Focused evidence:

- a frozen 40-point PLY fixture describes five symmetric woody rings over a
  three-metre axis with radii tapering from 0.20 to 0.12 metres;
- the deterministic split produces 30 training and 10 holdout woody points;
- reconstruction emits five ordered radius stations and four cylinders;
- the recovered axis is positive Z, all station radii match the fixture within
  one micrometre, and no training points are excluded;
- principal variance exceeds 0.97, both surface RMSE values remain below one
  micrometre, axial coverage is one, and taper violation is zero;
- repeated reconstruction preserves exact station identifiers and radii;
- strict variance, cylinder limits, insufficient woody support, unsupported
  algorithms, wrong targets, and source-hash mismatches fail closed;
- `tests/vegetation_point_cloud_ingestion_harness.cpp` passes under
  `-Wall -Wextra -Wpedantic -Werror` with Eigen3;
- `./tests/run_building_vegetation_triangle_distribution_checks.sh` passes;
- `make -j2 progen3d-editor-gui` passes;
- `./tests/run_vegetation_checks.sh` passes.

## Slice D3B2C1: Segmentation Contract and Quality-Admitted Branch Graph

Status: implemented and validated on August 28, 2026.

Implemented object model:

1. `VegetationWoodyAxisSegmentDefinition` declares one root or lateral axis,
   its parent axis, branch order, and reconstruction station spacing.
2. `VegetationWoodyPointSegmentAssignment` binds one immutable source-point
   index to one axis with explicit assignment confidence.
3. `VegetationWoodyPointSegmentation` owns exact dataset and source hashes,
   segmentation and algorithm identifiers, root-axis identity, completeness for
   observed woody points, axis definitions, and point assignments.
4. `VegetationWoodyPointSegmentationValidationService` rejects unsupported
   schemas, identity or hash drift, duplicate assignments, non-woody points,
   missing axes, invalid parent/order relations, cycles, incomplete coverage,
   and insufficient confidence before graph construction.
5. `VegetationWoodyBranchAxis`, `VegetationWoodyBranchCylinder`, and
   `VegetationWoodyBranchConnection` separate branch hierarchy, tapered
   geometry, and parent-child attachment relationships.
6. `VegetationSegmentedWoodyBranchGraphReconstructionService` accepts only an
   admitted `ShootArchitecture` job using
   `ProGen3D-SegmentedWoodyBranchGraph-v1`. Per-axis Eigen3 covariance, fixed
   axial stations, deterministic holdout selection, tapered radius estimates,
   parent-cylinder chains, and explicit attachment points remain inspectable.
7. `VegetationWoodyBranchGraphQualityEvaluationService` evaluates assignment
   coverage and confidence, per-axis principal variance, training and holdout
   surface residuals, holdout surface coverage, attachment surface gap, taper,
   branch order, and bounded axis/cylinder/connection counts.
8. `VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory`
   exports a deterministic provenance-bound QSM JSON artifact only after the
   graph quality report passes. The existing QSM adapter then validates the
   rooted cylinder graph and branch-order relationships.
9. `VegetationWoodyBranchGraphReconstructionScope::CompleteForObservedWoodyEvidence`
   means all explicitly segmented observed woody points are represented. It
   does not mean the scan observed every biological branch, and
   `representsCompleteBiologicalTree() == false` preserves that boundary.

Research-driven adjustments:

1. TreeQSM cover-set and cylinder construction, leaf/wood classification, and
   branch segmentation are separate inference responsibilities. The current
   point records contain organ labels but no general axis labels, so D3B2C1
   consumes an explicit segmentation contract instead of silently inventing
   branch identities.
2. Harvested-branch comparison demonstrates that branch architecture needs
   component-level ground truth, not silhouette or whole-volume agreement
   alone. D3B2C1 therefore preserves axis, cylinder, connection, source-point,
   and holdout evidence as separate inspectable records.
3. TreeQSM sensitivity experiments show that cover-patch and relative-cylinder
   parameters can materially change branch estimates. A single accepted graph
   therefore cannot establish parameter stability or whole-tree volume.
4. The frozen fixture maps three project-specific PLY classification values to
   three explicit axes only for deterministic contract validation. It is not a
   general LAS botanical segmentation rule.

Focused evidence:

- a frozen 96-point PLY fixture describes a tapered root axis, first-order
  lateral axis, and second-order lateral axis;
- complete explicit segmentation reconstructs three axes, nine cylinders, two
  parent-child connections, and maximum branch order two;
- recovered axis directions match positive Z, positive X, and positive Y;
- assignment coverage and holdout surface coverage equal one, surface RMSE is
  below one micrometre, taper violations are zero, and maximum attachment gap
  remains below 0.081 metres;
- repeated reconstruction and QSM export preserve exact identifiers, radii,
  canonical payload, and SHA-256;
- incomplete segmentation, missing assignments, low confidence, excessive
  attachment gap, cylinder limits, and unsupported algorithms fail closed;
- the generated QSM artifact retains source point-cloud and segmentation
  provenance and passes the canonical QSM adapter for measured-specimen scope.

## Slice D3B2C2: Automated Segmentation and Parameter-Sensitivity Ensembles

Status: implemented and validated on August 28, 2026.

Implemented object model:

1. `VegetationWoodySegmentationParameterSet` owns one bounded cover-cell,
   neighbour, continuation, assignment-confidence, axis-count, and axial-station
   configuration.
2. `VegetationWoodyCoverSet` preserves deterministic grid identity, metric
   centroid, and source-point membership. `VegetationWoodyCoverConnection`
   records one oriented spanning-tree relationship and centre distance.
3. `VegetationWoodySegmentationAxisCandidate` owns one inferred axis hierarchy,
   branch order, cover ownership, refined centreline, and station spacing.
4. `VegetationWoodySegmentationCandidate` aggregates the parameter set, cover
   partition, rooted cover graph, axis candidates, and complete
   `VegetationWoodyPointSegmentation` contract.
5. `VegetationWoodySegmentationCandidateEvaluation` binds the candidate report
   to its D3B2C1 branch-graph report plus derived total cylinder length and
   observed woody volume for sensitivity analysis.
6. `VegetationWoodySegmentationSensitivityPolicy` and
   `VegetationWoodySegmentationSensitivityReport` separate selection thresholds
   from measured candidate evidence.
7. `VegetationAutomatedWoodySegmentationEnsembleReport` preserves every
   candidate evaluation, sensitivity decision, and the selected segmentation
   and graph only when the full ensemble gate passes.

Implemented services:

1. `VegetationWoodyCoverSetSegmentationService` partitions canonical woody
   points into deterministic cover cells, constructs a bounded neighbour graph,
   selects a rooted minimum spanning tree, and separates axes through branch
   components and direction-continuity breaks.
2. A provisional nearest-centreline assignment is refined by per-axis Eigen3
   covariance. Child centrelines are extended to their closest parent-line
   attachment before final geometric assignment confidence is calculated.
3. The service never reads vendor or fixture classification values. Branch
   identity depends only on canonical metric positions and the explicit Woody
   organ class.
4. `VegetationAutomatedWoodySegmentationEnsembleService` reconstructs every
   successful candidate through the exact D3B2C1 graph service using a
   deterministic parameter-specific seed.
5. `VegetationWoodySegmentationSensitivityEvaluationService` selects the
   candidate with strongest assignment consensus, then lower holdout residual,
   then stable parameter identity. Admission checks accepted-candidate fraction,
   selected assignment agreement, axis-count and branch-order consensus, and
   coefficients of variation for cylinder count, total length, and derived
   observed-woody volume.

Research-driven adjustments:

1. Official TreeQSM code treats cover creation, neighbour relationships,
   branch segmentation, cylinder construction, and optimum multi-run selection
   as distinct stages. D3B2C2 retains the same responsibility boundaries rather
   than embedding them in one procedural reconstruction blob.
2. The frozen junction rings expose unavoidable assignment competition where
   two cylindrical surfaces meet. The automated graph policy therefore permits
   two training points at a junction station only inside the later sensitivity
   gate; the explicit D3B2C1 fixture retains its stronger five-point boundary.
3. Short terminal axes require a 0.35-metre station-spacing floor in the frozen
   fixture. Smaller inferred spacing over-segmented three observed rings and
   correctly failed station support.
4. Sensitivity agreement establishes deterministic reconstruction stability,
   not biological correctness. Derived candidate volume remains a comparison
   metric and cannot enter calibrated evidence.

Focused evidence:

- three cover-cell parameter candidates are evaluated without consulting PLY
  classification values;
- the selected candidate assigns all 96 woody points, reconstructs three axes,
  two parent-child connections, and maximum branch order two;
- selected assignment agreement is at least 0.88 and accepted graph candidates
  have complete axis-count and branch-order consensus;
- repeated ensembles preserve the selected parameter, every point assignment,
  assignment confidence, graph geometry, canonical QSM payload, and SHA-256;
- the selected graph remains `CompleteForObservedWoodyEvidence`, passes the QSM
  adapter for measured-specimen scope, and explicitly rejects complete-tree
  authority;
- disconnected cover graphs, ambiguous assignments, unstable parameter
  ensembles, and unsupported algorithms fail closed;
- focused compilation and runtime checks pass under
  `-Wall -Wextra -Wpedantic -Werror`.

## Slice D3B2C3: Component-Level Ground Truth and Volume Admission

Status: next.

- define measured branch-component observations with exact specimen, source,
  coordinate, unit, uncertainty, and payload-hash provenance;
- associate measured and reconstructed branches without assuming identifier or
  traversal-order equality;
- evaluate branch detection precision and recall by branch order and diameter
  class;
- evaluate per-axis attachment, direction, length, diameter, taper, and volume
  residuals against harvested-branch or equivalent component-level evidence;
- preserve unmatched measured and reconstructed components as explicit false
  negatives and false positives;
- admit observed woody volume only when component coverage, residual, bias,
  sensitivity, and uncertainty policies pass;
- retain complete biological-tree and biomass claims as separate later gates.

## Slice D3C: Additional Native Formats and Calibration Methods

Status: planned after D3B2C3.

- measured TreeQSM MATLAB `.mat` intake and other declared QSM export versions;
- LAS 1.5, LAZ, and E57 integration through audited dependencies;
- canopy reconstruction with independently evaluated leaf/wood classification,
  leaf area, gap fraction, and projected-area error;
- pixel-only or two-dimensional RSML with an independently calibrated image
  transform and explicit depth policy;
- richer laboratory and phenology exchange profiles with protocol metadata.

## Slice E: Runtime Consumers

Status: planned after reconstruction and additional native formats.

- calibrated light-interception service;
- calibrated wind-material assignment;
- root-zone spatial constraints and collision queries;
- phenology-driven representation state selection;
- environment-response and competition evaluation;
- renderer evidence reporting of selected calibrated versus authored fallback
  domains.

## Current Boundary

BVO-OMv2.1 now owns the concepts and gates needed to distinguish architectural
geometry from calibrated biology. It can parse, validate, hash, serialize, and
immutably bind typed external calibration evidence bundles. It accepts five
hashed canonical measured-source JSON schemas and D2 decodes the exact TreeQSM
text cylinder export, metric 3D RSML v1, and three declared measurement tables.
D3A additionally ingests bounded PLY 1.0 ASCII and uncompressed LAS 1.0-1.4
point records, preserves exact source metadata and hashes, and admits datasets
to provenance-bound deterministic reconstruction jobs. D3B1 extends the same
contract to PLY 1.0 little- and big-endian scalar vertex records. D3B2A adds
quality-gated foliage occupancy for geometry use while keeping canopy optical
calibration closed. D3B2B adds a quality-gated primary woody-axis station and
cylinder chain while keeping complete QSM topology, whole-tree volume, and D1
calibration closed. D3B2C1 now validates an explicit woody point segmentation,
constructs a parent-child axis/cylinder/connection graph, evaluates deterministic
holdout and junction quality, and emits a provenance-bound QSM artifact whose
scope is complete only for observed woody evidence. D3B2C2 now constructs
classification-independent cover candidates, refines axis centrelines, evaluates
each candidate through the D3B2C1 graph authority, and selects a graph only after
assignment, topology, cylinder-count, axis-length, and observed-volume
sensitivity pass. It does not yet establish component-level branch-detection
accuracy, admit calibrated observed woody volume, claim a complete biological
tree, calculate biomass, reconstruct leaf surfaces, read TreeQSM MATLAB files,
decode LAS 1.5, LAZ, or E57, accept pixel-only RSML, or infer arbitrary vendor
tables. The next implementation slice is D3B2C3 component-level ground truth and
volume admission—not direct profile mutation and not coupled biological
simulation.
