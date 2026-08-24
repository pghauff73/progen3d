#include "vehicle/service/VehicleGlassMotionValidationService.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

VehicleGlassMotion VehicleGlassMotionValidationService::validateAndBuildMotion(
	const VehicleGlassMotion &unevaluated_motion,
	const GeneratedPrimitiveMesh &glass_mesh,
	const GeneratedPrimitiveMesh &parent_closure_mesh,
	const GeneratedPrimitiveMesh &fixed_body_mesh,
	const glm::dmat4 &parent_closure_source_transform,
	const glm::dmat4 &source_to_progen3d_matrix) const
{
	if (glass_mesh.mesh()->faces.empty() || parent_closure_mesh.mesh()->faces.empty() ||
	    fixed_body_mesh.mesh()->faces.empty()) {
		throw std::invalid_argument(
			"Vehicle glass motion validation requires nonempty glass, parent-door, and fixed-body meshes.");
	}
	const VehicleGlassSurfaceBinding &binding =
		unevaluated_motion.surfaceBinding();
	const DoorCavityBounds cavity_bounds = createDoorCavityBounds(
		*parent_closure_mesh.mesh(), binding.motion().side());
	const glm::dmat4 target_parent_transform =
		reference_frame_service_.transformRigidBodyPose(
			parent_closure_source_transform, source_to_progen3d_matrix);

	std::vector<VehicleGlassPoseSample> validated_samples;
	validated_samples.reserve(unevaluated_motion.poseSamples().size());
	for (const VehicleGlassPoseSample &sample : unevaluated_motion.poseSamples()) {
		const glm::dmat4 target_local_transform =
			reference_frame_service_.transformRigidBodyPose(
				sample.localSourceTransform(), source_to_progen3d_matrix);
		const glm::dmat4 target_world_transform =
			reference_frame_service_.transformRigidBodyPose(
				sample.worldSourceTransform(), source_to_progen3d_matrix);
		const glm::dmat4 expected_world_transform =
			target_parent_transform * target_local_transform;
		const double support_error = maximumMatrixDifference(
			target_world_transform, expected_world_transform);
		const RigidTransformValidationReport rigid_report =
			rigid_transform_service_.validate(target_world_transform);
		const GeneratedPrimitiveMesh door_local_posed_glass =
			transformation_service_.transform(glass_mesh, target_local_transform);
		const double cavity_fraction = containmentFraction(
			*door_local_posed_glass.mesh(), cavity_bounds,
			sample.normalizedState());
		const MeshSurfaceDistanceReport distance = distance_service_.evaluate(
			*door_local_posed_glass.mesh(), *fixed_body_mesh.mesh());
		const MeshIntersectionScreeningReport intersections =
			intersection_service_.screen(
				*door_local_posed_glass.mesh(), *fixed_body_mesh.mesh());
		const double required_cavity_fraction =
			sample.normalizedState() >= 0.4 ? 0.90 : 0.82;
		const bool passed = rigid_report.passed() && support_error <= 1.0e-9 &&
			cavity_fraction >= required_cavity_fraction && distance.complete() &&
			distance.finite() && distance.minimumDistance() >= 0.0002 &&
			intersections.passed();
		validated_samples.emplace_back(
			sample.normalizedState(), sample.verticalDropMetres(),
			sample.rotationDegrees(), sample.localSourceTransform(),
			sample.worldSourceTransform(),
			VehicleGlassPoseValidation(
				cavity_fraction, distance, support_error, intersections,
				rigid_report, passed));
	}
	return VehicleGlassMotion(binding, std::move(validated_samples));
}

VehicleGlassMotionValidationService::DoorCavityBounds
VehicleGlassMotionValidationService::createDoorCavityBounds(
	const Mesh &parent_closure_mesh,
	const std::string &side) const
{
	if (side != "left" && side != "right") {
		throw std::invalid_argument(
			"Vehicle glass door-cavity side must be left or right.");
	}
	glm::dvec3 minimum(std::numeric_limits<double>::infinity());
	glm::dvec3 maximum(-std::numeric_limits<double>::infinity());
	for (const glm::vec3 &vertex : parent_closure_mesh.vertices) {
		minimum = glm::min(minimum, glm::dvec3(vertex));
		maximum = glm::max(maximum, glm::dvec3(vertex));
	}
	const glm::dvec3 target_margin(0.025, 0.030, 0.030);
	minimum -= target_margin;
	maximum += target_margin;
	const double centreward_allowance = 0.085;
	if (side == "left") {
		maximum.x += centreward_allowance;
	} else {
		minimum.x -= centreward_allowance;
	}
	return {minimum, maximum};
}

double VehicleGlassMotionValidationService::containmentFraction(
	const Mesh &posed_glass_mesh,
	const DoorCavityBounds &cavity_bounds,
	double normalized_state) const
{
	if (posed_glass_mesh.vertices.empty()) return 0.0;
	if (normalized_state <= 1.0e-9) return 1.0;
	const glm::dvec3 validation_allowance(0.020);
	std::size_t inside_count = 0u;
	for (const glm::vec3 &vertex : posed_glass_mesh.vertices) {
		const glm::dvec3 point(vertex);
		if (glm::all(glm::greaterThanEqual(
				point, cavity_bounds.minimum - validation_allowance)) &&
		    glm::all(glm::lessThanEqual(
				point, cavity_bounds.maximum + validation_allowance))) {
			++inside_count;
		}
	}
	return static_cast<double>(inside_count) /
		static_cast<double>(posed_glass_mesh.vertices.size());
}

double VehicleGlassMotionValidationService::maximumMatrixDifference(
	const glm::dmat4 &first,
	const glm::dmat4 &second) const
{
	double maximum_error = 0.0;
	for (std::size_t column = 0u; column < 4u; ++column) {
		for (std::size_t row = 0u; row < 4u; ++row) {
			maximum_error = std::max(
				maximum_error,
				std::abs(first[column][row] - second[column][row]));
		}
	}
	return maximum_error;
}
