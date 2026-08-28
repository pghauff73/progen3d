#include "vegetation/service/SpaceColonizationSeedGraphFactory.h"

#include "vegetation/model/BranchNode.h"
#include "vegetation/model/BranchNodeState.h"
#include "vegetation/model/BranchSegment.h"
#include "vegetation/model/BranchSegmentKind.h"
#include "vegetation/model/VegetationBud.h"
#include "vegetation/model/VegetationGrowthTip.h"
#include "vegetation/service/CrownVolumeContainmentService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

glm::vec3 seed_target(const CrownVolumeSpecification &crown)
{
	switch (crown.kind()) {
	case CrownVolumeKind::Sphere:
	case CrownVolumeKind::Ellipsoid:
	case CrownVolumeKind::Lobed:
		return crown.origin();
	case CrownVolumeKind::Cone:
	case CrownVolumeKind::Cylinder:
	case CrownVolumeKind::Dome:
		return crown.origin() + glm::vec3(0.0f, crown.dimensions().y * 0.25f, 0.0f);
	case CrownVolumeKind::InverseCone:
		return crown.origin() + glm::vec3(0.0f, crown.dimensions().y * 0.5f, 0.0f);
	case CrownVolumeKind::CustomSampled:
		return crown.customSamples().empty()
			? glm::vec3(0.0f)
			: crown.customSamples().front();
	}
	return glm::vec3(0.0f);
}

} // namespace

std::optional<BranchGraph> SpaceColonizationSeedGraphFactory::create(
	const PlantSpeciesSpecification &species,
	const PlantSpaceColonizationSpecification &space_colonization,
	std::string *diagnostic) const
{
	const CrownVolumeSpecification &crown = space_colonization.crownVolume();
	const CrownVolumeContainmentService containment;
	std::string crown_diagnostic;
	if (!containment.validate(crown, &crown_diagnostic)) {
		if (diagnostic != nullptr) *diagnostic = crown_diagnostic;
		return std::nullopt;
	}
	const glm::vec3 root_position(0.0f);
	const glm::vec3 target_position = seed_target(crown);
	if (!containment.contains(crown, target_position)) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Space-colonization crown could not provide a valid primary-axis seed point.";
		}
		return std::nullopt;
	}
	const glm::vec3 axis = target_position - root_position;
	const float axis_length = glm::length(axis);
	if (!std::isfinite(axis_length) || axis_length <= 1.0e-5f) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Space-colonization crown seed must be separated from the plant root.";
		}
		return std::nullopt;
	}

	const PlantBranchingSpecification &branching = species.branching();
	const int internode_count = std::max(1, branching.primaryInternodeCount());
	std::vector<BranchNode> nodes;
	std::vector<BranchSegment> segments;
	nodes.reserve(static_cast<std::size_t>(internode_count + 1));
	segments.reserve(static_cast<std::size_t>(internode_count));
	for (int index = 0; index <= internode_count; ++index) {
		const float factor = static_cast<float>(index) /
			static_cast<float>(internode_count);
		const std::string identifier = "colonization_seed_node_" +
			std::to_string(index);
		const float radius = glm::mix(
			branching.baseRadius(),
			std::max(branching.tipRadius(), branching.baseRadius() * 0.55f),
			factor);
		nodes.emplace_back(
			identifier, glm::mix(root_position, target_position, factor), radius,
			factor, 0, BranchNodeState::Active);
		if (index > 0) {
			segments.emplace_back(
				"colonization_seed_segment_" + std::to_string(index - 1),
				"colonization_seed_node_" + std::to_string(index - 1),
				identifier, BranchSegmentKind::Continuation);
		}
	}
	const std::string terminal_identifier =
		"colonization_seed_node_" + std::to_string(internode_count);
	const glm::vec3 direction = glm::normalize(axis);
	std::vector<VegetationBud> buds;
	buds.emplace_back(
		"bud_" + terminal_identifier, terminal_identifier, direction, 0.0f,
		VegetationBudState::BranchBud, VegetationOrganType::Branch, 1.0f);
	std::vector<VegetationGrowthTip> growth_tips;
	growth_tips.emplace_back(
		"tip_" + terminal_identifier, terminal_identifier, direction, true);
	if (diagnostic != nullptr) diagnostic->clear();
	return BranchGraph(
		"colonization_seed_node_0", std::move(nodes), std::move(segments),
		std::move(buds), {}, std::move(growth_tips));
}
