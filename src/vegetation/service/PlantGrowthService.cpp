#include "vegetation/service/PlantGrowthService.h"

#include "vegetation/service/BranchGraphDeterministicHashService.h"
#include "vegetation/service/BranchGraphValidationService.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

bool valid_development_state(PlantDevelopmentState state)
{
	const int value = static_cast<int>(state);
	return value >= static_cast<int>(PlantDevelopmentState::Seed) &&
	       value <= static_cast<int>(PlantDevelopmentState::Dead);
}

bool valid_growth_curve(GrowthCurve curve)
{
	const int value = static_cast<int>(curve);
	return value >= static_cast<int>(GrowthCurve::Linear) &&
	       value <= static_cast<int>(GrowthCurve::EaseOut);
}

bool valid_growth_channel(const PlantGrowthChannelSpecification &channel)
{
	return std::isfinite(channel.initialFactor()) &&
	       channel.initialFactor() > 0.0f &&
	       channel.initialFactor() <= 1.0f &&
	       valid_growth_curve(channel.curve());
}

float evaluate_curve(GrowthCurve curve, float progress)
{
	switch (curve) {
	case GrowthCurve::Linear:
		return progress;
	case GrowthCurve::SmoothStep:
		return progress * progress * (3.0f - 2.0f * progress);
	case GrowthCurve::EaseIn:
		return progress * progress;
	case GrowthCurve::EaseOut: {
		const float inverse = 1.0f - progress;
		return 1.0f - inverse * inverse;
	}
	}
	return progress;
}

float evaluate_factor(
	const PlantGrowthChannelSpecification &channel,
	float progress)
{
	const float curved_progress = evaluate_curve(channel.curve(), progress);
	return channel.initialFactor() +
	       (1.0f - channel.initialFactor()) * curved_progress;
}

BranchNodeState resolved_node_state(
	BranchNodeState source_state,
	PlantDevelopmentState plant_state)
{
	if (source_state == BranchNodeState::Dead ||
	    plant_state == PlantDevelopmentState::Dead) {
		return BranchNodeState::Dead;
	}
	switch (plant_state) {
	case PlantDevelopmentState::Seed:
	case PlantDevelopmentState::Bud:
	case PlantDevelopmentState::Dormant:
		return BranchNodeState::Dormant;
	case PlantDevelopmentState::Shoot:
	case PlantDevelopmentState::Juvenile:
		return BranchNodeState::Active;
	case PlantDevelopmentState::Mature:
	case PlantDevelopmentState::Flowering:
	case PlantDevelopmentState::Fruiting:
		return BranchNodeState::Mature;
	case PlantDevelopmentState::Senescent:
		return BranchNodeState::Senescent;
	case PlantDevelopmentState::Dead:
		return BranchNodeState::Dead;
	}
	return BranchNodeState::Dormant;
}

VegetationBudState resolved_bud_state(
	const VegetationBud &bud,
	PlantDevelopmentState plant_state)
{
	if (bud.state() == VegetationBudState::Dead ||
	    plant_state == PlantDevelopmentState::Dead) {
		return VegetationBudState::Dead;
	}
	if (plant_state == PlantDevelopmentState::Seed ||
	    plant_state == PlantDevelopmentState::Bud ||
	    plant_state == PlantDevelopmentState::Dormant ||
	    plant_state == PlantDevelopmentState::Senescent) {
		return VegetationBudState::Dormant;
	}
	if (plant_state == PlantDevelopmentState::Flowering &&
	    (bud.organType() == VegetationOrganType::Petal ||
	     bud.organType() == VegetationOrganType::Flower)) {
		return VegetationBudState::FlowerBud;
	}
	return bud.state() == VegetationBudState::Dormant
		? VegetationBudState::Active
		: bud.state();
}

bool growth_tip_is_active(
	const VegetationGrowthTip &growth_tip,
	PlantDevelopmentState plant_state)
{
	if (!growth_tip.isActive()) return false;
	return plant_state != PlantDevelopmentState::Seed &&
	       plant_state != PlantDevelopmentState::Bud &&
	       plant_state != PlantDevelopmentState::Dormant &&
	       plant_state != PlantDevelopmentState::Senescent &&
	       plant_state != PlantDevelopmentState::Dead;
}

