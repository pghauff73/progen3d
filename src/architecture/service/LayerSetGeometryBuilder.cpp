#include "architecture/service/LayerSetGeometryBuilder.h"

#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/GeneratedMeshComposer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <utility>

LayerSetBuildResult LayerSetGeometryBuilder::build(
	const LayerSetSpecification &specification) const
{
	if (specification.layers().empty()) {
		return LayerSetBuildResult::failed(
			GeometryBuildStatus::UnsupportedTopology,
			"LayerSet requires at least one layer.");
	}

	std::vector<LayerGeometryPart> layer_parts;
	std::vector<GeneratedMeshPlacement> placements;
	layer_parts.reserve(specification.layers().size());
	placements.reserve(specification.layers().size());
	float axial_offset = 0.0f;
	for (const LayerDefinition &layer : specification.layers()) {
		if (layer.name().empty() || layer.materialIdentifier().empty() ||
		    !std::isfinite(layer.thickness()) || layer.thickness() <= 0.0f) {
			return LayerSetBuildResult::failed(
				GeometryBuildStatus::InvalidProfile,
				"LayerSet layers require names, materials, and finite positive thickness.");
		}
		ExtrudeProfileShapeSpecificationCandidate candidate;
		candidate.profile.outer_loop =
			specification.referenceProfile().outerLoop().points();
		for (const ProfileLoop2D &hole :
		     specification.referenceProfile().innerLoops()) {
			candidate.profile.inner_loops.push_back(hole.points());
		}
		candidate.depth = layer.thickness();
		std::string diagnostic;
		auto shape = ExtrudeProfileSpecificationValidator(complexity_limits_).validate(
			std::move(candidate), &diagnostic);
		if (!shape) {
			return LayerSetBuildResult::failed(
				GeometryBuildStatus::InvalidProfile, diagnostic);
		}
		GeometryBuildResult built =
			ExtrudeProfileMeshGenerator(complexity_limits_).build(*shape);
		if (!built.succeeded()) {
			return LayerSetBuildResult::failed(
				built.status(), built.firstDiagnostic());
		}
		const glm::mat4 transform = glm::translate(
			glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, axial_offset));
		placements.emplace_back(layer.name(), built.generatedMesh(), transform);
		layer_parts.emplace_back(
			layer, axial_offset, shape, built.generatedMesh());
		axial_offset += layer.thickness();
	}

	GeometryBuildResult combined =
		GeneratedMeshComposer(complexity_limits_).compose(placements);
	if (!combined.succeeded()) {
		return LayerSetBuildResult::failed(
			combined.status(), combined.firstDiagnostic());
	}
	return LayerSetBuildResult::succeeded(
		LayerSetGeometry(
			std::move(layer_parts), combined.generatedMesh()));
}
