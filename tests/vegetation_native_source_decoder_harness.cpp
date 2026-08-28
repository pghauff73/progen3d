#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"
#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransform.h"
#include "vegetation/model/VegetationMeasuredSourceDecodeReport.h"
#include "vegetation/service/VegetationBiomechanicalMaterialTestAdapter.h"
#include "vegetation/service/VegetationCanopyObservationAdapter.h"
#include "vegetation/service/VegetationMeasuredCoordinateReferenceTransformService.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"
#include "vegetation/service/VegetationMeasuredSourceDecodingService.h"
#include "vegetation/service/VegetationMeasuredSourceSchemaRegistry.h"
#include "vegetation/service/VegetationPhenologyObservationSeriesAdapter.h"
#include "vegetation/service/VegetationQuantitativeStructureModelAdapter.h"
#include "vegetation/service/VegetationRootArchitectureGraphAdapter.h"

#include <json/json.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using DecodeIssueCode = VegetationMeasuredSourceDecodeIssueCode;
using ObservationKind = VegetationMeasuredSourceDecodeObservationKind;

std::string read_text_file(const std::string &path)
{
	std::ifstream stream(path);
	assert(stream.good());
	return std::string(
		std::istreambuf_iterator<char>(stream),
		std::istreambuf_iterator<char>());
}

Json::Value parse_json(const std::string &text)
{
	Json::CharReaderBuilder builder;
	Json::Value value;
	std::string errors;
	std::istringstream stream(text);
	assert(Json::parseFromStream(builder, stream, &value, &errors));
	return value;
}

VegetationCalibrationSubjectScope measured_tree_scope()
{
	return VegetationCalibrationSubjectScope(
		VegetationCalibrationSubjectScopeKind::MeasuredSpecimen,
		PlantArchitecture::Tree,
		"specimen:tree-001",
		"Lophostemon confertus",
		std::string(),
		"tree-001");
}

VegetationMeasuredSourceArtifact source_artifact(
	const std::string &source_identifier,
	const std::string &media_type,
	const std::string &coordinate_system,
	const std::string &unit_system,
	std::string payload)
{
	const VegetationMeasuredSourceArtifact unsigned_artifact(
		"ProGen3D-VegetationMeasuredSourceArtifact-v1",
		source_identifier,
		"Frozen deterministic native measured-source fixture",
		"fixture://vegetation-native-sources/" + source_identifier,
		media_type,
		coordinate_system,
		unit_system,
		"Frozen native measured-source decoder fixture",
		"Fixture measurements retain explicit source, decoder, and transform uncertainty.",
		std::move(payload),
		std::string());
	return VegetationMeasuredSourceArtifactHashService().attachPayloadSha256(
		unsigned_artifact);
}

bool has_issue(
	const VegetationMeasuredSourceDecodeReport &report,
	DecodeIssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](const VegetationMeasuredSourceDecodeIssue &issue) {
			return issue.code() == expected_code;
		});
}

bool has_observation(
	const VegetationMeasuredSourceDecodeReport &report,
	ObservationKind expected_kind)
{
	return std::any_of(
		report.observations().begin(), report.observations().end(),
		[expected_kind](
			const VegetationMeasuredSourceDecodeObservation &observation) {
			return observation.kind() == expected_kind;
		});
}

VegetationMeasuredCoordinateReferenceTransform world_to_plant_transform()
{
	return VegetationMeasuredCoordinateReferenceTransform(
		"ProGen3D-VegetationMeasuredCoordinateReferenceTransform-v1",
		"transform:survey-world-to-tree-001",
		"SurveyWorldXYZ-ZUp",
		"LocalPlantXYZ-ZUp",
		"cm",
		"m",
		{
			0.0, -1.0, 0.0, 10.0,
			1.0, 0.0, 0.0, -5.0,
			0.0, 0.0, 1.0, 2.0,
			0.0, 0.0, 0.0, 1.0,
		},
		"Survey control points constrain the affine transform within one millimetre.",
		"survey-control:tree-001",
		std::string(64u, 'a'));
}

VegetationMeasuredCoordinateReferenceTransform identity_world_transform()
{
	return VegetationMeasuredCoordinateReferenceTransform(
		"ProGen3D-VegetationMeasuredCoordinateReferenceTransform-v1",
		"transform:identity-world-to-tree-001",
		"SurveyWorldXYZ-ZUp",
		"LocalPlantXYZ-ZUp",
		"m",
		"m",
		{
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0,
		},
		"Identity fixture transform has exact coordinates.",
		"survey-control:identity",
		std::string(64u, 'b'));
}

