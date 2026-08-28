#include "vegetation/service/RuleBranchingGenerationService.h"

#include "vegetation/service/BranchGraphDeterministicHashService.h"
#include "vegetation/service/BranchGraphValidationService.h"
#include "vegetation/service/DeterministicPlantVariationService.h"
#include "vegetation/service/OrganPlacementService.h"
#include "vegetation/service/TropismDirectionService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

bool valid_compound_leaf(
	const PlantCompoundLeafSpecification &compound_leaf)
{
	if (!compound_leaf.isEnabled()) return true;
	return compound_leaf.leafletNodeCount() >= 2 &&
	       finite_positive(compound_leaf.rachisLength()) &&
	       finite_positive(compound_leaf.rachisBaseRadius()) &&
	       finite_positive(compound_leaf.rachisTipRadius()) &&
	       compound_leaf.rachisTipRadius() <=
		       compound_leaf.rachisBaseRadius() &&
	       (compound_leaf.pattern() == PhyllotaxisMode::Alternate ||
	        compound_leaf.pattern() == PhyllotaxisMode::Opposite) &&
	       std::isfinite(compound_leaf.divergenceDegrees()) &&
	       std::isfinite(compound_leaf.orientationUpBias()) &&
	       compound_leaf.orientationUpBias() >= 0.0f &&
	       finite_positive(compound_leaf.leafletScale());
}

bool valid_organ_array(const PlantOrganArraySpecification &organ_array)
{
	const bool supported_organ =
		organ_array.organType() == VegetationOrganType::Bud ||
		organ_array.organType() == VegetationOrganType::Leaf ||
		organ_array.organType() == VegetationOrganType::Petal ||
		organ_array.organType() == VegetationOrganType::Flower ||
		organ_array.organType() == VegetationOrganType::Fruit ||
		organ_array.organType() == VegetationOrganType::Thorn;
	const bool path_host = organ_array.host() == VegetationOrganArrayHost::Stem ||
	                       organ_array.host() == VegetationOrganArrayHost::Branch;
	return !organ_array.identifier().empty() && supported_organ &&
	       organ_array.count() >= 1 && std::isfinite(organ_array.spacing()) &&
	       organ_array.spacing() >= 0.0f &&
	       (!path_host || organ_array.count() == 1 ||
	        organ_array.spacing() > 0.0f) &&
	       std::isfinite(organ_array.azimuthProgressionDegrees()) &&
	       finite_positive(organ_array.initialScale()) &&
	       finite_positive(organ_array.scaleFalloff()) &&
	       std::isfinite(organ_array.jitterFraction()) &&
	       organ_array.jitterFraction() >= 0.0f &&
	       organ_array.jitterFraction() <= 1.0f &&
	       organ_array.minimumBranchOrder() >= 0 &&
	       (organ_array.orientation() !=
		        VegetationOrganOrientation::SurfaceNormal ||
	        organ_array.host() == VegetationOrganArrayHost::Surface);
}

bool valid_fruit(const PlantFruitSpecification &fruit)
{
	return !fruit.isEnabled() ||
	       (finite_positive(fruit.height()) &&
	        finite_positive(fruit.maximumRadius()) &&
	        std::isfinite(fruit.shoulderFraction()) &&
	        fruit.shoulderFraction() >= 0.20f &&
	        fruit.shoulderFraction() <= 0.80f &&
	        std::isfinite(fruit.fullness()) && fruit.fullness() >= 2.0f &&
	        fruit.fullness() <= 12.0f && fruit.radialSegments() >= 6 &&
	        fruit.profileSegments() >= 4);
}

