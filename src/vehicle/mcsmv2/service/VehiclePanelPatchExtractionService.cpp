#include "vehicle/mcsmv2/service/VehiclePanelPatchExtractionService.h"

#include "vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h"

#include <stdexcept>
#include <vector>

namespace {

const GeneratedMcsMv2VariantRecord &sourceRecordFor(
	const std::string &variant_identifier)
{
	for (const GeneratedMcsMv2VariantRecord &record :
	     generatedMcsMv2VariantRecords()) {
		if (record.key == variant_identifier) return record;
	}
	throw std::invalid_argument(
		"No generated MCSMv2 source record exists for variant '" +
		variant_identifier + "'.");
}

} // namespace

VehiclePanelPatchGraph VehiclePanelPatchExtractionService::extract(
	const ModernCarSemanticVariant &variant) const
{
	const GeneratedMcsMv2VariantRecord &record =
		sourceRecordFor(variant.identifier());
	std::vector<VehiclePanelPatch> patches;
	patches.reserve(record.panel_patches.size());
	for (const GeneratedMcsMv2PanelPatchRecord &panel : record.panel_patches) {
		patches.emplace_back(
			std::string(panel.identifier),
			panel.source_face_count,
			panel.closure,
			VehiclePanelPatchSelectionBasis::SemanticSurfaceTag);
	}
	std::vector<VehiclePanelAdjacencyRelationship> relationships;
	relationships.reserve(record.panel_relationships.size());
	for (const GeneratedMcsMv2PanelRelationshipRecord &relationship :
	     record.panel_relationships) {
		relationships.emplace_back(
			std::string(relationship.first_panel_identifier),
			std::string(relationship.second_panel_identifier),
			std::string(relationship.relationship));
	}
	return VehiclePanelPatchGraph(
		variant.identifier(), std::move(patches), std::move(relationships));
}
