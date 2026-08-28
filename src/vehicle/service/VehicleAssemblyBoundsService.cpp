#include "vehicle/service/VehicleAssemblyBoundsService.h"

#include <glm/common.hpp>

AxisAlignedBounds VehicleAssemblyBoundsService::calculate(
	const VehiclePlacedAssembly &assembly) const
{
	AxisAlignedBounds bounds;
	for (const VehicleAssemblyPart &part : assembly.geometry().parts()) {
		if (!part.generatedMesh().mesh()) continue;
		const glm::mat4 transform =
			assembly.localTransform() * part.localTransform();
		for (const glm::vec3 &vertex : part.generatedMesh().mesh()->vertices) {
			const glm::vec3 transformed = glm::vec3(
				transform * glm::vec4(vertex, 1.0f));
			if (!bounds.valid) {
				bounds.min = transformed;
				bounds.max = transformed;
				bounds.valid = true;
			}
			else {
				bounds.min = glm::min(bounds.min, transformed);
				bounds.max = glm::max(bounds.max, transformed);
			}
		}
	}
	if (bounds.valid) {
		bounds.center = (bounds.min + bounds.max) * 0.5f;
		bounds.half_extents = (bounds.max - bounds.min) * 0.5f;
	}
	return bounds;
}
