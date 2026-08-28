#include "vegetation/service/VineMeshGenerator.h"

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/OrganAttachment.h"
#include "vegetation/model/VegetationGrowthTip.h"
#include "vegetation/model/VineGrowthRequest.h"
#include "vegetation/model/VineShapeSpecification.h"
#include "vegetation/service/BranchGraphValidationService.h"
#include "vegetation/service/OrganPlacementService.h"
#include "vegetation/service/VegetationGeometryAssemblyService.h"
#include "vegetation/service/VineGrowthService.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

GeneratedPrimitiveMesh failed_mesh(const std::string &message, std::string *diagnostic)
{
	if (diagnostic != nullptr) *diagnostic = message;
	return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
}

BranchGraph seed_graph(const VineShapeSpecification &vine)
{
	return BranchGraph(
		"vine_root",
		{BranchNode(
			"vine_root", vine.startPosition(), vine.initialRadius(), 0.0f, 0,
			BranchNodeState::Active)},
		{}, {}, {},
		{VegetationGrowthTip(
			"vine_growth_tip", "vine_root", vine.initialDirection(), true)});
}

BranchGraph attach_leaves(
	const BranchGraph &graph,
	const PlantSpeciesSpecification &species,
	std::string *diagnostic)
{
	std::vector<glm::vec3> path_positions;
	std::vector<std::string> path_node_identifiers;
	path_positions.reserve(graph.nodes().size());
	path_node_identifiers.reserve(graph.nodes().size());
	for (const BranchNode &node : graph.nodes()) {
		path_positions.push_back(node.position());
		path_node_identifiers.push_back(node.identifier());
	}
	if (path_positions.size() < 2u) {
		if (diagnostic != nullptr) {
			*diagnostic = "Vine growth must produce at least one visible segment.";
		}
		return graph;
	}
	float path_length = 0.0f;
	for (std::size_t index = 1u; index < path_positions.size(); ++index) {
		path_length += glm::length(path_positions[index] - path_positions[index - 1u]);
	}
	const PhyllotaxisSpecification &phyllotaxis =
		species.leaf().phyllotaxis();
	std::size_t placement_node_count = path_positions.size();
	if (phyllotaxis.internodeLength() > 1.0e-6f) {
		placement_node_count = static_cast<std::size_t>(
			std::floor(path_length / phyllotaxis.internodeLength())) + 1u;
	}
	std::string placement_diagnostic;
	const std::vector<VegetationOrganPlacement> placements =
		OrganPlacementService().placeAlongPath(
			"vine_leaf", path_positions, placement_node_count,
			phyllotaxis, &placement_diagnostic);
	if (!placement_diagnostic.empty()) {
		if (diagnostic != nullptr) *diagnostic = placement_diagnostic;
		return graph;
	}
	std::vector<OrganAttachment> attachments = graph.organAttachments();
	attachments.reserve(attachments.size() + placements.size());
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
		attachments.emplace_back(
			placement.identifier(), path_node_identifiers[nearest_node_index],
			VegetationOrganType::Leaf, placement.localTransform(), 1.0f,
			species.identifier() + ":leaf");
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return BranchGraph(
		graph.rootNodeIdentifier(), graph.nodes(), graph.segments(), graph.buds(),
		std::move(attachments), graph.growthTips());
}

} // namespace

GeneratedPrimitiveMesh VineMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const auto *vine = dynamic_cast<const VineShapeSpecification *>(&specification);
	if (vine == nullptr) {
		return failed_mesh(
			"Vine mesh generation requires a VineShapeSpecification.", diagnostic);
	}
	const VineGrowthResult growth = VineGrowthService().resolve(
		VineGrowthRequest(
			seed_graph(*vine), vine->growth(), vine->target(), vine->obstacles()));
	if (!growth.succeeded() || !growth.snapshot().has_value()) {
		return failed_mesh(growth.diagnostic(), diagnostic);
	}
	std::string attachment_diagnostic;
	BranchGraph graph = attach_leaves(
		growth.snapshot()->path().graph(), vine->species(),
		&attachment_diagnostic);
	if (!attachment_diagnostic.empty()) {
		return failed_mesh(attachment_diagnostic, diagnostic);
	}
	const BranchGraphValidationReport validation =
		BranchGraphValidationService().validate(graph);
	if (!validation.succeeded()) {
		return failed_mesh(
			"Vine organ placement produced an invalid graph: " +
			validation.issues().front().diagnostic(), diagnostic);
	}
	const VegetationGeometryBuildResult geometry =
		VegetationGeometryAssemblyService().build(
			graph, vine->species(), vine->detailLevel());
	if (!geometry.succeeded() || !geometry.geometry().has_value()) {
		return failed_mesh(geometry.diagnostic(), diagnostic);
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return geometry.geometry()->combinedPreviewMesh();
}
