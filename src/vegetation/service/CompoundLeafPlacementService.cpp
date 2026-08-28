#include "vegetation/service/CompoundLeafPlacementService.h"

#include "vegetation/model/PhyllotaxisSpecification.h"
#include "vegetation/model/VegetationOrganPlacement.h"
#include "vegetation/model/VegetationOrganType.h"
#include "vegetation/service/OrganPlacementService.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

bool supported_pattern(PhyllotaxisMode pattern)
{
	return pattern == PhyllotaxisMode::Alternate ||
	       pattern == PhyllotaxisMode::Opposite;
}

} // namespace

std::optional<CompoundLeafPlacement> CompoundLeafPlacementService::place(
	const std::vector<OrganAttachment> &organ_attachments,
	const PlantPetioleSpecification &petiole,
	const PlantCompoundLeafSpecification &compound_leaf,
	std::string *diagnostic) const
{
	if (!compound_leaf.isEnabled() || compound_leaf.leafletNodeCount() < 2 ||
	    !finite_positive(petiole.length()) ||
	    !finite_positive(compound_leaf.rachisLength()) ||
	    !finite_positive(compound_leaf.rachisBaseRadius()) ||
	    !finite_positive(compound_leaf.rachisTipRadius()) ||
	    compound_leaf.rachisTipRadius() > compound_leaf.rachisBaseRadius() ||
	    !supported_pattern(compound_leaf.pattern()) ||
	    !std::isfinite(compound_leaf.divergenceDegrees()) ||
	    !std::isfinite(compound_leaf.orientationUpBias()) ||
	    compound_leaf.orientationUpBias() < 0.0f ||
	    !finite_positive(compound_leaf.leafletScale())) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Compound leaf placement requires an enabled alternate or opposite LeafArray with finite positive rachis and leaflet parameters.";
		}
		return std::nullopt;
	}

	const float leaflet_spacing = compound_leaf.rachisLength() /
		static_cast<float>(compound_leaf.leafletNodeCount() - 1);
	const int leaflets_per_node =
		compound_leaf.pattern() == PhyllotaxisMode::Opposite ? 2 : 1;
	const PhyllotaxisSpecification leaflet_pattern(
		compound_leaf.pattern(), compound_leaf.divergenceDegrees(),
		leaflet_spacing, leaflets_per_node, 0.0f,
		compound_leaf.rachisBaseRadius(),
		compound_leaf.orientationUpBias());

	std::string placement_diagnostic;
	const std::vector<VegetationOrganPlacement> local_leaflet_placements =
		OrganPlacementService().placeAlongPath(
			"compound_leaflet",
			{glm::vec3(0.0f),
			 glm::vec3(0.0f, compound_leaf.rachisLength(), 0.0f)},
			static_cast<std::size_t>(compound_leaf.leafletNodeCount()),
			leaflet_pattern, &placement_diagnostic);
	if (!placement_diagnostic.empty()) {
		if (diagnostic != nullptr) *diagnostic = placement_diagnostic;
		return std::nullopt;
	}

	std::size_t leaf_attachment_count = 0u;
	for (const OrganAttachment &attachment : organ_attachments) {
		if (attachment.organType() == VegetationOrganType::Leaf) {
			++leaf_attachment_count;
		}
	}

	std::vector<glm::mat4> rachis_transforms;
	std::vector<glm::mat4> leaflet_transforms;
	rachis_transforms.reserve(leaf_attachment_count);
	leaflet_transforms.reserve(
		leaf_attachment_count * local_leaflet_placements.size());
	const glm::mat4 leaflet_scale = glm::scale(
		glm::mat4(1.0f), glm::vec3(compound_leaf.leafletScale()));
	for (const OrganAttachment &attachment : organ_attachments) {
		if (attachment.organType() != VegetationOrganType::Leaf) continue;
		const glm::mat4 rachis_transform =
			attachment.localTransform() *
			glm::translate(
				glm::mat4(1.0f), glm::vec3(0.0f, petiole.length(), 0.0f));
		rachis_transforms.push_back(rachis_transform);
		for (const VegetationOrganPlacement &leaflet : local_leaflet_placements) {
			leaflet_transforms.push_back(
				rachis_transform * leaflet.localTransform() * leaflet_scale);
		}
	}

	if (diagnostic != nullptr) diagnostic->clear();
	return CompoundLeafPlacement(
		std::move(rachis_transforms), std::move(leaflet_transforms));
}

