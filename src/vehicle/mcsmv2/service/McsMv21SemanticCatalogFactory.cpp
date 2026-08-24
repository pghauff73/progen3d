#include "vehicle/mcsmv2/service/McsMv21SemanticCatalogFactory.h"

#include "vehicle/mcsmv2/generated/GeneratedMcsMv21Catalog.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

VehicleSurfaceDomainKind domainKind(const std::string &kind)
{
	if (kind == "panel") return VehicleSurfaceDomainKind::Panel;
	if (kind == "aperture") return VehicleSurfaceDomainKind::Aperture;
	throw std::invalid_argument("Unsupported MCSMv2.1 surface domain kind.");
}

VehicleSurfaceDomain createDomain(const GeneratedMcsMv21SurfaceDomainRecord &record)
{
	std::vector<VehicleSurfaceDomainLoop> loops;
	loops.reserve(record.loop_count);
	for (std::size_t loop_index = 0u; loop_index < record.loop_count; ++loop_index) {
		std::vector<VehicleSurfaceCoordinate> coordinates;
		coordinates.reserve(record.loops[loop_index].size());
		for (const GeneratedMcsMv21SurfaceCoordinateRecord &coordinate :
		     record.loops[loop_index]) {
			coordinates.emplace_back(coordinate.u, coordinate.v);
		}
		loops.emplace_back(std::move(coordinates));
	}
	return VehicleSurfaceDomain(
		std::string(record.name), domainKind(std::string(record.kind)), record.closure,
		std::string(record.parent_panel), std::move(loops));
}

SurfaceCorrespondenceDirectionReport createDirectionReport(
	const GeneratedMcsMv21CorrespondenceDirectionRecord &record)
{
	return SurfaceCorrespondenceDirectionReport(
		record.sample_count,
		record.mean_distance_m,
		record.rms_distance_m,
		record.p95_distance_m,
		record.maximum_distance_m,
		record.mean_normal_angle_deg,
		record.p95_normal_angle_deg,
		record.maximum_normal_angle_deg,
		record.minimum_normal_dot,
		std::string(record.closest_point_checksum));
}

SemanticImplicitCorrespondenceReport createCorrespondenceReport(
	const GeneratedMcsMv21CorrespondenceRecord &record)
{
	return SemanticImplicitCorrespondenceReport(
		createDirectionReport(record.semantic_to_scaffold),
		createDirectionReport(record.scaffold_to_semantic),
		record.p95_distance_tolerance_m,
		record.maximum_distance_tolerance_m,
		record.p95_normal_angle_tolerance_deg,
		{}, record.pass);
}

} // namespace

McsMv21SemanticFamilyDefinition
McsMv21SemanticCatalogFactory::createFamilyDefinition() const
{
	const GeneratedMcsMv21SourceReleaseRecord &source_record =
		generatedMcsMv21SourceReleaseRecord();
	std::vector<McsMv21SemanticVariantDefinition> variants;
	variants.reserve(generatedMcsMv21VariantRecords().size());
	for (const GeneratedMcsMv21VariantRecord &record :
	     generatedMcsMv21VariantRecords()) {
		std::vector<VehicleSurfaceDomain> panels;
		std::vector<VehicleSurfaceDomain> apertures;
		panels.reserve(record.panel_domains.size());
		apertures.reserve(record.aperture_domains.size());
		for (const GeneratedMcsMv21SurfaceDomainRecord &domain : record.panel_domains) {
			panels.push_back(createDomain(domain));
		}
		for (const GeneratedMcsMv21SurfaceDomainRecord &domain : record.aperture_domains) {
			apertures.push_back(createDomain(domain));
		}
		variants.emplace_back(
			std::string(record.identifier), std::string(record.display_name),
			VehicleSurfaceDomainCatalog(
				std::string(record.identifier), std::move(panels), std::move(apertures)),
			createCorrespondenceReport(record.accepted_correspondence),
			record.registered_surface_vertex_count,
			record.registered_surface_face_count,
			record.final_body_vertex_count,
			record.final_body_face_count,
			record.source_release_gate_pass,
			std::string(record.assurance_level));
	}
	return McsMv21SemanticFamilyDefinition(
		McsMv210SourceRelease(
			std::string(source_record.version),
			std::string(source_record.model_schema),
			std::string(source_record.manifest_schema),
			std::string(source_record.release_manifest_sha256),
			source_record.signed_artifact_count),
		std::move(variants));
}
