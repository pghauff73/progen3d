#include "vegetation/service/VegetationGeometryAssemblyService.h"

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/InstanceArrayShapeSpecification.h"
#include "geometry/service/BotanicalBladeMeshGenerator.h"
#include "geometry/service/BotanicalBladeSpecificationValidator.h"
#include "geometry/service/BranchJunctionMeshGenerator.h"
#include "geometry/service/BranchJunctionSpecificationValidator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/ShapeSpecificationValidator.h"
#include "geometry/service/SphereMeshGenerator.h"
#include "geometry/service/TaperedSweepMeshGenerator.h"
#include "geometry/service/TaperedSweepSpecificationValidator.h"
#include "vegetation/model/BranchGraph.h"
#include "vegetation/service/BranchGraphValidationService.h"
#include "vegetation/service/CompoundLeafPlacementService.h"
#include "vegetation/service/FlowerHeadPlacementService.h"
#include "vegetation/service/InflorescencePlacementService.h"
#include "vegetation/service/OrganArrayPlacementService.h"
#include "vegetation/service/OrganPlacementService.h"
#include "vegetation/service/PlantOrganGeometryFactory.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

int maximum_branch_order_for(GeometryDetailLevel detail_level)
{
	switch (detail_level) {
	case GeometryDetailLevel::Bounds:
		return -1;
	case GeometryDetailLevel::CoarseShape:
		return 0;
	case GeometryDetailLevel::Assembly:
		return 1;
	case GeometryDetailLevel::Component:
	case GeometryDetailLevel::ConstructionDetail:
	case GeometryDetailLevel::FastenersAndSeals:
		return 1000000;
	}
	return 0;
}

int radial_segments_for(GeometryDetailLevel detail_level)
{
	switch (detail_level) {
	case GeometryDetailLevel::Bounds:
	case GeometryDetailLevel::CoarseShape:
		return 6;
	case GeometryDetailLevel::Assembly:
		return 8;
	case GeometryDetailLevel::Component:
		return 10;
	case GeometryDetailLevel::ConstructionDetail:
		return 12;
	case GeometryDetailLevel::FastenersAndSeals:
		return 16;
	}
	return 8;
}

const BranchNode *find_node(
	const std::unordered_map<std::string, const BranchNode *> &nodes,
	const std::string &identifier)
{
	const auto found = nodes.find(identifier);
	return found == nodes.end() ? nullptr : found->second;
}

std::shared_ptr<const SphereShapeSpecification> create_sphere_shape(
	GeometryDetailLevel detail_level,
	std::string *diagnostic)
{
	SphereShapeSpecificationCandidate candidate;
	candidate.azimuth_segments = radial_segments_for(detail_level) * 2;
	candidate.polar_segments = radial_segments_for(detail_level);
	return ShapeSpecificationValidator().validateSphere(
		std::move(candidate), diagnostic);
}

GeneratedPrimitiveMesh retagged_petals(
	const GeneratedPrimitiveMesh &source)
{
	std::vector<MeshSurfaceTag> tags;
	tags.reserve(source.faceSurfaceTags().size());
	for (const MeshSurfaceTag &tag : source.faceSurfaceTags()) {
		MeshSurfaceRole role = MeshSurfaceRole::BotanicalPetalEdge;
		if (tag.role() == MeshSurfaceRole::BotanicalBladeUpper) {
			role = MeshSurfaceRole::BotanicalPetalUpper;
		}
		else if (tag.role() == MeshSurfaceRole::BotanicalBladeLower) {
			role = MeshSurfaceRole::BotanicalPetalLower;
		}
		tags.emplace_back(role, tag.boundaryIndex(), tag.objectPartIndex());
	}
	return GeneratedPrimitiveMesh(source.mesh(), std::move(tags));
}

struct OrganHostPath
{
	std::string identifier;
	int branch_order = 0;
	std::vector<glm::vec3> points;
};

float path_length(const std::vector<glm::vec3> &points)
{
	float length = 0.0f;
	for (std::size_t index = 1u; index < points.size(); ++index) {
		length += glm::length(points[index] - points[index - 1u]);
	}
	return length;
}