void verify_schema_registry()
{
	const VegetationMeasuredSourceSchemaRegistry registry;
	assert(registry.registrations().size() == 5u);
	const auto treeqsm = registry.resolveUnique(
		"text/vnd.treeqsm.cylinder-table",
		"TreeQSM-save_model_text-1.1.0");
	assert(treeqsm.has_value());
	assert(
		treeqsm->decoderIdentifier() ==
		"VegetationTreeQsmCylinderTableDecoder");
	assert(!registry.resolveUnique(
		"text/vnd.treeqsm.cylinder-table", "TreeQSM-unknown")
			.has_value());

	const VegetationMeasuredSourceSchemaRegistration duplicate = *treeqsm;
	const VegetationMeasuredSourceSchemaRegistry ambiguous_registry(
		{*treeqsm, duplicate});
	assert(
		ambiguous_registry
			.findRegistrations(
				"text/vnd.treeqsm.cylinder-table",
				"TreeQSM-save_model_text-1.1.0")
			.size() == 2u);
	assert(!ambiguous_registry
			.resolveUnique(
				"text/vnd.treeqsm.cylinder-table",
				"TreeQSM-save_model_text-1.1.0")
			.has_value());
}

void verify_coordinate_reference_transform()
{
	const auto transform = world_to_plant_transform();
	const VegetationMeasuredCoordinateReferenceTransformService service;
	assert(service.validate(transform).valid());
	const auto target = service.mapSourceToTarget(
		VegetationMeasuredPoint3d(100.0, 200.0, 300.0), transform);
	assert(target.succeeded());
	assert(std::abs(target.transformedPoint()->x() - 8.0) < 1.0e-12);
	assert(std::abs(target.transformedPoint()->y() + 4.0) < 1.0e-12);
	assert(std::abs(target.transformedPoint()->z() - 5.0) < 1.0e-12);
	const auto source =
		service.mapTargetToSource(*target.transformedPoint(), transform);
	assert(source.succeeded());
	assert(std::abs(source.transformedPoint()->x() - 100.0) < 1.0e-10);
	assert(std::abs(source.transformedPoint()->y() - 200.0) < 1.0e-10);
	assert(std::abs(source.transformedPoint()->z() - 300.0) < 1.0e-10);

	const VegetationMeasuredCoordinateReferenceTransform singular(
		transform.schemaVersion(),
		"transform:singular",
		transform.sourceCoordinateSystem(),
		transform.targetCoordinateSystem(),
		transform.sourceCoordinateUnit(),
		transform.targetCoordinateUnit(),
		{
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 1.0,
		},
		transform.uncertaintyStatement(),
		transform.evidenceIdentifier(),
		transform.evidenceSha256());
	assert(!service.validate(singular).valid());
}

void verify_provenance(
	const VegetationMeasuredSourceArtifact &source,
	const VegetationMeasuredSourceDecodeReport &report)
{
	assert(report.succeeded());
	assert(report.sourceIdentifier() == source.sourceIdentifier());
	assert(report.sourcePayloadSha256() == source.payloadSha256());
	assert(report.canonicalArtifact()->payloadSha256() != source.payloadSha256());
	const Json::Value document =
		parse_json(report.canonicalArtifact()->sourcePayload());
	assert(
		document["source_provenance"]["source_identifier"].asString() ==
		source.sourceIdentifier());
	assert(
		document["source_provenance"]["source_payload_sha256"].asString() ==
		source.payloadSha256());
	assert(
		document["source_provenance"]["decoder_identifier"].asString() ==
		report.decoderIdentifier());
}

struct NativeFixtures
{
	VegetationMeasuredSourceArtifact treeqsm;
	VegetationMeasuredSourceArtifact rsml;
	VegetationMeasuredSourceArtifact canopy;
	VegetationMeasuredSourceArtifact phenology;
	VegetationMeasuredSourceArtifact biomechanics;
};

NativeFixtures load_fixtures(const std::string &directory)
{
	return NativeFixtures{
		source_artifact(
			"source:treeqsm:tree-001",
			"text/vnd.treeqsm.cylinder-table",
			"LocalPlantXYZ-ZUp",
			"SI",
			read_text_file(
				directory + "/treeqsm_cylinder_save_model_text_1_1_0.txt")),
		source_artifact(
			"source:rsml:tree-001",
			"application/rsml+xml",
			"LocalPlantXYZ-ZUp",
			"DeclaredPerField",
			read_text_file(
				directory + "/tree_root_architecture_rsml_v1.rsml")),
		source_artifact(
			"source:canopy-table:tree-001",
			"text/vnd.progen3d.canopy-observation-table",
			"LocalPlantXYZ-ZUp",
			"DeclaredPerField",
			read_text_file(
				directory + "/tree_canopy_observation_table_v1.csv")),
		source_artifact(
			"source:phenology-table:tree-001",
			"text/vnd.progen3d.phenology-series-table",
			"LocalPlantXYZ-ZUp",
			"DeclaredPerField",
			read_text_file(
				directory + "/tree_phenology_observation_table_v1.csv")),
		source_artifact(
			"source:biomechanics-table:tree-001",
			"text/vnd.progen3d.biomechanical-test-table",
			"LocalPlantXYZ-ZUp",
			"DeclaredPerField",
			read_text_file(
				directory +
				"/tree_biomechanical_material_test_table_v1.csv")),
	};
}

