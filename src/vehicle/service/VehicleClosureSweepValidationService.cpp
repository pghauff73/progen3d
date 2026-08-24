#include "vehicle/service/VehicleClosureSweepValidationService.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

double distanceToAxis(
	const glm::dvec3 &point,
	const glm::dvec3 &axis_point,
	const glm::dvec3 &axis_direction)
{
	return glm::length(glm::cross(point - axis_point, axis_direction));
}

} // namespace

VehicleClosureSweep VehicleClosureSweepValidationService::validateAndBuildSweep(
	const VehicleClosureSweep &unevaluated_sweep,
	const GeneratedPrimitiveMesh &closure_mesh,
	const GeneratedPrimitiveMesh &fixed_body_mesh,
	const glm::dmat4 &source_to_progen3d_matrix) const
{
	const VehicleClosureSurfaceBinding &binding =
		unevaluated_sweep.surfaceBinding();
	if (closure_mesh.mesh()->faces.empty() || fixed_body_mesh.mesh()->faces.empty()) {
		throw std::invalid_argument(
			"Vehicle closure sweep validation requires nonempty closure and fixed-body meshes.");
	}
	const VehicleClosureAdjacencyExclusion exclusion =
		createAdjacencyExclusion(binding.closureIdentifier());
	const glm::dvec3 target_axis_point = reference_frame_service_.transformPoint(
		binding.joint().sourcePivot(), source_to_progen3d_matrix);
	const glm::dvec3 transformed_axis = glm::dmat3(source_to_progen3d_matrix) *
		binding.joint().sourceAxis();
	const double axis_length = glm::length(transformed_axis);
	if (!std::isfinite(axis_length) || axis_length <= 1.0e-12) {
		throw std::invalid_argument(
			"Vehicle closure sweep validation requires a finite hinge axis.");
	}
	const glm::dvec3 target_axis_direction = transformed_axis / axis_length;
	const GeneratedPrimitiveMesh fixed_non_hinge_mesh = excludeAxisZone(
		fixed_body_mesh, target_axis_point, target_axis_direction,
		exclusion.fixedSurfaceAxisRadiusMetres());

	std::vector<VehicleClosurePoseSample> validated_samples;
	validated_samples.reserve(unevaluated_sweep.poseSamples().size());
	for (const VehicleClosurePoseSample &sample : unevaluated_sweep.poseSamples()) {
		const glm::dmat4 target_transform =
			reference_frame_service_.transformRigidBodyPose(
				sample.sourceTransform(), source_to_progen3d_matrix);
		const RigidTransformValidationReport rigid_report =
			rigid_transform_service_.validate(target_transform);
		const GeneratedPrimitiveMesh posed_closure =
			transformation_service_.transform(closure_mesh, target_transform);
		const GeneratedPrimitiveMesh moving_non_hinge_mesh = excludeAxisZone(
			posed_closure, target_axis_point, target_axis_direction,
			exclusion.movingSurfaceAxisRadiusMetres());
		const MeshSurfaceDistanceReport distance = distance_service_.evaluate(
			*moving_non_hinge_mesh.mesh(), *fixed_non_hinge_mesh.mesh());
		const MeshIntersectionScreeningReport intersections =
			intersection_service_.screen(
				*moving_non_hinge_mesh.mesh(), *fixed_non_hinge_mesh.mesh());
		const bool passed = rigid_report.passed() && distance.complete() &&
			distance.finite() && distance.minimumDistance() >= -1.0e-9 &&
			intersections.passed();
		validated_samples.emplace_back(
			sample.normalizedState(), sample.angleDegrees(), sample.riseMetres(),
			sample.sourceTransform(),
			VehicleClosurePoseValidation(
				distance, intersections, rigid_report, passed));
	}
	return VehicleClosureSweep(binding, std::move(validated_samples));
}

VehicleClosureAdjacencyExclusion
VehicleClosureSweepValidationService::createAdjacencyExclusion(
	const std::string &closure_identifier) const
{
	const double moving_radius =
		closure_identifier.find("door") != std::string::npos ? 0.120 : 0.075;
	return VehicleClosureAdjacencyExclusion(
		moving_radius, moving_radius * 0.72);
}

GeneratedPrimitiveMesh VehicleClosureSweepValidationService::excludeAxisZone(
	const GeneratedPrimitiveMesh &source_mesh,
	const glm::dvec3 &axis_point,
	const glm::dvec3 &axis_direction,
	double radius_metres) const
{
	auto selected_mesh = std::make_shared<Mesh>();
	std::vector<MeshSurfaceTag> selected_tags;
	std::vector<int> remapped_vertices(source_mesh.mesh()->vertices.size(), -1);
	for (std::size_t face_index = 0u;
	     face_index < source_mesh.mesh()->faces.size(); ++face_index) {
		const glm::ivec3 source_face = source_mesh.mesh()->faces[face_index];
		const glm::dvec3 centroid =
			(glm::dvec3(source_mesh.mesh()->vertices[static_cast<std::size_t>(source_face.x)]) +
			 glm::dvec3(source_mesh.mesh()->vertices[static_cast<std::size_t>(source_face.y)]) +
			 glm::dvec3(source_mesh.mesh()->vertices[static_cast<std::size_t>(source_face.z)])) /
			3.0;
		if (distanceToAxis(centroid, axis_point, axis_direction) < radius_metres) {
			continue;
		}
		glm::ivec3 target_face(0);
		for (int corner = 0; corner < 3; ++corner) {
			const int source_index = source_face[corner];
			int &target_index =
				remapped_vertices[static_cast<std::size_t>(source_index)];
			if (target_index < 0) {
				target_index = static_cast<int>(selected_mesh->vertices.size());
				selected_mesh->vertices.push_back(
					source_mesh.mesh()->vertices[static_cast<std::size_t>(source_index)]);
			}
			target_face[corner] = target_index;
		}
		selected_mesh->faces.push_back(target_face);
		selected_tags.push_back(
			face_index < source_mesh.faceSurfaceTags().size()
				? source_mesh.faceSurfaceTags()[face_index]
				: MeshSurfaceTag(MeshSurfaceRole::Outer));
	}
	if (selected_mesh->faces.empty()) {
		throw std::runtime_error(
			"Vehicle closure adjacency exclusion removed every mesh face.");
	}
	selected_mesh->calc_normals();
	return GeneratedPrimitiveMesh(std::move(selected_mesh), std::move(selected_tags));
}