bool valid_species(const PlantSpeciesSpecification &species)
{
	const PlantBranchingSpecification &branching = species.branching();
	const PlantLeafSpecification &leaf = species.leaf();
	const PlantPetioleSpecification &petiole = leaf.petiole();
	const PlantCompoundLeafSpecification &compound_leaf = leaf.compoundLeaf();
	bool requires_fruit = false;
	for (const PlantOrganArraySpecification &organ_array : species.organArrays()) {
		if (!valid_organ_array(organ_array)) return false;
		if (organ_array.organType() == VegetationOrganType::Fruit) {
			requires_fruit = true;
		}
	}
	return !species.identifier().empty() &&
	       finite_positive(branching.primaryAxisLength()) &&
	       finite_positive(branching.baseRadius()) &&
	       finite_positive(branching.tipRadius()) &&
	       branching.tipRadius() <= branching.baseRadius() &&
	       branching.primaryInternodeCount() >= 1 &&
	       branching.maximumBranchOrder() >= 0 &&
	       branching.lateralBranchesPerNode() >= 0 &&
	       branching.branchEveryNInternodes() >= 1 &&
	       std::isfinite(branching.branchAngleDegrees()) &&
	       std::isfinite(branching.azimuthDivergenceDegrees()) &&
	       finite_positive(branching.branchLengthFraction()) &&
	       finite_positive(branching.branchLengthFalloff()) &&
	       finite_positive(branching.continuationRadiusRatio()) &&
	       branching.continuationRadiusRatio() < 1.0f &&
	       finite_positive(branching.radiusConservationGamma()) &&
	       std::isfinite(branching.directionVariationDegrees()) &&
	       branching.directionVariationDegrees() >= 0.0f &&
	       std::isfinite(branching.lengthVariationFraction()) &&
	       branching.lengthVariationFraction() >= 0.0f &&
	       std::isfinite(branching.upwardTropismWeight()) &&
	       branching.upwardTropismWeight() >= 0.0f &&
	       finite_positive(leaf.length()) && finite_positive(leaf.width()) &&
	       std::isfinite(leaf.camber()) &&
	       std::isfinite(leaf.twistDegrees()) &&
	       std::isfinite(leaf.thickness()) && leaf.thickness() >= 0.0f &&
	       leaf.minimumBranchOrder() >= 0 &&
	       finite_positive(petiole.length()) &&
	       finite_positive(petiole.baseRadius()) &&
	       finite_positive(petiole.tipRadius()) &&
	       petiole.tipRadius() <= petiole.baseRadius() &&
	       valid_compound_leaf(compound_leaf) &&
	       std::isfinite(species.floweringAge()) &&
	       std::isfinite(species.matureAge()) &&
	       species.floweringAge() >= 0.0f && species.matureAge() > 0.0f &&
	       valid_fruit(species.fruit()) &&
	       (!requires_fruit || species.fruit().isEnabled());
}

glm::vec3 radial_axis(const glm::vec3 &direction)
{
	const glm::vec3 reference = std::abs(direction.y) < 0.9f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::normalize(glm::cross(reference, direction));
}

struct GenerationState
{
	GenerationState(
		const RuleBranchingGenerationRequest &generation_request,
		const VegetationComplexityLimits &complexity_limits)
		: request(generation_request), limits(complexity_limits)
	{
	}

	const RuleBranchingGenerationRequest &request;
	const VegetationComplexityLimits &limits;
	DeterministicPlantVariationService variation;
	std::vector<BranchNode> nodes;
	std::vector<BranchSegment> segments;
	std::vector<VegetationBud> buds;
	std::vector<OrganAttachment> attachments;
	std::vector<VegetationGrowthTip> growth_tips;
	std::size_t next_node_index = 0u;
	std::size_t next_segment_index = 0u;
	std::string failure_diagnostic;

	std::string nextNodeIdentifier(int order)
	{
		return "branch_o" + std::to_string(order) + "_node_" +
		       std::to_string(next_node_index++);
	}

	std::string nextSegmentIdentifier(int order)
	{
		return "branch_o" + std::to_string(order) + "_segment_" +
		       std::to_string(next_segment_index++);
	}

	bool withinLimits() const
	{
		return nodes.size() <= limits.maximumBranchNodes() &&
		       segments.size() <= limits.maximumBranchSegments() &&
		       buds.size() <= limits.maximumBuds() &&
		       attachments.size() <= limits.maximumOrganAttachments() &&
		       growth_tips.size() <= limits.maximumGrowthTips();
	}
};

} // namespace