glm::mat4 grown_organ_transform(
	const OrganAttachment &attachment,
	const glm::vec3 &source_host_position,
	const glm::vec3 &grown_host_position,
	float length_factor,
	float organ_scale_factor)
{
	glm::mat4 transform = attachment.localTransform();
	for (int column = 0; column < 3; ++column) {
		for (int row = 0; row < 3; ++row) {
			transform[column][row] *= organ_scale_factor;
		}
	}
	const glm::vec3 source_origin(attachment.localTransform()[3]);
	const glm::vec3 grown_origin = grown_host_position +
		(source_origin - source_host_position) * length_factor;
	transform[3][0] = grown_origin.x;
	transform[3][1] = grown_origin.y;
	transform[3][2] = grown_origin.z;
	return transform;
}

void hash_bytes(std::uint64_t *hash, const void *data, std::size_t size)
{
	const auto *bytes = static_cast<const unsigned char *>(data);
	for (std::size_t index = 0; index < size; ++index) {
		*hash ^= bytes[index];
		*hash *= 1099511628211ull;
	}
}

std::uint64_t evidence_hash(
	std::uint64_t source_graph_hash,
	std::uint64_t resolved_graph_hash,
	float evaluation_time,
	float progress,
	float length_factor,
	float radius_factor,
	float organ_scale_factor,
	PlantDevelopmentState development_state)
{
	std::uint64_t value = 1469598103934665603ull;
	hash_bytes(&value, &source_graph_hash, sizeof(source_graph_hash));
	hash_bytes(&value, &resolved_graph_hash, sizeof(resolved_graph_hash));
	hash_bytes(&value, &evaluation_time, sizeof(evaluation_time));
	hash_bytes(&value, &progress, sizeof(progress));
	hash_bytes(&value, &length_factor, sizeof(length_factor));
	hash_bytes(&value, &radius_factor, sizeof(radius_factor));
	hash_bytes(&value, &organ_scale_factor, sizeof(organ_scale_factor));
	const int state = static_cast<int>(development_state);
	hash_bytes(&value, &state, sizeof(state));
	return value;
}

} // namespace

