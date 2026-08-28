#include "vegetation/service/PlantOrganGeometryFactory.h"

#include "geometry/model/InstanceArrayShapeSpecification.h"
#include "geometry/service/BotanicalBladeMeshGenerator.h"
#include "geometry/service/BotanicalBladeSpecificationValidator.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/ShapeSpecificationValidator.h"
#include "geometry/service/SphereMeshGenerator.h"
#include "geometry/service/TaperedSweepMeshGenerator.h"
#include "geometry/service/TaperedSweepSpecificationValidator.h"
#include "vegetation/service/OrganPlacementService.h"
#include "vegetation/service/FruitShellGeometryFactory.h"

#include <glm/glm.hpp>

#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace {

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

std::optional<PlantOrganGeometrySource> create_leaf_or_petal(
	const PlantSpeciesSpecification &species,
	VegetationOrganType organ_type,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	const bool petal = organ_type == VegetationOrganType::Petal;
	if (petal && !species.flower().isEnabled()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Petal OrganArray geometry requires flower(...).";
		}
		return std::nullopt;
	}
	BotanicalBladeShapeSpecificationCandidate candidate;
	candidate.kind = petal ? BotanicalBladeKind::Petal : BotanicalBladeKind::Leaf;
	if (petal) {
		candidate.profile = species.flower().petalProfile();
		candidate.length = species.flower().petalLength();
		candidate.maximum_width = species.flower().petalWidth();
		candidate.longitudinal_curvature =
			species.flower().longitudinalCurvature();
		candidate.camber = species.flower().camber();
		candidate.twist_degrees = species.flower().twistDegrees();
		candidate.thickness = species.flower().thickness();
		candidate.width_power = species.flower().widthPower();
	}
	else {
		candidate.profile = species.leaf().profile();
		candidate.length = species.leaf().length();
		candidate.maximum_width = species.leaf().width();
		candidate.camber = species.leaf().camber();
		candidate.twist_degrees = species.leaf().twistDegrees();
		candidate.thickness = species.leaf().thickness();
	}
	candidate.longitudinal_segments =
		detail_level == GeometryDetailLevel::FastenersAndSeals ? 16 : 10;
	candidate.lateral_segments =
		detail_level == GeometryDetailLevel::FastenersAndSeals ? 6 : 3;
	candidate.detail_level = detail_level;
	auto shape = BotanicalBladeSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return std::nullopt;
	GeometryBuildResult mesh =
		BotanicalBladeMeshGenerator(complexity_limits).build(*shape);
	if (!mesh.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = mesh.firstDiagnostic();
		return std::nullopt;
	}
	return PlantOrganGeometrySource(
		std::move(shape), mesh.generatedMesh(),
		petal ? "InstancedOrganArrayPetals" : "InstancedOrganArrayLeaves",
		petal ? "flowerpetalmattepaint" : "leafgreenmattepaint");
}

std::optional<PlantOrganGeometrySource> create_bud_sphere(
	GeometryDetailLevel detail_level,
	std::string *diagnostic)
{
	SphereShapeSpecificationCandidate candidate;
	candidate.azimuth_segments = radial_segments_for(detail_level) * 2;
	candidate.polar_segments = radial_segments_for(detail_level);
	auto shape = ShapeSpecificationValidator().validateSphere(
		std::move(candidate), diagnostic);
	if (!shape) return std::nullopt;
	GeneratedPrimitiveMesh mesh = SphereMeshGenerator().generate(*shape, diagnostic);
	if (!mesh.mesh() || mesh.mesh()->faces.empty()) return std::nullopt;
	return PlantOrganGeometrySource(
		std::move(shape), std::move(mesh),
		"InstancedOrganArrayBuds", "leafbudmattepaint");
}

std::optional<PlantOrganGeometrySource> create_thorn(
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	TaperedSweepShapeSpecificationCandidate candidate;
	candidate.path_points = {
		glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
	candidate.radii = {0.08f, 0.008f};
	candidate.circumferential_segments = radial_segments_for(detail_level);
	candidate.detail_level = detail_level;
	auto shape = TaperedSweepSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return std::nullopt;
	GeometryBuildResult mesh =
		TaperedSweepMeshGenerator(complexity_limits).build(*shape);
	if (!mesh.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = mesh.firstDiagnostic();
		return std::nullopt;
	}
	return PlantOrganGeometrySource(
		std::move(shape), mesh.generatedMesh(), "InstancedOrganArrayThorns",
		"thornroughwood");
}

std::optional<PlantOrganGeometrySource> create_flower(
	const PlantSpeciesSpecification &species,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	const std::optional<PlantOrganGeometrySource> petal_source =
		create_leaf_or_petal(
			species, VegetationOrganType::Petal, detail_level,
			complexity_limits, diagnostic);
	if (!petal_source.has_value()) return std::nullopt;
	const WhorlSpecification &whorl = species.flower().petalWhorl();
	std::vector<VegetationOrganPlacement> petal_placements =
		OrganPlacementService().placeWhorl(
			"flower_source_petal", glm::vec3(0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f), whorl, diagnostic);
	if (petal_placements.empty()) return std::nullopt;
	std::vector<glm::mat4> transforms;
	transforms.reserve(petal_placements.size());
	for (const VegetationOrganPlacement &placement : petal_placements) {
		transforms.push_back(placement.localTransform());
	}
	InstanceArraySpecification array(std::move(transforms));
	GeometryBuildResult mesh = InstanceArrayGeometryBuilder(complexity_limits).build(
		petal_source->generatedMesh(), array);
	if (!mesh.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = mesh.firstDiagnostic();
		return std::nullopt;
	}
	std::ostringstream canonical;
	canonical << "FlowerOrganSource:v1:source="
	          << petal_source->shape()->key().canonicalValue() << ":count="
	          << whorl.organCount();
	auto shape = std::make_shared<const InstanceArrayShapeSpecification>(
		petal_source->shape(), std::move(array),
		ShapeSpecificationKey(canonical.str()), canonical.str(), detail_level);
	return PlantOrganGeometrySource(
		std::move(shape), mesh.generatedMesh(), "InstancedOrganArrayFlowers",
		"flowerpetalmattepaint");
}

} // namespace

std::optional<PlantOrganGeometrySource> PlantOrganGeometryFactory::create(
	const PlantSpeciesSpecification &species,
	VegetationOrganType organ_type,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	std::optional<PlantOrganGeometrySource> source;
	switch (organ_type) {
	case VegetationOrganType::Leaf:
	case VegetationOrganType::Petal:
		source = create_leaf_or_petal(
			species, organ_type, detail_level, complexity_limits_, diagnostic);
		break;
	case VegetationOrganType::Flower:
		source = create_flower(
			species, detail_level, complexity_limits_, diagnostic);
		break;
	case VegetationOrganType::Bud:
		source = create_bud_sphere(detail_level, diagnostic);
		break;
	case VegetationOrganType::Fruit:
		source = FruitShellGeometryFactory(complexity_limits_).create(
			species.fruit(), detail_level, diagnostic);
		break;
	case VegetationOrganType::Thorn:
		source = create_thorn(detail_level, complexity_limits_, diagnostic);
		break;
	case VegetationOrganType::Branch:
	case VegetationOrganType::Petiole:
	case VegetationOrganType::Tendril:
		if (diagnostic != nullptr) {
			*diagnostic =
				"OrganArray geometry supports Bud, Leaf, Petal, Flower, Fruit, and Thorn.";
		}
		return std::nullopt;
	}
	if (source.has_value() && diagnostic != nullptr) diagnostic->clear();
	return source;
}