void verify_native_decoders(const NativeFixtures &fixtures)
{
	const VegetationMeasuredSourceDecodingService decoding;
	const auto qsm_context = VegetationMeasuredSourceDecodeContext(
		"TreeQSM-save_model_text-1.1.0",
		PlantArchitecture::Tree,
		"qsm:tree-001:native-v1");
	const auto qsm = decoding.decode(fixtures.treeqsm, qsm_context);
	assert(qsm.succeeded());
	assert(has_observation(qsm, ObservationKind::DecodedRecord));
	assert(has_observation(qsm, ObservationKind::DerivedGeometry));
	assert(has_observation(qsm, ObservationKind::DeferredField));
	verify_provenance(fixtures.treeqsm, qsm);
	const auto repeated_qsm = decoding.decode(fixtures.treeqsm, qsm_context);
	assert(repeated_qsm.succeeded());
	assert(
		repeated_qsm.canonicalArtifact()->sourcePayload() ==
		qsm.canonicalArtifact()->sourcePayload());
	assert(
		repeated_qsm.canonicalArtifact()->payloadSha256() ==
		qsm.canonicalArtifact()->payloadSha256());

	const auto rsml = decoding.decode(
		fixtures.rsml,
		VegetationMeasuredSourceDecodeContext(
			"RSML-1",
			PlantArchitecture::Tree,
			"root-graph:tree-001:rsml-v1"));
	assert(rsml.succeeded());
	assert(has_observation(rsml, ObservationKind::DecodedRecord));
	assert(has_observation(rsml, ObservationKind::DerivedGeometry));
	assert(has_observation(rsml, ObservationKind::LossyConversion));
	assert(has_observation(rsml, ObservationKind::DeferredField));
	verify_provenance(fixtures.rsml, rsml);

	const auto canopy = decoding.decode(
		fixtures.canopy,
		VegetationMeasuredSourceDecodeContext(
			"ProGen3D-CanopyObservationTable-v1",
			PlantArchitecture::Tree,
			"canopy-observation:tree-001:leaf-on"));
	assert(canopy.succeeded());
	verify_provenance(fixtures.canopy, canopy);

	const auto phenology = decoding.decode(
		fixtures.phenology,
		VegetationMeasuredSourceDecodeContext(
			"ProGen3D-PhenologyObservationTable-v1",
			PlantArchitecture::Tree,
			"phenology-series:tree-001:2025"));
	assert(phenology.succeeded());
	verify_provenance(fixtures.phenology, phenology);

	const auto biomechanics = decoding.decode(
		fixtures.biomechanics,
		VegetationMeasuredSourceDecodeContext(
			"ProGen3D-BiomechanicalMaterialTestTable-v1",
			PlantArchitecture::Tree,
			"biomechanical-test:tree-001:v1"));
	assert(biomechanics.succeeded());
	verify_provenance(fixtures.biomechanics, biomechanics);

	const auto scope = measured_tree_scope();
	assert(VegetationQuantitativeStructureModelAdapter()
		       .adapt(*qsm.canonicalArtifact(), scope)
		       .succeeded());
	assert(VegetationRootArchitectureGraphAdapter()
		       .adapt(*rsml.canonicalArtifact(), scope)
		       .succeeded());
	assert(VegetationCanopyObservationAdapter()
		       .adapt(*canopy.canonicalArtifact(), scope)
		       .succeeded());
	assert(VegetationPhenologyObservationSeriesAdapter()
		       .adapt(*phenology.canonicalArtifact(), scope)
		       .succeeded());
	assert(VegetationBiomechanicalMaterialTestAdapter()
		       .adapt(*biomechanics.canonicalArtifact(), scope)
		       .succeeded());
}

