#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_directory="${TMPDIR:-/tmp}/progen3d-building-vegetation-triangle-tests"
compiler="${CXX:-g++}"
jsoncpp_cflags="$(pkg-config --cflags jsoncpp 2>/dev/null || true)"
jsoncpp_libs="$(pkg-config --libs jsoncpp 2>/dev/null || printf '%s' '-ljsoncpp')"
libxml2_cflags="$(pkg-config --cflags libxml-2.0 2>/dev/null || true)"
libxml2_libs="$(pkg-config --libs libxml-2.0 2>/dev/null || printf '%s' '-lxml2')"
eigen3_cflags="$(pkg-config --cflags eigen3 2>/dev/null || true)"

rm -rf "$build_directory"
mkdir -p "$build_directory"

"$compiler" \
	-std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Werror \
	-I"$repository_root/include" \
	-I"$repository_root" \
	"$repository_root/src/vegetation/service/VegetationTriangleDistributionService.cpp" \
	"$repository_root/src/vegetation/service/VegetationRepresentationFidelityEvaluationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationBiologicalProfileValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationReadinessEvaluationService.cpp" \
	"$repository_root/tests/building_vegetation_triangle_distribution_harness.cpp" \
	-o "$build_directory/building-vegetation-triangle-distribution-harness"

"$build_directory/building-vegetation-triangle-distribution-harness"

# Intentional word splitting applies pkg-config compiler and linker flags.
# shellcheck disable=SC2086
"$compiler" \
	-std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Werror \
	-I"$repository_root/include" \
	-I"$repository_root" \
	$jsoncpp_cflags \
	"$repository_root/src/vegetation/model/VegetationCalibrationMeasurement.cpp" \
	"$repository_root/src/vegetation/model/VegetationCalibrationEvidenceBundle.cpp" \
	"$repository_root/src/vegetation/service/VegetationBiologicalProfileValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationReadinessEvaluationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationPayloadHashService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleParsingService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBindingService.cpp" \
	"$repository_root/tests/vegetation_calibration_evidence_bundle_harness.cpp" \
	$jsoncpp_libs -lcrypto \
	-o "$build_directory/vegetation-calibration-evidence-bundle-harness"

"$build_directory/vegetation-calibration-evidence-bundle-harness"

# Intentional word splitting applies pkg-config compiler and linker flags.
# shellcheck disable=SC2086
"$compiler" \
	-std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Werror \
	-I"$repository_root/include" \
	-I"$repository_root" \
	$jsoncpp_cflags \
	"$repository_root/src/vegetation/model/VegetationCalibrationMeasurement.cpp" \
	"$repository_root/src/vegetation/model/VegetationCalibrationEvidenceBundle.cpp" \
	"$repository_root/src/vegetation/model/VegetationMeasuredSourceArtifact.cpp" \
	"$repository_root/src/vegetation/service/VegetationBiologicalProfileValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationReadinessEvaluationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationPayloadHashService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleParsingService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBindingService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceArtifactHashService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceArtifactValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCoordinateNormalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasurementUnitNormalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredEvidenceBundleFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceJsonDocumentReader.cpp" \
	"$repository_root/src/vegetation/service/VegetationQuantitativeStructureModelAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationRootArchitectureGraphAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationCanopyObservationAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationPhenologyObservationSeriesAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationBiomechanicalMaterialTestAdapter.cpp" \
	"$repository_root/tests/vegetation_measured_source_adapter_harness.cpp" \
	$jsoncpp_libs -lcrypto \
	-o "$build_directory/vegetation-measured-source-adapter-harness"

"$build_directory/vegetation-measured-source-adapter-harness" \
	"$repository_root/tests/fixtures/vegetation_measured_sources"