PlantGrowthResult PlantGrowthService::resolve(
	const PlantGrowthRequest &request) const
{
	const PlantGrowthSpecification &specification = request.specification();
	if (!std::isfinite(request.evaluationTime()) ||
	    !valid_development_state(request.developmentState()) ||
	    !std::isfinite(specification.startTime()) ||
	    !std::isfinite(specification.duration()) ||
	    specification.duration() <= 0.0f ||
	    !valid_growth_channel(specification.lengthGrowth()) ||
	    !valid_growth_channel(specification.radiusGrowth()) ||
	    !valid_growth_channel(specification.organGrowth())) {
		return PlantGrowthResult::failed(
			"Plant growth requires finite time values, positive duration, valid "
			"development state, and channel factors in the range (0, 1].");
	}

	const BranchGraphValidationService validation_service(complexity_limits_);
	const BranchGraphValidationReport source_validation =
		validation_service.validate(request.sourceGraph());
	if (!source_validation.succeeded()) {
		return PlantGrowthResult::failed(
			"Plant growth source graph is invalid: " +
			source_validation.issues().front().diagnostic());
	}

	std::string traversal_diagnostic;
	const std::vector<std::string> traversal =
		validation_service.deterministicTraversal(
			request.sourceGraph(), &traversal_diagnostic);
	if (!traversal_diagnostic.empty()) {
		return PlantGrowthResult::failed(traversal_diagnostic);
	}

	const float progress = std::clamp(
		(request.evaluationTime() - specification.startTime()) /
			specification.duration(),
		0.0f,
		1.0f);
	const float length_factor =
		evaluate_factor(specification.lengthGrowth(), progress);
	const float radius_factor =
		evaluate_factor(specification.radiusGrowth(), progress);
	const float organ_scale_factor =
		evaluate_factor(specification.organGrowth(), progress);

	std::unordered_map<std::string, const BranchNode *> source_nodes;
	std::unordered_map<std::string, std::string> parent_identifiers;
	for (const BranchNode &node : request.sourceGraph().nodes()) {
		source_nodes.emplace(node.identifier(), &node);
	}
	for (const BranchSegment &segment : request.sourceGraph().segments()) {
		parent_identifiers.emplace(
			segment.childNodeIdentifier(), segment.parentNodeIdentifier());
	}

	std::unordered_map<std::string, glm::vec3> grown_positions;
	for (const std::string &identifier : traversal) {
		const BranchNode &source_node = *source_nodes.at(identifier);
		const auto parent = parent_identifiers.find(identifier);
		if (parent == parent_identifiers.end()) {
			grown_positions.emplace(identifier, source_node.position());
			continue;
		}
		const BranchNode &source_parent = *source_nodes.at(parent->second);
		const glm::vec3 grown_parent = grown_positions.at(parent->second);
		grown_positions.emplace(
			identifier,
			grown_parent +
				(source_node.position() - source_parent.position()) *
					length_factor);
	}

	std::vector<BranchNode> nodes;
	nodes.reserve(request.sourceGraph().nodes().size());
	for (const BranchNode &source_node : request.sourceGraph().nodes()) {
		nodes.emplace_back(
			source_node.identifier(),
			grown_positions.at(source_node.identifier()),
			source_node.radius() * radius_factor,
			source_node.developmentalAge() * progress,
			source_node.branchOrder(),
			resolved_node_state(
				source_node.state(), request.developmentState()));
	}

	std::vector<VegetationBud> buds;
	buds.reserve(request.sourceGraph().buds().size());
	for (const VegetationBud &source_bud : request.sourceGraph().buds()) {
		buds.emplace_back(
			source_bud.identifier(), source_bud.hostNodeIdentifier(),
			source_bud.direction(), source_bud.developmentalAge() * progress,
			resolved_bud_state(source_bud, request.developmentState()),
			source_bud.organType(),
			source_bud.activationProbability() * progress);
	}

	std::vector<OrganAttachment> attachments;
	attachments.reserve(request.sourceGraph().organAttachments().size());
	for (const OrganAttachment &source_attachment :
	     request.sourceGraph().organAttachments()) {
		const BranchNode &source_host =
			*source_nodes.at(source_attachment.hostNodeIdentifier());
		attachments.emplace_back(
			source_attachment.identifier(),
			source_attachment.hostNodeIdentifier(),
			source_attachment.organType(),
			grown_organ_transform(
				source_attachment, source_host.position(),
				grown_positions.at(source_attachment.hostNodeIdentifier()),
				length_factor, organ_scale_factor),
			source_attachment.developmentalAge() * progress,
			source_attachment.shapeIdentifier());
	}

	std::vector<VegetationGrowthTip> growth_tips;
	growth_tips.reserve(request.sourceGraph().growthTips().size());
	for (const VegetationGrowthTip &source_growth_tip :
	     request.sourceGraph().growthTips()) {
		growth_tips.emplace_back(
			source_growth_tip.identifier(),
			source_growth_tip.hostNodeIdentifier(),
			source_growth_tip.direction(),
			growth_tip_is_active(
				source_growth_tip, request.developmentState()));
	}

	BranchGraph resolved_graph(
		request.sourceGraph().rootNodeIdentifier(), std::move(nodes),
		request.sourceGraph().segments(), std::move(buds),
		std::move(attachments), std::move(growth_tips));
	const BranchGraphValidationReport resolved_validation =
		validation_service.validate(resolved_graph);
	if (!resolved_validation.succeeded()) {
		return PlantGrowthResult::failed(
			"Plant growth produced an invalid graph: " +
			resolved_validation.issues().front().diagnostic());
	}

	const BranchGraphDeterministicHashService hash_service;
	const std::uint64_t source_graph_hash =
		hash_service.hash(request.sourceGraph());
	const std::uint64_t resolved_graph_hash = hash_service.hash(resolved_graph);
	const std::uint64_t resolution_evidence_hash = evidence_hash(
		source_graph_hash, resolved_graph_hash, request.evaluationTime(),
		progress, length_factor, radius_factor, organ_scale_factor,
		request.developmentState());
	return PlantGrowthResult::succeeded(PlantGrowthSnapshot(
		std::move(resolved_graph),
		PlantGrowthResolutionEvidence(
			source_graph_hash, resolved_graph_hash, resolution_evidence_hash,
			request.evaluationTime(), progress, length_factor, radius_factor,
			organ_scale_factor, request.developmentState())));
}
