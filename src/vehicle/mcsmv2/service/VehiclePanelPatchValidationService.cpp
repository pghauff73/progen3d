#include "vehicle/mcsmv2/service/VehiclePanelPatchValidationService.h"

#include <set>
#include <string>

ModernCarSemanticValidationReport VehiclePanelPatchValidationService::validate(
	const VehiclePanelPatchGraph &graph) const
{
	ModernCarSemanticValidationReport report;
	const std::set<std::string> required = {
		"hood", "roof", "front_door_left", "front_door_right",
		"rear_door_left", "rear_door_right", "rear_quarter_left",
		"rear_quarter_right", "rear_hatch", "front_bumper", "rear_bumper"};
	std::set<std::string> identifiers;
	for (const VehiclePanelPatch &patch : graph.patches()) {
		if (!identifiers.insert(patch.identifier()).second ||
		    patch.sourceFaceCount() == 0u ||
		    patch.selectionBasis() !=
			    VehiclePanelPatchSelectionBasis::SemanticSurfaceTag) {
			report.addIssue({
				ModernCarSemanticValidationCode::InvalidPanelGraph,
				"Panel patches must be unique, non-empty semantic-tag selections.",
				{graph.variantIdentifier(), patch.identifier()}});
		}
	}
	for (const std::string &identifier : required) {
		if (identifiers.count(identifier) == 0u) {
			report.addIssue({
				ModernCarSemanticValidationCode::InvalidPanelGraph,
				"Required vehicle panel patch is missing.",
				{graph.variantIdentifier(), identifier}});
		}
	}
	std::set<std::string> relationship_keys;
	for (const VehiclePanelAdjacencyRelationship &relationship :
	     graph.relationships()) {
		const std::string key = relationship.firstPanelIdentifier() + "|" +
			relationship.secondPanelIdentifier() + "|" + relationship.relationship();
		if (relationship.firstPanelIdentifier() ==
			    relationship.secondPanelIdentifier() ||
		    identifiers.count(relationship.firstPanelIdentifier()) == 0u ||
		    identifiers.count(relationship.secondPanelIdentifier()) == 0u ||
		    relationship.relationship() != "panel_gap" ||
		    !relationship_keys.insert(key).second) {
			report.addIssue({
				ModernCarSemanticValidationCode::InvalidPanelGraph,
				"Panel adjacency must reference distinct known panels through one panel_gap relationship.",
				{graph.variantIdentifier(), relationship.firstPanelIdentifier(),
				 relationship.secondPanelIdentifier()}});
		}
	}
	return report;
}