# Intentional word splitting applies pkg-config compiler and linker flags.
# shellcheck disable=SC2086
"$compiler" \
	-std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Werror \
	-I"$repository_root/include" \
	-I"$repository_root" \
	$jsoncpp_cflags \
	$libxml2_cflags \
	"$repository_root/src/vegetation/model/VegetationCalibrationMeasurement.cpp" \
	"$repository_root/src/vegetation/model/VegetationCalibrationEvidenceBundle.cpp" \
	"$repository_root/src/vegetation/model/VegetationMeasuredSourceArtifact.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationPayloadHashService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceArtifactHashService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceArtifactValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCoordinateNormalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasurementUnitNormalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredEvidenceBundleFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceJsonDocumentReader.cpp" \
	"$repository_root/src/vegetation/service/VegetationQuantitativeStructureModelAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationRootArchitectureGraphAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationCanopyObservationAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationPhenologyObservationSeriesAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationBiomechanicalMaterialTestAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceSchemaRegistry.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCoordinateReferenceTransformService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredPointCanonicalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceDecoderArtifactValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationDecodedMeasuredSourceArtifactFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationDelimitedSourceTableReader.cpp" \
	"$repository_root/src/vegetation/service/VegetationTreeQsmCylinderTableDecoder.cpp" \
	"$repository_root/src/vegetation/service/VegetationRootSystemMarkupLanguageDecoder.cpp" \
	"$repository_root/src/vegetation/service/VegetationCanopyObservationTableDecoder.cpp" \
	"$repository_root/src/vegetation/service/VegetationPhenologyObservationTableDecoder.cpp" \
	"$repository_root/src/vegetation/service/VegetationBiomechanicalMaterialTestTableDecoder.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceDecodingService.cpp" \
	"$repository_root/tests/vegetation_native_source_decoder_harness.cpp" \
	$jsoncpp_libs $libxml2_libs -lcrypto \
	-o "$build_directory/vegetation-native-source-decoder-harness"

"$build_directory/vegetation-native-source-decoder-harness" \
	"$repository_root/tests/fixtures/vegetation_native_sources"

# Intentional word splitting applies pkg-config compiler and linker flags.
# shellcheck disable=SC2086
"$compiler" \
	-std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Werror \
	-I"$repository_root/include" \
	-I"$repository_root" \
	$jsoncpp_cflags \
	$eigen3_cflags \
	"$repository_root/src/vegetation/model/VegetationCalibrationMeasurement.cpp" \
	"$repository_root/src/vegetation/model/VegetationCalibrationEvidenceBundle.cpp" \
	"$repository_root/src/vegetation/model/VegetationMeasuredSourceArtifact.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCalibrationPayloadHashService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceArtifactHashService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceArtifactValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasurementUnitNormalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCoordinateNormalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCoordinateReferenceTransformService.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredPointCanonicalizationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationPointCloudFormatCapabilityCatalog.cpp" \
	"$repository_root/src/vegetation/service/VegetationPointCloudSourceArtifactValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationPlyPointCloudDecoder.cpp" \
	"$repository_root/src/vegetation/service/VegetationLasPointCloudDecoder.cpp" \
	"$repository_root/src/vegetation/service/VegetationPointCloudIngestionService.cpp" \
	"$repository_root/src/vegetation/service/VegetationPointCloudReconstructionAdmissionService.cpp" \
	"$repository_root/src/vegetation/service/VegetationPointCloudReconstructionJobFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationCanopyOccupancyQualityEvaluationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationCanopyOccupancyReconstructionService.cpp" \
	"$repository_root/src/vegetation/service/VegetationWoodyAxisQualityEvaluationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationPrimaryWoodyAxisReconstructionService.cpp" \
	"$repository_root/src/vegetation/service/VegetationWoodyPointSegmentationValidationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationWoodyBranchGraphQualityEvaluationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationSegmentedWoodyBranchGraphReconstructionService.cpp" \
	"$repository_root/src/vegetation/service/VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationWoodyCoverSetSegmentationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationWoodySegmentationSensitivityEvaluationService.cpp" \
	"$repository_root/src/vegetation/service/VegetationAutomatedWoodySegmentationEnsembleService.cpp" \
	"$repository_root/src/vegetation/service/VegetationQuantitativeStructureModelAdapter.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredEvidenceBundleFactory.cpp" \
	"$repository_root/src/vegetation/service/VegetationMeasuredSourceJsonDocumentReader.cpp" \
	"$repository_root/tests/vegetation_point_cloud_ingestion_harness.cpp" \
	$jsoncpp_libs -lcrypto \
	-o "$build_directory/vegetation-point-cloud-ingestion-harness"

"$build_directory/vegetation-point-cloud-ingestion-harness" \
	"$repository_root/tests/fixtures/vegetation_native_sources"