std::vector<OrganHostPath> organ_host_paths(
	const BranchGraph &graph,
	const std::unordered_map<std::string, const BranchNode *> &nodes)
{
	std::unordered_map<std::string, std::vector<const BranchSegment *>> outgoing;
	for (const BranchSegment &segment : graph.segments()) {
		outgoing[segment.parentNodeIdentifier()].push_back(&segment);
	}
	for (auto &entry : outgoing) {
		std::sort(
			entry.second.begin(), entry.second.end(),
			[](const BranchSegment *first, const BranchSegment *second) {
				return first->identifier() < second->identifier();
			});
	}

	std::vector<const BranchSegment *> starts;
	for (const BranchSegment &segment : graph.segments()) {
		const BranchNode *child = find_node(nodes, segment.childNodeIdentifier());
		if (child == nullptr) continue;
		if (segment.kind() == BranchSegmentKind::LateralBranch ||
		    (segment.parentNodeIdentifier() == graph.rootNodeIdentifier() &&
		     child->branchOrder() == 0)) {
			starts.push_back(&segment);
		}
	}
	std::sort(
		starts.begin(), starts.end(),
		[](const BranchSegment *first, const BranchSegment *second) {
			return first->identifier() < second->identifier();
		});

	std::vector<OrganHostPath> paths;
	for (const BranchSegment *start : starts) {
		const BranchNode *parent = find_node(nodes, start->parentNodeIdentifier());
		const BranchNode *child = find_node(nodes, start->childNodeIdentifier());
		if (parent == nullptr || child == nullptr) continue;
		OrganHostPath path;
		path.identifier = start->identifier();
		path.branch_order = child->branchOrder();
		path.points = {parent->position(), child->position()};
		std::string current_identifier = child->identifier();
		while (true) {
			const auto found = outgoing.find(current_identifier);
			if (found == outgoing.end()) break;
			const BranchSegment *continuation = nullptr;
			for (const BranchSegment *candidate : found->second) {
				const BranchNode *next = find_node(
					nodes, candidate->childNodeIdentifier());
				if (candidate->kind() == BranchSegmentKind::Continuation && next != nullptr &&
				    next->branchOrder() == path.branch_order) {
					continuation = candidate;
					break;
				}
			}
			if (continuation == nullptr) break;
			const BranchNode *next = find_node(
				nodes, continuation->childNodeIdentifier());
			if (next == nullptr) break;
			path.points.push_back(next->position());
			current_identifier = next->identifier();
		}
		paths.push_back(std::move(path));
	}
	return paths;
}

PlantOrganArraySpecification identified_organ_array(
	const PlantOrganArraySpecification &source,
	const std::string &suffix)
{
	return PlantOrganArraySpecification(
		source.identifier() + ":" + suffix, source.organType(), source.host(),
		source.count(), source.spacing(), source.azimuthProgressionDegrees(),
		source.initialScale(), source.scaleFalloff(), source.orientation(),
		source.jitterFraction(), source.minimumBranchOrder());
}

bool organ_array_is_visible(
	const PlantOrganArraySpecification &organ_array,
	float plant_age,
	PlantDevelopmentState development_state,
	const PlantSpeciesSpecification &species)
{
	if (development_state == PlantDevelopmentState::Seed ||
	    development_state == PlantDevelopmentState::Dead) {
		return false;
	}
	switch (organ_array.organType()) {
	case VegetationOrganType::Bud:
		return development_state != PlantDevelopmentState::Fruiting &&
		       development_state != PlantDevelopmentState::Senescent;
	case VegetationOrganType::Leaf:
		return development_state != PlantDevelopmentState::Bud;
	case VegetationOrganType::Petal:
	case VegetationOrganType::Flower:
		return species.flower().isEnabled() && plant_age >= species.floweringAge() &&
		       (development_state == PlantDevelopmentState::Flowering ||
		        development_state == PlantDevelopmentState::Fruiting ||
		        development_state == PlantDevelopmentState::Mature);
	case VegetationOrganType::Fruit:
		return development_state == PlantDevelopmentState::Fruiting;
	case VegetationOrganType::Thorn:
		return development_state != PlantDevelopmentState::Bud;
	case VegetationOrganType::Branch:
	case VegetationOrganType::Petiole:
	case VegetationOrganType::Tendril:
		return false;
	}
	return false;
}

std::string part_identifier_for(const PlantOrganArraySpecification &organ_array)
{
	std::string identifier = "organ_array_" + organ_array.identifier();
	std::replace(identifier.begin(), identifier.end(), ':', '_');
	return identifier;
}

} // namespace

VegetationGeometryBuildResult VegetationGeometryAssemblyService::build(
	const BranchGraph &graph,
	const PlantSpeciesSpecification &species,
	GeometryDetailLevel detail_level) const

{
	return build(
		graph, species, detail_level, species.matureAge(),
		PlantDevelopmentState::Mature, 0u);
}

