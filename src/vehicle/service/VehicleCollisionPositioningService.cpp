#include "vehicle/service/VehicleCollisionPositioningService.h"

#include "spatial/service/SpatialResolutionEvidenceHashService.h"
#include "vehicle/service/VehicleAssemblyBoundsService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace {

int cardinal_axis(glm::vec3 direction)
{
	int selected_axis = -1;
	for (int axis = 0; axis < 3; ++axis) {
		if (std::fabs(direction[axis]) <= 1.0e-6f) continue;
		if (selected_axis >= 0) return -1;
		selected_axis = axis;
	}
	return selected_axis;
}

bool overlaps_orthogonal_axes(
	const AxisAlignedBounds &moving,
	const AxisAlignedBounds &target,
	int movement_axis,
	float tolerance)
{
	for (int axis = 0; axis < 3; ++axis) {
		if (axis == movement_axis) continue;
		if (moving.max[axis] < target.min[axis] - tolerance ||
		    target.max[axis] < moving.min[axis] - tolerance) {
			return false;
		}
	}
	return true;
}

} // namespace

VehicleAssemblyPlacementResult
VehicleCollisionPositioningService::positionAgainst(
	const std::string &constraint_identifier,
	const VehiclePlacedAssembly &moving_assembly,
	const VehiclePlacedAssembly &target_assembly,
	glm::vec3 direction,
	float clearance,
	float maximum_distance,
	float tolerance) const
{
	if (constraint_identifier.empty() ||
	    moving_assembly.objectIdentifier() == target_assembly.objectIdentifier() ||
	    !std::isfinite(clearance) || clearance < 0.0f ||
	    !std::isfinite(maximum_distance) || maximum_distance <= 0.0f ||
	    !std::isfinite(tolerance) || tolerance <= 0.0f ||
	    glm::length(direction) <= 1.0e-6f) {
		return VehicleAssemblyPlacementResult::failed(
			"Vehicle collision positioning requires distinct assemblies, a cardinal direction, nonnegative clearance, and positive limits.");
	}
	direction = glm::normalize(direction);
	const int movement_axis = cardinal_axis(direction);
	if (movement_axis < 0 ||
	    std::fabs(std::fabs(direction[movement_axis]) - 1.0f) > 1.0e-5f) {
		return VehicleAssemblyPlacementResult::failed(
			"Vehicle collision positioning P0 supports one world cardinal axis at a time.");
	}

	VehicleAssemblyBoundsService bounds_service;
	const AxisAlignedBounds moving_bounds = bounds_service.calculate(moving_assembly);
	const AxisAlignedBounds target_bounds = bounds_service.calculate(target_assembly);
	if (!moving_bounds.valid || !target_bounds.valid) {
		return VehicleAssemblyPlacementResult::failed(
			"Vehicle collision positioning requires rendered bounds for both assemblies at the selected LOD.");
	}
	if (!overlaps_orthogonal_axes(
			moving_bounds, target_bounds, movement_axis, tolerance)) {
		return VehicleAssemblyPlacementResult::failed(
			"Vehicle collision positioning cannot reach the target because orthogonal bounds do not overlap.");
	}

	float travel = 0.0f;
	if (direction[movement_axis] > 0.0f) {
		travel = target_bounds.min[movement_axis] -
		         moving_bounds.max[movement_axis] - clearance;
	}
	else {
		travel = moving_bounds.min[movement_axis] -
		         target_bounds.max[movement_axis] - clearance;
	}
	if (travel < -tolerance) {
		return VehicleAssemblyPlacementResult::failed(
			"Vehicle collision positioning starts in penetration or beyond the target contact plane.");
	}
	travel = std::max(0.0f, travel);
	if (travel > maximum_distance + tolerance) {
		return VehicleAssemblyPlacementResult::failed(
			"Vehicle collision positioning exceeds the declared maximum travel.");
	}

	const glm::vec3 translation = direction * travel;
	const glm::mat4 final_transform = glm::translate(
		glm::mat4(1.0f), translation) * moving_assembly.localTransform();
	glm::vec3 contact_point = moving_bounds.center + translation;
	contact_point[movement_axis] = direction[movement_axis] > 0.0f
		? target_bounds.min[movement_axis]
		: target_bounds.max[movement_axis];
	for (int axis = 0; axis < 3; ++axis) {
		if (axis == movement_axis) continue;
		contact_point[axis] =
			(std::max(moving_bounds.min[axis] + translation[axis], target_bounds.min[axis]) +
			 std::min(moving_bounds.max[axis] + translation[axis], target_bounds.max[axis])) *
			0.5f;
	}

	SpatialResolutionRecord record(
		SpatialConstraintId(constraint_identifier),
		SpatialObjectId(moving_assembly.objectIdentifier()),
		SpatialObjectId(target_assembly.objectIdentifier()),
		moving_assembly.localTransform(), final_transform,
		SpatialResolutionAlgorithm::AxisAlignedSweep,
		1, 0, 1, tolerance, contact_point, -direction,
		clearance, 0.0f, SpatialResolutionStatus::Succeeded, {}, 0u);
	record = SpatialResolutionEvidenceHashService().attachCalculatedHash(record);
	return VehicleAssemblyPlacementResult::succeeded(VehicleAssemblyPlacement(
		moving_assembly.withLocalTransform(final_transform), std::move(record)));
}