python3 \
	"$repository_root/examples/SimpleModernBuilding/VegetationBuildingObjects/generate_vegetation_building_objects.py" \
	--check

python3 - "$repository_root" <<'PY'
import json
import pathlib
import sys

repository_root = pathlib.Path(sys.argv[1])
model_path = (
    repository_root
    / "examples/SimpleModernBuilding/VegetationBuildingObjects/building_vegetation_object_model.json"
)
model = json.loads(model_path.read_text())

assert model["schema"] == "ProGen3D-BuildingVegetationObjectModel-v2.1"
assert model["model_id"] == "BVO-OMv2.1"
assert model["research_snapshot_date"] == "2026-08-28"
assert model["object_count"] == 50
assert len(model["literature_sources"]) == 34
assert len({source["source_id"] for source in model["literature_sources"]}) == 34
assert model["calibration_readiness_authority"]["current_suite_ready_domains"] == [
    "BuildingPlacement"
]
assert model["calibration_evidence_bundle_authority"]["schema"] == (
    "ProGen3D-VegetationCalibrationEvidenceBundle-v1"
)
assert model["calibration_evidence_bundle_authority"]["binding_service"] == (
    "VegetationCalibrationEvidenceBindingService"
)
assert model["measured_source_adapter_authority"]["schema"] == (
    "ProGen3D-VegetationMeasuredSourceArtifact-v1"
)
assert model["measured_source_adapter_authority"]["unit_normalization_service"] == (
    "VegetationMeasurementUnitNormalizationService"
)
assert set(model["measured_source_adapter_authority"]["adapters"]) == {
    "VegetationQuantitativeStructureModelAdapter",
    "VegetationRootArchitectureGraphAdapter",
    "VegetationCanopyObservationAdapter",
    "VegetationPhenologyObservationSeriesAdapter",
    "VegetationBiomechanicalMaterialTestAdapter",
}
assert model["native_measured_source_decoder_authority"]["schema_registry"] == (
    "VegetationMeasuredSourceSchemaRegistry"
)
assert set(model["native_measured_source_decoder_authority"]["decoders"]) == {
    "VegetationTreeQsmCylinderTableDecoder",
    "VegetationRootSystemMarkupLanguageDecoder",
    "VegetationCanopyObservationTableDecoder",
    "VegetationPhenologyObservationTableDecoder",
    "VegetationBiomechanicalMaterialTestTableDecoder",
}
assert model["point_cloud_reconstruction_authority"]["capability_catalog"] == (
    "VegetationPointCloudFormatCapabilityCatalog"
)
assert model["point_cloud_reconstruction_authority"]["ingestion_service"] == (
    "VegetationPointCloudIngestionService"
)
assert set(
    model["point_cloud_reconstruction_authority"]["point_record_decoders"]
) == {
    "VegetationPlyPointCloudDecoder",
    "VegetationLasPointCloudDecoder",
}
ply_support = next(
    item
    for item in model["point_cloud_reconstruction_authority"][
        "point_record_support"
    ]
    if item["media_type"] == "application/ply"
)
assert set(ply_support["encodings"]) == {
    "Ascii",
    "BinaryLittleEndian",
    "BinaryBigEndian",
}
assert set(ply_support["scalar_types"]) == {
    "int8",
    "uint8",
    "int16",
    "uint16",
    "int32",
    "uint32",
    "float32",
    "float64",
}
assert model["point_cloud_reconstruction_authority"][
    "metadata_only_support"
] == []
assert model["point_cloud_reconstruction_authority"][
    "canopy_occupancy_reconstruction"
]["algorithm_identifier"] == "ProGen3D-CanopyVoxelOccupancy-v1"
assert model["point_cloud_reconstruction_authority"][
    "canopy_occupancy_reconstruction"
]["biological_calibration_admitted"] is False
assert model["point_cloud_reconstruction_authority"][
    "primary_woody_axis_reconstruction"
]["algorithm_identifier"] == "ProGen3D-PrimaryWoodyAxis-v1"
assert model["point_cloud_reconstruction_authority"][
    "primary_woody_axis_reconstruction"
]["scope"] == "PrimaryAxisOnly"
assert model["point_cloud_reconstruction_authority"][
    "primary_woody_axis_reconstruction"
]["complete_qsm_admitted"] is False
segmented_graph = model["point_cloud_reconstruction_authority"][
    "segmented_woody_branch_graph_reconstruction"
]
assert segmented_graph["segmentation_schema"] == (
    "ProGen3D-VegetationWoodyPointSegmentation-v1"
)
assert segmented_graph["reconstruction_schema"] == (
    "ProGen3D-VegetationWoodyBranchGraphReconstruction-v1"
)
assert segmented_graph["algorithm_identifier"] == (
    "ProGen3D-SegmentedWoodyBranchGraph-v1"
)
assert segmented_graph["scope"] == "CompleteForObservedWoodyEvidence"
assert segmented_graph["canonical_qsm_artifact_admitted"] is True
assert segmented_graph["complete_biological_tree_admitted"] is False
assert segmented_graph["segmentation_validation_service"] == (
    "VegetationWoodyPointSegmentationValidationService"
)
assert segmented_graph["reconstruction_service"] == (
    "VegetationSegmentedWoodyBranchGraphReconstructionService"
)
assert segmented_graph["qsm_artifact_factory"] == (
    "VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory"
)
automated_segmentation = model["point_cloud_reconstruction_authority"][
    "automated_woody_segmentation_ensemble"
]
assert automated_segmentation["candidate_algorithm_identifier"] == (
    "ProGen3D-WoodyCoverSetSegmentation-v1"
)
assert automated_segmentation["ensemble_algorithm_identifier"] == (
    "ProGen3D-AutomatedWoodySegmentationEnsemble-v1"
)
assert automated_segmentation["candidate_service"] == (
    "VegetationWoodyCoverSetSegmentationService"
)
assert automated_segmentation["sensitivity_service"] == (
    "VegetationWoodySegmentationSensitivityEvaluationService"
)
assert automated_segmentation["ensemble_service"] == (
    "VegetationAutomatedWoodySegmentationEnsembleService"
)
assert automated_segmentation["complete_biological_tree_admitted"] is False
assert automated_segmentation["volume_calibration_admitted"] is False
assert {
    item["media_type"]
    for item in model["point_cloud_reconstruction_authority"][
        "fail_closed_formats"
    ]
} == {
    "application/vnd.las",
    "application/vnd.laz",
    "model/e57",
}
assert set(model["calibration_readiness_authority"]["domains"]) == {
    "TaxonomicIdentity",
    "ShootTopology",
    "RootArchitecture",
    "CanopyOptics",
    "Phenology",
    "Biomechanics",
    "EnvironmentalResponse",
    "SemanticAnnotation",
    "FunctionalTraits",
    "Hydraulics",
    "SizeAllometry",
    "SubstrateSuitability",
    "BuildingPlacement",
    "RepresentationFidelity",
}