RuleBranchingGenerationResult RuleBranchingGenerationService::generate(
	const RuleBranchingGenerationRequest &request) const
{
	if (!std::isfinite(request.age()) || request.age() < 0.0f ||
	    !valid_species(request.species())) {
		return RuleBranchingGenerationResult::failed(
			"Rule branching requires a valid species and non-negative finite age.");
	}
	const PlantBranchingSpecification &branching = request.species().branching();
	const float maturity = std::max(
		0.05f, std::min(1.0f, request.age() / request.species().matureAge()));
	GenerationState state{request, complexity_limits_};
	const std::string root_identifier = state.nextNodeIdentifier(0);
	state.nodes.emplace_back(
		root_identifier, glm::vec3(0.0f), branching.baseRadius() * maturity,
		0.0f, 0, BranchNodeState::Mature);

	std::function<bool(
		const std::string &, const glm::vec3 &, const glm::vec3 &, float,
		float, float, int, int, const std::string &)> grow_axis;
	grow_axis = [&](const std::string &start_identifier,
	                const glm::vec3 &start_position,
	                const glm::vec3 &initial_direction,
	                float axis_length,
	                float start_radius,
	                float target_tip_radius,
	                int branch_order,
	                int internode_count,
	                const std::string &scope) {
		if (internode_count < 1) return false;
		glm::vec3 direction = glm::normalize(initial_direction);
		glm::vec3 current_position = start_position;
		std::string current_identifier = start_identifier;
		float current_radius = start_radius;
		std::vector<glm::vec3> path_positions{start_position};
		std::vector<std::string> path_node_identifiers{start_identifier};
		const float base_internode_length = axis_length /
			static_cast<float>(internode_count);

		for (int internode_index = 0;
		     internode_index < internode_count;
		     ++internode_index) {
			const bool can_branch =
				branch_order < branching.maximumBranchOrder() &&
				branching.lateralBranchesPerNode() > 0 && internode_index > 0 &&
				internode_index % branching.branchEveryNInternodes() == 0;
			const float progress = static_cast<float>(internode_index + 1) /
			                       static_cast<float>(internode_count);
			const float nominal_radius = glm::mix(
				start_radius, target_tip_radius, progress);
			float continuation_radius = nominal_radius;
			std::vector<float> lateral_radii;
			if (can_branch) {
				continuation_radius = std::min(
					nominal_radius, current_radius * branching.continuationRadiusRatio());
				const float parent_measure = std::pow(
					current_radius, branching.radiusConservationGamma());
				const float continuation_measure = std::pow(
					continuation_radius, branching.radiusConservationGamma());
				const float remaining_measure = std::max(
					parent_measure - continuation_measure, 1.0e-12f);
				const float lateral_measure = remaining_measure /
					static_cast<float>(branching.lateralBranchesPerNode());
				const float lateral_radius = std::pow(
					lateral_measure, 1.0f / branching.radiusConservationGamma());
				lateral_radii.assign(
					static_cast<std::size_t>(branching.lateralBranchesPerNode()),
					lateral_radius);
			}

			const float length_variation = 1.0f +
				branching.lengthVariationFraction() * state.variation.sampleSigned(
					request.deterministicSeed(),
					scope + ":internode:" + std::to_string(internode_index));
			const float internode_length = base_internode_length * length_variation;
			std::vector<TropismInfluence> tropism_influences =
				request.tropismInfluences();
			tropism_influences.emplace_back(
				TropismType::Up, glm::vec3(0.0f, 1.0f, 0.0f),
				branching.upwardTropismWeight());
			const TropismResolution tropism = TropismDirectionService().resolve(
				direction, tropism_influences);
			if (!tropism.succeeded()) return false;
			direction = tropism.direction();
			const glm::vec3 next_position =
				current_position + direction * internode_length;
			const std::string next_identifier =
				state.nextNodeIdentifier(branch_order);
			state.nodes.emplace_back(
				next_identifier, next_position, continuation_radius,
				static_cast<float>(state.nodes.size()), branch_order,
				BranchNodeState::Active);
			state.segments.emplace_back(
				state.nextSegmentIdentifier(branch_order), current_identifier,
				next_identifier, BranchSegmentKind::Continuation);
			path_positions.push_back(next_position);
			path_node_identifiers.push_back(next_identifier);

			if (can_branch) {
				const glm::vec3 first_radial = radial_axis(direction);
				const glm::vec3 second_radial = glm::normalize(
					glm::cross(direction, first_radial));
				for (int branch_index = 0;
				     branch_index < branching.lateralBranchesPerNode();
				     ++branch_index) {
					const std::string branch_scope =
						scope + ":lateral:" + std::to_string(internode_index) + ":" +
						std::to_string(branch_index);
					const float azimuth_degrees =
						branching.azimuthDivergenceDegrees() *
						static_cast<float>(internode_index + branch_index) +
						branching.directionVariationDegrees() *
						state.variation.sampleSigned(
							request.deterministicSeed(), branch_scope + ":azimuth");
					const float branch_angle_degrees =
						branching.branchAngleDegrees() +
						branching.directionVariationDegrees() *
						state.variation.sampleSigned(
							request.deterministicSeed(), branch_scope + ":angle");
					const float azimuth = glm::radians(azimuth_degrees);
					const float branch_angle = glm::radians(branch_angle_degrees);
					const glm::vec3 radial_direction =
						first_radial * std::cos(azimuth) +
						second_radial * std::sin(azimuth);
					const glm::vec3 child_direction = glm::normalize(
						direction * std::cos(branch_angle) +
						radial_direction * std::sin(branch_angle));
					const float child_length =
						axis_length * branching.branchLengthFraction() *
						std::pow(
							branching.branchLengthFalloff(),
							static_cast<float>(branch_order)) *
						(1.0f + branching.lengthVariationFraction() *
							state.variation.sampleSigned(
								request.deterministicSeed(), branch_scope + ":length"));
					const float child_radius =
						lateral_radii[static_cast<std::size_t>(branch_index)];
					const float first_step = child_length /
						static_cast<float>(std::max(2, internode_count - 1));
					const glm::vec3 child_position =
						current_position + child_direction * first_step;
					const std::string child_identifier =
						state.nextNodeIdentifier(branch_order + 1);
					state.nodes.emplace_back(
						child_identifier, child_position, child_radius,
						static_cast<float>(state.nodes.size()), branch_order + 1,
						BranchNodeState::Active);
					state.segments.emplace_back(
						state.nextSegmentIdentifier(branch_order + 1),
						current_identifier, child_identifier,
						BranchSegmentKind::LateralBranch);
					if (!grow_axis(
							child_identifier, child_position, child_direction,
							child_length - first_step, child_radius,
							std::max(child_radius * 0.18f, 0.0005f),
							branch_order + 1, std::max(1, internode_count - 2),
							branch_scope)) {
						return false;
					}
				}
			}

			current_position = next_position;
			current_identifier = next_identifier;
			current_radius = continuation_radius;
			if (!state.withinLimits()) return false;
		}

		state.growth_tips.emplace_back(
			"tip_" + current_identifier, current_identifier, direction,
			request.developmentState() != PlantDevelopmentState::Dormant &&
			request.developmentState() != PlantDevelopmentState::Dead);
		state.buds.emplace_back(
			"bud_" + current_identifier, current_identifier, direction,
			request.age(), VegetationBudState::LeafBud,
			VegetationOrganType::Leaf, 1.0f);
		if (branch_order >= request.species().leaf().minimumBranchOrder()) {
			const PhyllotaxisSpecification &phyllotaxis =
				request.species().leaf().phyllotaxis();
			float path_length = 0.0f;
			for (std::size_t index = 1u; index < path_positions.size(); ++index) {
				path_length += glm::length(path_positions[index] - path_positions[index - 1u]);
			}
			std::size_t placement_node_count = path_positions.size();
			if (phyllotaxis.mode() == PhyllotaxisMode::Rosette) {
				placement_node_count = static_cast<std::size_t>(
					phyllotaxis.organsPerNode());
			}
			else if (phyllotaxis.internodeLength() > 1.0e-6f) {
				placement_node_count = static_cast<std::size_t>(
					std::floor(path_length / phyllotaxis.internodeLength())) + 1u;
			}
			std::string placement_diagnostic;
			const std::vector<VegetationOrganPlacement> placements =
				OrganPlacementService().placeAlongPath(
					scope + ":leaf", path_positions, placement_node_count,
					phyllotaxis, &placement_diagnostic);
			if (!placement_diagnostic.empty()) {
				state.failure_diagnostic = placement_diagnostic;
				return false;
			}
			if (state.attachments.size() + placements.size() >
			    state.limits.maximumOrganAttachments()) {
				state.failure_diagnostic =
					"Rule branching organ placement exceeded the attachment safety limit.";
				return false;
			}
			for (const VegetationOrganPlacement &placement : placements) {
				std::size_t nearest_node_index = 0u;
				float nearest_distance = std::numeric_limits<float>::max();
				for (std::size_t node_index = 0u;
				     node_index < path_positions.size(); ++node_index) {
					const float distance = glm::length(
						placement.position() - path_positions[node_index]);
					if (distance < nearest_distance) {
						nearest_distance = distance;
						nearest_node_index = node_index;
					}
				}
				state.attachments.emplace_back(
					placement.identifier(),
					path_node_identifiers[nearest_node_index],
					VegetationOrganType::Leaf, placement.localTransform(),
					request.age(), request.species().identifier() + ":leaf",
					"node_surface", "petiole_base");
			}
		}
		return state.withinLimits();
	};

	if (!grow_axis(
			root_identifier, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
			branching.primaryAxisLength() * maturity,
			branching.baseRadius() * maturity,
			std::max(branching.tipRadius() * maturity, 0.0005f), 0,
			branching.primaryInternodeCount(), request.species().identifier())) {
		return RuleBranchingGenerationResult::failed(
			state.failure_diagnostic.empty()
				? "Rule branching exceeded safety limits or could not resolve growth direction."
				: state.failure_diagnostic);
	}

	BranchGraph graph(
		root_identifier, std::move(state.nodes), std::move(state.segments),
		std::move(state.buds), std::move(state.attachments),
		std::move(state.growth_tips));
	const BranchGraphValidationReport validation =
		BranchGraphValidationService(complexity_limits_).validate(graph);
	if (!validation.succeeded()) {
		return RuleBranchingGenerationResult::failed(
			validation.issues().front().diagnostic());
	}
	return RuleBranchingGenerationResult::succeeded(
		graph, BranchGraphDeterministicHashService().hash(graph));
}