void verify_georeferenced_decode(const NativeFixtures &fixtures)
{
	const VegetationMeasuredSourceArtifact global_treeqsm(
		fixtures.treeqsm.schemaVersion(),
		fixtures.treeqsm.sourceIdentifier(),
		fixtures.treeqsm.sourceCitation(),
		fixtures.treeqsm.sourceLocator(),
		fixtures.treeqsm.mediaType(),
		"SurveyWorldXYZ-ZUp",
		fixtures.treeqsm.unitSystem(),
		fixtures.treeqsm.acquisitionMethod(),
		fixtures.treeqsm.uncertaintyStatement(),
		fixtures.treeqsm.sourcePayload(),
		fixtures.treeqsm.payloadSha256());
	const VegetationMeasuredSourceDecodingService decoding;
	const auto without_transform = decoding.decode(
		global_treeqsm,
		VegetationMeasuredSourceDecodeContext(
			"TreeQSM-save_model_text-1.1.0",
			PlantArchitecture::Tree,
			"qsm:tree-001:global-without-transform"));
	assert(!without_transform.succeeded());
	assert(has_issue(
		without_transform,
		DecodeIssueCode::InvalidCoordinateReferenceTransform));

	const auto with_transform = decoding.decode(
		global_treeqsm,
		VegetationMeasuredSourceDecodeContext(
			"TreeQSM-save_model_text-1.1.0",
			PlantArchitecture::Tree,
			"qsm:tree-001:global-with-transform",
			identity_world_transform()));
	assert(with_transform.succeeded());
	const Json::Value canonical =
		parse_json(with_transform.canonicalArtifact()->sourcePayload());
	assert(
		canonical["source_provenance"]["coordinate_transform_identifier"]
			.asString() == "transform:identity-world-to-tree-001");
	assert(VegetationQuantitativeStructureModelAdapter()
		       .adapt(*with_transform.canonicalArtifact(), measured_tree_scope())
		       .succeeded());
}

void verify_rejections(const NativeFixtures &fixtures)
{
	const VegetationMeasuredSourceDecodingService decoding;
	const auto unsupported_version = decoding.decode(
		fixtures.treeqsm,
		VegetationMeasuredSourceDecodeContext(
			"TreeQSM-save_model_text-unknown",
			PlantArchitecture::Tree,
			"qsm:unsupported"));
	assert(!unsupported_version.succeeded());
	assert(has_issue(
		unsupported_version,
		DecodeIssueCode::UnsupportedSourceSchemaVersion));

	std::string invalid_header = fixtures.treeqsm.sourcePayload();
	invalid_header.replace(
		invalid_header.find("radius (m)"),
		std::string("radius (m)").size(),
		"radius");
	const auto malformed_treeqsm = decoding.decode(
		source_artifact(
			"source:treeqsm:invalid-header",
			fixtures.treeqsm.mediaType(),
			fixtures.treeqsm.coordinateSystem(),
			fixtures.treeqsm.unitSystem(),
			invalid_header),
		VegetationMeasuredSourceDecodeContext(
			"TreeQSM-save_model_text-1.1.0",
			PlantArchitecture::Tree,
			"qsm:invalid-header"));
	assert(!malformed_treeqsm.succeeded());
	assert(has_issue(
		malformed_treeqsm,
		DecodeIssueCode::UnexpectedColumnLayout));

	std::string two_dimensional_rsml = fixtures.rsml.sourcePayload();
	const std::string z_attribute = " z=\"0\"";
	const std::size_t z_position = two_dimensional_rsml.find(z_attribute);
	assert(z_position != std::string::npos);
	two_dimensional_rsml.erase(z_position, z_attribute.size());
	const auto invalid_rsml = decoding.decode(
		source_artifact(
			"source:rsml:two-dimensional",
			fixtures.rsml.mediaType(),
			fixtures.rsml.coordinateSystem(),
			fixtures.rsml.unitSystem(),
			two_dimensional_rsml),
		VegetationMeasuredSourceDecodeContext(
			"RSML-1",
			PlantArchitecture::Tree,
			"root-graph:two-dimensional"));
	assert(!invalid_rsml.succeeded());
	assert(has_issue(invalid_rsml, DecodeIssueCode::MissingRequiredField));

	const auto stale = decoding.decode(
		fixtures.canopy.withPayloadSha256(std::string(64u, '0')),
		VegetationMeasuredSourceDecodeContext(
			"ProGen3D-CanopyObservationTable-v1",
			PlantArchitecture::Tree,
			"canopy:stale"));
	assert(!stale.succeeded());
	assert(has_issue(stale, DecodeIssueCode::SourceHashMismatch));
}

}

int main(int argc, char **argv)
{
	assert(argc == 2);
	verify_schema_registry();
	verify_coordinate_reference_transform();
	const NativeFixtures fixtures = load_fixtures(argv[1]);
	verify_native_decoders(fixtures);
	verify_georeferenced_decode(fixtures);
	verify_rejections(fixtures);
	std::cout << "Vegetation native source decoder checks passed.\n";
	return 0;
}