required_scales = {
    "WholePlant",
    "Axis",
    "GrowthUnit",
    "Metamer",
    "Organ",
    "GeometryRegion",
    "RepresentationCluster",
}
required_relationships = {
    "Decomposition",
    "Succession",
    "Branching",
    "Attachment",
}
required_graphs = {
    "vegetation_phenology_graph",
    "root_zone_constraint_graph",
    "environmental_response_graph",
    "vegetation_competition_graph",
    "vegetation_evidence_dependency_graph",
}

for vegetation_object in model["objects"]:
    profile = vegetation_object["biological_profile"]
    assert profile["schema"] == "ProGen3D-VegetationBiologicalProfile-v2"
    assert profile["identity"]["resolution"] == "ArchitecturalArchetype"
    assert profile["identity"]["botanical_taxon"] is None
    assert profile["identity"]["cultivar"] is None
    assert profile["identity"]["specimen_identifier"] is None
    assert set(profile["architectural_topology"]["scale_hierarchy"]) == required_scales
    assert set(profile["architectural_topology"]["relationship_kinds"]) == required_relationships
    assert profile["architectural_topology"]["maximum_branch_order"] is None
    assert profile["architectural_topology"]["topology_evidence_identifiers"] == []
    assert profile["root_architecture"]["maximum_depth_metres"] is None
    assert profile["root_architecture"]["root_graph_identifier"] is None
    assert profile["canopy_optics"]["leaf_area_index"] is None
    assert profile["biomechanics"]["elastic_modulus_pascals"] is None
    assert profile["semantic_annotations"]["ontology_identifier"] is None
    assert profile["functional_traits"]["specific_leaf_area_square_metres_per_kilogram"] is None
    assert profile["hydraulics"]["maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal"] is None
    assert profile["size_allometry"]["size_relationship_model_identifier"] is None
    assert profile["substrate_requirements"]["minimum_rootable_volume_cubic_metres"] is None
    assert profile["evidence_provenance"]["geometry_sha256"] == vegetation_object["grammar_sha256"]
    assert len(profile["evidence_provenance"]["literature_source_ids"]) == 34
    assert required_graphs <= set(vegetation_object["connection_graph_memberships"])
    readiness = vegetation_object["calibration_readiness"]
    assert readiness["profile_validation_expected"] is True
    assert readiness["all_biological_domains_ready"] is False
    assert readiness["domains"]["BuildingPlacement"]["ready"] is True
    assert {
        domain
        for domain, observation in readiness["domains"].items()
        if observation["ready"]
    } == {"BuildingPlacement"}