VegetationGeometryBuildResult VegetationGeometryAssemblyService::build(
	const BranchGraph &graph,
	const PlantSpeciesSpecification &species,
	GeometryDetailLevel detail_level,
	float plant_age,
	PlantDevelopmentState development_state,
	std::uint64_t deterministic_seed) const
{
	const BranchGraphValidationReport graph_validation =
		BranchGraphValidationService().validate(graph);
	if (!graph_validation.succeeded()) {
		return VegetationGeometryBuildResult::failed(
			graph_validation.issues().front().diagnostic());
	}
	std::unordered_map<std::string, const BranchNode *> nodes;
	float maximum_radius = 0.0f;
	glm::vec3 minimum(0.0f);
	glm::vec3 maximum(0.0f);
	bool first_node = true;
	for (const BranchNode &node : graph.nodes()) {
		nodes.emplace(node.identifier(), &node);
		maximum_radius = std::max(maximum_radius, node.radius());
		if (first_node) {
			minimum = maximum = node.position();
			first_node = false;
		}
		else {
			minimum = glm::min(minimum, node.position());
			maximum = glm::max(maximum, node.position());
		}
	}
	minimum -= glm::vec3(maximum_radius);
	maximum += glm::vec3(maximum_radius);

	std::vector<VegetationGeometryPart> parts;
	std::string diagnostic;
	if (detail_level == GeometryDetailLevel::Bounds) {
		auto sphere_shape = create_sphere_shape(detail_level, &diagnostic);
		if (!sphere_shape) return VegetationGeometryBuildResult::failed(diagnostic);
		const GeneratedPrimitiveMesh sphere_mesh =
			SphereMeshGenerator().generate(*sphere_shape, &diagnostic);
		if (!sphere_mesh.mesh() || sphere_mesh.mesh()->faces.empty()) {
			return VegetationGeometryBuildResult::failed(
				diagnostic.empty() ? "Plant bounds sphere generation failed." : diagnostic);
		}
		const glm::vec3 dimensions = glm::max(
			maximum - minimum, glm::vec3(0.001f));
		const glm::vec3 centre = (minimum + maximum) * 0.5f;
		const glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), centre) *
			glm::scale(glm::mat4(1.0f), dimensions);
		parts.emplace_back(
			"plant_bounds", "CrownAndRootBounds", "vegetation_bounds",
			std::move(sphere_shape), sphere_mesh, transform,
			GeometryDetailRange::exact(GeometryDetailLevel::Bounds));
	}
	else {
		const int maximum_branch_order = maximum_branch_order_for(detail_level);
		for (const BranchSegment &segment : graph.segments()) {
			const BranchNode *parent = find_node(nodes, segment.parentNodeIdentifier());
			const BranchNode *child = find_node(nodes, segment.childNodeIdentifier());
			if (parent == nullptr || child == nullptr ||
			    child->branchOrder() > maximum_branch_order) {
				continue;
			}
			TaperedSweepShapeSpecificationCandidate candidate;
			candidate.path_points = {parent->position(), child->position()};
			candidate.radii = {parent->radius(), child->radius()};
			candidate.circumferential_segments = radial_segments_for(detail_level);
			candidate.detail_level = detail_level;
			auto shape = TaperedSweepSpecificationValidator(complexity_limits_).validate(
				std::move(candidate), &diagnostic);
			if (!shape) return VegetationGeometryBuildResult::failed(diagnostic);
			GeometryBuildResult mesh =
				TaperedSweepMeshGenerator(complexity_limits_).build(*shape);
			if (!mesh.succeeded()) {
				return VegetationGeometryBuildResult::failed(mesh.firstDiagnostic());
			}
			parts.emplace_back(
				segment.identifier(),
				segment.kind() == BranchSegmentKind::Continuation
					? "Internode"
					: "LateralBranch",
				"barkroughwood", std::move(shape), mesh.generatedMesh(),
				glm::mat4(1.0f),
				GeometryDetailRange(
					GeometryDetailLevel::CoarseShape,
					GeometryDetailLevel::FastenersAndSeals));
		}

		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			std::unordered_map<std::string, const BranchSegment *> incoming_segments;
			std::unordered_map<std::string, std::vector<const BranchSegment *>>
				outgoing_segments;
			for (const BranchSegment &segment : graph.segments()) {
				incoming_segments.emplace(segment.childNodeIdentifier(), &segment);
				outgoing_segments[segment.parentNodeIdentifier()].push_back(&segment);
			}
			for (const BranchNode &node : graph.nodes()) {
				const auto outgoing_found = outgoing_segments.find(node.identifier());
				const auto incoming_found = incoming_segments.find(node.identifier());
				if (outgoing_found == outgoing_segments.end() ||
				    outgoing_found->second.size() < 2u ||
				    incoming_found == incoming_segments.end() ||
				    node.branchOrder() > maximum_branch_order) {
					continue;
				}
				const BranchSegment *incoming_segment = incoming_found->second;
				const BranchNode *parent_node = find_node(
					nodes, incoming_segment->parentNodeIdentifier());
				if (parent_node == nullptr) continue;

				BranchJunctionShapeSpecificationCandidate junction_candidate;
				junction_candidate.core_radius = node.radius();
				junction_candidate.bulge_scale = 1.15f;
				junction_candidate.circumferential_segments =
					radial_segments_for(detail_level);
				junction_candidate.detail_level = detail_level;
				const glm::vec3 parent_delta = parent_node->position() - node.position();
				junction_candidate.arms.emplace_back(
					BranchJunctionArmRole::Parent, parent_delta, node.radius(),
					std::min(
						glm::length(parent_delta) * 0.45f,
						node.radius() * 2.75f));

				std::vector<const BranchSegment *> child_segments =
					outgoing_found->second;
				std::sort(
					child_segments.begin(), child_segments.end(),
					[](const BranchSegment *first, const BranchSegment *second) {
						return first->identifier() < second->identifier();
					});
				for (const BranchSegment *child_segment : child_segments) {
					const BranchNode *child_node = find_node(
						nodes, child_segment->childNodeIdentifier());
					if (child_node == nullptr ||
					    child_node->branchOrder() > maximum_branch_order) {
						continue;
					}
					const glm::vec3 child_delta =
						child_node->position() - node.position();
					junction_candidate.arms.emplace_back(
						BranchJunctionArmRole::Child, child_delta,
						child_node->radius(),
						std::min(
							glm::length(child_delta) * 0.45f,
							node.radius() * 2.75f));
				}
				if (junction_candidate.arms.size() < 3u) continue;
				auto junction_shape =
					BranchJunctionSpecificationValidator(complexity_limits_).validate(
						std::move(junction_candidate), &diagnostic);
				if (!junction_shape) {
					return VegetationGeometryBuildResult::failed(diagnostic);
				}
				const GeometryBuildResult junction_mesh =
					BranchJunctionMeshGenerator(complexity_limits_).build(
						*junction_shape);
				if (!junction_mesh.succeeded()) {
					return VegetationGeometryBuildResult::failed(
						junction_mesh.firstDiagnostic());
				}
				parts.emplace_back(
					"junction_" + node.identifier(), "BranchJunction",
					"barkroughwood", std::move(junction_shape),
					junction_mesh.generatedMesh(),
					glm::translate(glm::mat4(1.0f), node.position()),
					GeometryDetailRange(
						GeometryDetailLevel::Assembly,
						GeometryDetailLevel::FastenersAndSeals));
			}
		}

		if (geometryDetailLevelRank(detail_level) >=
		        geometryDetailLevelRank(GeometryDetailLevel::Component) &&
		    !graph.organAttachments().empty()) {
			BotanicalBladeShapeSpecificationCandidate leaf_candidate;
			leaf_candidate.kind = BotanicalBladeKind::Leaf;
			leaf_candidate.profile = species.leaf().profile();
			leaf_candidate.length = species.leaf().length();
			leaf_candidate.maximum_width = species.leaf().width();
			leaf_candidate.camber = species.leaf().camber();
			leaf_candidate.twist_degrees = species.leaf().twistDegrees();
			leaf_candidate.thickness =
				geometryDetailLevelRank(detail_level) >=
					geometryDetailLevelRank(GeometryDetailLevel::ConstructionDetail)
				? species.leaf().thickness()
				: 0.0f;
			leaf_candidate.longitudinal_segments =
				detail_level == GeometryDetailLevel::FastenersAndSeals ? 16 : 10;
			leaf_candidate.lateral_segments =
				detail_level == GeometryDetailLevel::FastenersAndSeals ? 6 : 3;
			leaf_candidate.detail_level = detail_level;
			auto leaf_shape =
				BotanicalBladeSpecificationValidator(complexity_limits_).validate(
					std::move(leaf_candidate), &diagnostic);
			if (!leaf_shape) return VegetationGeometryBuildResult::failed(diagnostic);
			GeometryBuildResult leaf_mesh =
				BotanicalBladeMeshGenerator(complexity_limits_).build(*leaf_shape);
			if (!leaf_mesh.succeeded()) {
				return VegetationGeometryBuildResult::failed(
					leaf_mesh.firstDiagnostic());
			}

			const PlantPetioleSpecification &petiole = species.leaf().petiole();
			TaperedSweepShapeSpecificationCandidate petiole_candidate;
			petiole_candidate.path_points = {
				glm::vec3(0.0f), glm::vec3(0.0f, petiole.length(), 0.0f)};
			petiole_candidate.radii = {
				petiole.baseRadius(), petiole.tipRadius()};
			petiole_candidate.circumferential_segments =
				radial_segments_for(detail_level);
			petiole_candidate.detail_level = detail_level;
			auto petiole_shape =
				TaperedSweepSpecificationValidator(complexity_limits_).validate(
					std::move(petiole_candidate), &diagnostic);
			if (!petiole_shape) {
				return VegetationGeometryBuildResult::failed(diagnostic);
			}
			GeometryBuildResult petiole_mesh =
				TaperedSweepMeshGenerator(complexity_limits_).build(*petiole_shape);
			if (!petiole_mesh.succeeded()) {
				return VegetationGeometryBuildResult::failed(
					petiole_mesh.firstDiagnostic());
			}

			std::vector<glm::mat4> petiole_transforms;
			petiole_transforms.reserve(graph.organAttachments().size());
			for (const OrganAttachment &attachment : graph.organAttachments()) {
				if (attachment.organType() == VegetationOrganType::Leaf) {
					petiole_transforms.push_back(attachment.localTransform());
				}
			}

			std::vector<glm::mat4> rachis_transforms;
			std::vector<glm::mat4> leaf_transforms;
			const PlantCompoundLeafSpecification &compound_leaf =
				species.leaf().compoundLeaf();
			if (compound_leaf.isEnabled()) {
				std::string placement_diagnostic;
				const std::optional<CompoundLeafPlacement> compound_placement =
					CompoundLeafPlacementService().place(
						graph.organAttachments(), petiole, compound_leaf,
						&placement_diagnostic);
				if (!compound_placement.has_value()) {
					return VegetationGeometryBuildResult::failed(
						placement_diagnostic);
				}
				rachis_transforms = compound_placement->rachisTransforms();
				leaf_transforms = compound_placement->leafletTransforms();
			}
			else {
				leaf_transforms.reserve(petiole_transforms.size());
				for (const glm::mat4 &petiole_transform : petiole_transforms) {
					leaf_transforms.push_back(
						petiole_transform *
						glm::translate(
							glm::mat4(1.0f),
							glm::vec3(0.0f, petiole.length(), 0.0f)));
				}
			}

			if (!petiole_transforms.empty()) {
				InstanceArraySpecification petiole_array(petiole_transforms);
				GeometryBuildResult petiole_instances =
					InstanceArrayGeometryBuilder(complexity_limits_).build(
						petiole_mesh.generatedMesh(), petiole_array);
				if (!petiole_instances.succeeded()) {
					return VegetationGeometryBuildResult::failed(
						petiole_instances.firstDiagnostic());
				}
				std::ostringstream petiole_canonical;
				petiole_canonical << "LeafPetioleInstanceArray:v1:source="
				                  << petiole_shape->key().canonicalValue() << ":count="
				                  << petiole_transforms.size();
				auto petiole_array_shape =
					std::make_shared<const InstanceArrayShapeSpecification>(
						petiole_shape, std::move(petiole_array),
						ShapeSpecificationKey(petiole_canonical.str()),
						petiole_canonical.str(), detail_level);
				parts.emplace_back(
					"leaf_petioles", "InstancedLeafPetioles",
					"leafstemmattepaint", std::move(petiole_array_shape),
					petiole_instances.generatedMesh(), glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::Component,
						GeometryDetailLevel::FastenersAndSeals));
			}

			if (!rachis_transforms.empty()) {
				TaperedSweepShapeSpecificationCandidate rachis_candidate;
				rachis_candidate.path_points = {
					glm::vec3(0.0f),
					glm::vec3(0.0f, compound_leaf.rachisLength(), 0.0f)};
				rachis_candidate.radii = {
					compound_leaf.rachisBaseRadius(),
					compound_leaf.rachisTipRadius()};
				rachis_candidate.circumferential_segments =
					radial_segments_for(detail_level);
				rachis_candidate.detail_level = detail_level;
				auto rachis_shape =
					TaperedSweepSpecificationValidator(complexity_limits_).validate(
						std::move(rachis_candidate), &diagnostic);
				if (!rachis_shape) {
					return VegetationGeometryBuildResult::failed(diagnostic);
				}
				GeometryBuildResult rachis_mesh =
					TaperedSweepMeshGenerator(complexity_limits_).build(*rachis_shape);
				if (!rachis_mesh.succeeded()) {
					return VegetationGeometryBuildResult::failed(
						rachis_mesh.firstDiagnostic());
				}

				InstanceArraySpecification rachis_array(rachis_transforms);
				GeometryBuildResult rachis_instances =
					InstanceArrayGeometryBuilder(complexity_limits_).build(
						rachis_mesh.generatedMesh(), rachis_array);
				if (!rachis_instances.succeeded()) {
					return VegetationGeometryBuildResult::failed(
						rachis_instances.firstDiagnostic());
				}
				std::ostringstream rachis_canonical;
				rachis_canonical << "CompoundLeafRachisInstanceArray:v1:source="
				                 << rachis_shape->key().canonicalValue() << ":count="
				                 << rachis_transforms.size();
				auto rachis_array_shape =
					std::make_shared<const InstanceArrayShapeSpecification>(
						rachis_shape, std::move(rachis_array),
						ShapeSpecificationKey(rachis_canonical.str()),
						rachis_canonical.str(), detail_level);
				parts.emplace_back(
					"compound_leaf_rachises", "InstancedCompoundLeafRachises",
					"leafstemmattepaint", std::move(rachis_array_shape),
					rachis_instances.generatedMesh(), glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::Component,
						GeometryDetailLevel::FastenersAndSeals));
			}

			if (!leaf_transforms.empty()) {
				InstanceArraySpecification leaf_array(leaf_transforms);
				GeometryBuildResult leaf_instances =
					InstanceArrayGeometryBuilder(complexity_limits_).build(
						leaf_mesh.generatedMesh(), leaf_array);
				if (!leaf_instances.succeeded()) {
					return VegetationGeometryBuildResult::failed(
						leaf_instances.firstDiagnostic());
				}
				std::ostringstream canonical;
				canonical << "LeafInstanceArray:v1:source="
				          << leaf_shape->key().canonicalValue() << ":count="
				          << leaf_transforms.size();
				auto array_shape =
					std::make_shared<const InstanceArrayShapeSpecification>(
						leaf_shape, std::move(leaf_array),
						ShapeSpecificationKey(canonical.str()), canonical.str(),
						detail_level);
				parts.emplace_back(
					"leaf_instances",
					compound_leaf.isEnabled()
						? "InstancedCompoundLeaflets"
						: "InstancedLeafBlades",
					"leafgreenmattepaint", std::move(array_shape),
					leaf_instances.generatedMesh(), glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::Component,
						GeometryDetailLevel::FastenersAndSeals));
			}
		}

		if (geometryDetailLevelRank(detail_level) >=
		        geometryDetailLevelRank(GeometryDetailLevel::Component) &&
		    !species.organArrays().empty()) {
			const std::vector<OrganHostPath> host_paths =
				organ_host_paths(graph, nodes);
			for (const PlantOrganArraySpecification &organ_array :
			     species.organArrays()) {
				if (!organ_array_is_visible(
						organ_array, plant_age, development_state, species)) {
					continue;
				}
				if (organ_array.host() == VegetationOrganArrayHost::Surface) {
					return VegetationGeometryBuildResult::failed(
						"Plant Surface OrganArray requires a resolved SurfaceAttachment context.");
				}
				const std::optional<PlantOrganGeometrySource> source =
					PlantOrganGeometryFactory(complexity_limits_).create(
						species, organ_array.organType(), detail_level, &diagnostic);
				if (!source.has_value()) {
					return VegetationGeometryBuildResult::failed(diagnostic);
				}

				std::vector<glm::mat4> transforms;
				if (organ_array.host() == VegetationOrganArrayHost::Stem ||
				    organ_array.host() == VegetationOrganArrayHost::Branch) {
					const float required_length =
						static_cast<float>(organ_array.count() - 1) *
						organ_array.spacing();
					for (const OrganHostPath &host_path : host_paths) {
						const bool selected =
							organ_array.host() == VegetationOrganArrayHost::Stem
								? host_path.branch_order == 0
								: host_path.branch_order >=
										std::max(1, organ_array.minimumBranchOrder());
						if (!selected ||
						    path_length(host_path.points) + 1.0e-5f < required_length) {
							continue;
						}
						const PlantOrganArraySpecification path_array =
							identified_organ_array(organ_array, host_path.identifier);
						const std::vector<OrganArrayPlacement> placements =
							OrganArrayPlacementService().placeAlongPath(
								path_array, host_path.points, deterministic_seed,
								&diagnostic);
						if (!diagnostic.empty()) {
							return VegetationGeometryBuildResult::failed(diagnostic);
						}
						for (const OrganArrayPlacement &placement : placements) {
							transforms.push_back(placement.localTransform());
						}
					}
				}
				else if (organ_array.host() ==
				         VegetationOrganArrayHost::FlowerHead) {
					for (const VegetationGrowthTip &growth_tip : graph.growthTips()) {
						if (!growth_tip.isActive()) continue;
						const BranchNode *host_node = find_node(
							nodes, growth_tip.hostNodeIdentifier());
						if (host_node == nullptr || host_node->branchOrder() <
							organ_array.minimumBranchOrder()) {
							continue;
						}
						const PlantOrganArraySpecification head_array =
							identified_organ_array(organ_array, growth_tip.identifier());
						const std::vector<OrganArrayPlacement> placements =
							OrganArrayPlacementService().placeAroundFlowerHead(
								head_array, host_node->position(), growth_tip.direction(),
								deterministic_seed, &diagnostic);
						if (!diagnostic.empty()) {
							return VegetationGeometryBuildResult::failed(diagnostic);
						}
						for (const OrganArrayPlacement &placement : placements) {
							transforms.push_back(placement.localTransform());
						}
					}
				}
				if (transforms.empty()) continue;
				if (transforms.size() > complexity_limits_.maximumInstanceArrayCount()) {
					return VegetationGeometryBuildResult::failed(
						"Plant OrganArray exceeded the instance-array safety limit.");
				}

				InstanceArraySpecification instance_array(transforms);
				GeometryBuildResult instances =
					InstanceArrayGeometryBuilder(complexity_limits_).build(
						source->generatedMesh(), instance_array);
				if (!instances.succeeded()) {
					return VegetationGeometryBuildResult::failed(
						instances.firstDiagnostic());
				}
				std::ostringstream canonical;
				canonical << "PlantOrganArray:v1:id=" << organ_array.identifier()
				          << ":organ=" << vegetationOrganTypeName(organ_array.organType())
				          << ":host=" << vegetationOrganArrayHostName(organ_array.host())
				          << ":source=" << source->shape()->key().canonicalValue()
				          << ":count=" << transforms.size();
				auto array_shape =
					std::make_shared<const InstanceArrayShapeSpecification>(
						source->shape(), std::move(instance_array),
						ShapeSpecificationKey(canonical.str()), canonical.str(),
						detail_level);
				parts.emplace_back(
					part_identifier_for(organ_array), source->semanticRole(),
					source->materialIdentifier(), std::move(array_shape),
					instances.generatedMesh(), glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::Component,
						GeometryDetailLevel::FastenersAndSeals));
			}
		}

		const bool flowering_state =
			development_state == PlantDevelopmentState::Flowering ||
			development_state == PlantDevelopmentState::Fruiting ||
			development_state == PlantDevelopmentState::Mature;
		if (geometryDetailLevelRank(detail_level) >=
		        geometryDetailLevelRank(GeometryDetailLevel::ConstructionDetail) &&
		    species.flower().isEnabled() && flowering_state &&
		    plant_age >= species.floweringAge() && !graph.growthTips().empty()) {
			BotanicalBladeShapeSpecificationCandidate petal_candidate;
			petal_candidate.kind = BotanicalBladeKind::Petal;
			petal_candidate.profile = species.flower().petalProfile();
			petal_candidate.length = species.flower().petalLength();
			petal_candidate.maximum_width = species.flower().petalWidth();
			petal_candidate.longitudinal_curvature =
				species.flower().longitudinalCurvature();
			petal_candidate.camber = species.flower().camber();
			petal_candidate.twist_degrees = species.flower().twistDegrees();
			petal_candidate.thickness = species.flower().thickness();
			petal_candidate.width_power = species.flower().widthPower();
			petal_candidate.longitudinal_segments =
				detail_level == GeometryDetailLevel::FastenersAndSeals ? 18 : 12;
			petal_candidate.lateral_segments =
				detail_level == GeometryDetailLevel::FastenersAndSeals ? 6 : 4;
			petal_candidate.detail_level = detail_level;
			auto petal_shape =
				BotanicalBladeSpecificationValidator(complexity_limits_).validate(
					std::move(petal_candidate), &diagnostic);
			if (!petal_shape) return VegetationGeometryBuildResult::failed(diagnostic);
			GeometryBuildResult petal_mesh =
				BotanicalBladeMeshGenerator(complexity_limits_).build(*petal_shape);
			if (!petal_mesh.succeeded()) {
				return VegetationGeometryBuildResult::failed(
					petal_mesh.firstDiagnostic());
			}

			std::vector<glm::mat4> petal_transforms;
			const bool uses_structured_flower_topology =
				species.flower().flowerHead().isEnabled() ||
				species.flower().inflorescence().isEnabled();
			const VegetationGrowthTip *principal_growth_tip = nullptr;
			if (uses_structured_flower_topology) {
				for (const VegetationGrowthTip &candidate_tip : graph.growthTips()) {
					if (!candidate_tip.isActive()) continue;
					const BranchNode *candidate_node = find_node(
						nodes, candidate_tip.hostNodeIdentifier());
					if (candidate_node == nullptr) continue;
					if (principal_growth_tip == nullptr) {
						principal_growth_tip = &candidate_tip;
						continue;
					}
					const BranchNode *principal_node = find_node(
						nodes, principal_growth_tip->hostNodeIdentifier());
					if (principal_node == nullptr ||
					    candidate_node->position().y > principal_node->position().y ||
					    (candidate_node->position().y == principal_node->position().y &&
					     candidate_tip.identifier() < principal_growth_tip->identifier())) {
						principal_growth_tip = &candidate_tip;
					}
				}
			}
			for (const VegetationGrowthTip &growth_tip : graph.growthTips()) {
				if (!growth_tip.isActive()) continue;
				if (uses_structured_flower_topology &&
				    &growth_tip != principal_growth_tip) {
					continue;
				}
				const BranchNode *host_node = find_node(
					nodes, growth_tip.hostNodeIdentifier());
				if (host_node == nullptr) continue;
				std::string placement_diagnostic;
				std::vector<InflorescencePlacement> flower_sites;
				if (species.flower().flowerHead().isEnabled()) {
					flower_sites = FlowerHeadPlacementService().place(
						species.flower().flowerHead(), growth_tip.identifier(),
						host_node->position(), growth_tip.direction(),
						&placement_diagnostic);
				}
				else if (species.flower().inflorescence().isEnabled()) {
					flower_sites = InflorescencePlacementService().place(
						species.flower().inflorescence(), growth_tip.identifier(),
						host_node->position(), growth_tip.direction(),
						&placement_diagnostic);
				}
				else {
					flower_sites.emplace_back(
						growth_tip.identifier() + ":flower",
						InflorescenceKind::Raceme, 0u, host_node->position(),
						growth_tip.direction(), 1.0f, glm::mat4(1.0f));
				}
				if (!placement_diagnostic.empty()) {
					return VegetationGeometryBuildResult::failed(
						placement_diagnostic);
				}
				for (const InflorescencePlacement &flower_site : flower_sites) {
					const WhorlSpecification base_whorl = species.flower().petalWhorl();
					const WhorlSpecification scaled_whorl(
						base_whorl.organCount(),
						base_whorl.radius() * flower_site.scale(),
						base_whorl.phaseDegrees(), base_whorl.tiltDegrees());
					const std::vector<VegetationOrganPlacement> whorl =
						OrganPlacementService().placeWhorl(
							flower_site.identifier() + ":petal",
							flower_site.position(), flower_site.direction(), scaled_whorl,
							&placement_diagnostic);
					if (!placement_diagnostic.empty()) {
						return VegetationGeometryBuildResult::failed(
							placement_diagnostic);
					}
					for (const VegetationOrganPlacement &placement : whorl) {
						petal_transforms.push_back(glm::scale(
							placement.localTransform(), glm::vec3(flower_site.scale())));
					}
				}
			}
			if (!petal_transforms.empty()) {
				InstanceArraySpecification petal_array(petal_transforms);
				const GeneratedPrimitiveMesh tagged_petal_mesh =
					retagged_petals(petal_mesh.generatedMesh());
				GeometryBuildResult petal_instances =
					InstanceArrayGeometryBuilder(complexity_limits_).build(
						tagged_petal_mesh, petal_array);
				if (!petal_instances.succeeded()) {
					return VegetationGeometryBuildResult::failed(
						petal_instances.firstDiagnostic());
				}
				std::ostringstream canonical;
				canonical << "PetalWhorlInstanceArray:v2:source="
				          << petal_shape->key().canonicalValue() << ":count="
				          << petal_transforms.size() << ":flowerHead="
				          << species.flower().flowerHead().isEnabled()
				          << ":inflorescence="
				          << species.flower().inflorescence().isEnabled();
				auto array_shape =
					std::make_shared<const InstanceArrayShapeSpecification>(
						petal_shape, std::move(petal_array),
						ShapeSpecificationKey(canonical.str()), canonical.str(),
						detail_level);
				parts.emplace_back(
					"petal_whorls", "InflorescencePetalWhorls",
					"flowerpetalmattepaint", std::move(array_shape),
					petal_instances.generatedMesh(), glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::ConstructionDetail,
						GeometryDetailLevel::FastenersAndSeals));
			}
		}
	}

	std::vector<GeneratedMeshPlacement> placements;
	placements.reserve(parts.size());
	for (const VegetationGeometryPart &part : parts) {
		placements.emplace_back(
			part.partIdentifier(), part.generatedMesh(), part.localTransform());
	}
	GeometryBuildResult combined =
		GeneratedMeshComposer(complexity_limits_).compose(placements);
	if (!combined.succeeded()) {
		return VegetationGeometryBuildResult::failed(combined.firstDiagnostic());
	}
	return VegetationGeometryBuildResult::succeeded(
		VegetationAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), detail_level));
}