print(
    "Building vegetation biological model coverage passed: "
    f"objects={model['object_count']} sources={len(model['literature_sources'])}"
)
PY

if rg -n 'random_device|mt19937|\brand\s*\(' \
	"$repository_root/src/vegetation/service/VegetationTriangleDistributionService.cpp"; then
	echo "FAIL: vegetation triangle allocation must remain deterministic" >&2
	exit 1
fi

for semantic_role in RootFlare PrimaryStem BranchJunction BranchSegment FoliageSilhouette FoliageInterior FlowerOrFruit GroundContact SupportStructure; do
	if ! rg -q "$semantic_role" \
		"$repository_root/include/vegetation/model/VegetationTriangleRegionRole.h"; then
		echo "FAIL: missing vegetation triangle semantic role $semantic_role" >&2
		exit 1
	fi
done

for model_class in PlantIdentityProfile PlantArchitecturalTopologyProfile PlantPhenologyProfile PlantRootArchitectureProfile PlantCanopyOpticalProfile PlantBiomechanicalProfile PlantEnvironmentalResponseProfile PlantSemanticAnnotationProfile PlantFunctionalTraitProfile PlantHydraulicProfile PlantSizeAllometryProfile VegetationSubstrateRequirementProfile VegetationEvidenceProfile VegetationBiologicalProfile VegetationBiologicalProfileValidationReport VegetationCalibrationReadinessReport VegetationCalibrationMeasurement VegetationCalibrationEvidenceBundle VegetationCalibrationEvidenceBundleParsingResult VegetationCalibrationEvidenceBundleValidationReport VegetationCalibrationEvidenceBindingReport VegetationMeasuredSourceArtifact VegetationMeasuredSourceAdapterReport VegetationMeasurementUnitNormalizationResult VegetationMeasuredCoordinateNormalizationResult VegetationMeasuredPoint3d VegetationQuantitativeStructureModelCylinderRecord VegetationRootArchitectureGraphSegmentRecord VegetationCanopyObservationRecord VegetationPhenologyObservationRecord VegetationBiomechanicalMaterialTestRecord VegetationMeasuredCoordinateReferenceTransform VegetationMeasuredCoordinateReferenceTransformValidationReport VegetationMeasuredCoordinateReferenceTransformResult VegetationMeasuredSourceSchemaRegistration VegetationMeasuredSourceDecodeContext VegetationMeasuredSourceDecodeReport VegetationPointCloudBounds3d VegetationPointCloudFormatCapability VegetationPointCloudSourceMetadata VegetationPointCloudPointRecord VegetationPointCloudDataset VegetationPointCloudIngestionPolicy VegetationPointCloudIngestionContext VegetationPointCloudIngestionReport VegetationPointCloudReconstructionAdmissionPolicy VegetationPointCloudReconstructionAdmissionReport VegetationPointCloudReconstructionJob VegetationCanopyVoxelIndex VegetationCanopyOccupancyCell VegetationCanopyOccupancyReconstructionPolicy VegetationCanopyOccupancyQualityEvidence VegetationCanopyOccupancyQualityReport VegetationCanopyOccupancyReconstruction VegetationCanopyOccupancyReconstructionReport VegetationWoodyAxisRadiusStation VegetationWoodyAxisCylinder VegetationWoodyAxisReconstructionPolicy VegetationWoodyAxisQualityEvidence VegetationWoodyAxisQualityReport VegetationWoodyAxisReconstruction VegetationWoodyAxisReconstructionReport VegetationWoodyAxisSegmentDefinition VegetationWoodyPointSegmentAssignment VegetationWoodyPointSegmentation VegetationWoodyPointSegmentationValidationReport VegetationWoodyBranchCylinder VegetationWoodyBranchAxis VegetationWoodyBranchConnection VegetationWoodyBranchGraphReconstructionPolicy VegetationWoodyBranchAxisQualityEvidence VegetationWoodyBranchGraphQualityEvidence VegetationWoodyBranchGraphQualityReport VegetationWoodyBranchGraphReconstruction VegetationWoodyBranchGraphReconstructionReport VegetationWoodySegmentationParameterSet VegetationWoodyCoverSet VegetationWoodyCoverConnection VegetationWoodySegmentationAxisCandidate VegetationWoodySegmentationCandidate VegetationWoodySegmentationCandidateIssue VegetationWoodySegmentationCandidateReport VegetationWoodySegmentationCandidateEvaluation VegetationWoodySegmentationSensitivityPolicy VegetationWoodySegmentationSensitivityReport VegetationAutomatedWoodySegmentationEnsembleIssue VegetationAutomatedWoodySegmentationEnsembleReport; do
	if ! rg -q "class $model_class" "$repository_root/include/vegetation/model"; then
		echo "FAIL: missing vegetation biological model class $model_class" >&2
		exit 1
	fi
done

for service_class in VegetationBiologicalProfileValidationService VegetationCalibrationReadinessEvaluationService VegetationCalibrationEvidenceBundleSerializationService VegetationCalibrationPayloadHashService VegetationCalibrationEvidenceBundleParsingService VegetationCalibrationEvidenceBundleValidationService VegetationCalibrationEvidenceBindingService VegetationMeasuredSourceArtifactHashService VegetationMeasuredSourceArtifactValidationService VegetationMeasuredCoordinateNormalizationService VegetationMeasurementUnitNormalizationService VegetationMeasuredCalibrationMeasurementFactory VegetationMeasuredEvidenceBundleFactory VegetationQuantitativeStructureModelAdapter VegetationRootArchitectureGraphAdapter VegetationCanopyObservationAdapter VegetationPhenologyObservationSeriesAdapter VegetationBiomechanicalMaterialTestAdapter VegetationMeasuredSourceSchemaRegistry VegetationMeasuredCoordinateReferenceTransformService VegetationMeasuredPointCanonicalizationService VegetationMeasuredSourceDecoderArtifactValidationService VegetationDecodedMeasuredSourceArtifactFactory VegetationTreeQsmCylinderTableDecoder VegetationRootSystemMarkupLanguageDecoder VegetationCanopyObservationTableDecoder VegetationPhenologyObservationTableDecoder VegetationBiomechanicalMaterialTestTableDecoder VegetationMeasuredSourceDecodingService VegetationPointCloudFormatCapabilityCatalog VegetationPointCloudSourceArtifactValidationService VegetationPlyPointCloudDecoder VegetationLasPointCloudDecoder VegetationPointCloudIngestionService VegetationPointCloudReconstructionAdmissionService VegetationPointCloudReconstructionJobFactory VegetationCanopyOccupancyQualityEvaluationService VegetationCanopyOccupancyReconstructionService VegetationWoodyAxisQualityEvaluationService VegetationPrimaryWoodyAxisReconstructionService VegetationWoodyPointSegmentationValidationService VegetationWoodyBranchGraphQualityEvaluationService VegetationSegmentedWoodyBranchGraphReconstructionService VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory VegetationWoodyCoverSetSegmentationService VegetationWoodySegmentationSensitivityEvaluationService VegetationAutomatedWoodySegmentationEnsembleService; do
	if ! rg -q "class $service_class" "$repository_root/include/vegetation/service"; then
		echo "FAIL: missing vegetation biological service class $service_class" >&2
		exit 1
	fi
done

if rg -n '\.classification\(\)' \
	"$repository_root/src/vegetation/service/VegetationWoodyCoverSetSegmentationService.cpp"; then
	echo "FAIL: automated woody segmentation must not infer axes from vendor classification values" >&2
	exit 1
fi

echo "Building vegetation triangle distribution source gate passed."
