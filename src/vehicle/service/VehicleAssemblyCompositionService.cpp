#include "vehicle/service/VehicleAssemblyCompositionService.h"

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/service/GeneratedMeshComposer.h"

#include <cmath>
#include <string>
#include <vector>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool finite(const glm::mat4 &value)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(value[column][row])) return false;
		}
	}
	return true;
}

} // namespace

GeometryBuildResult VehicleAssemblyCompositionService::compose(
	const std::vector<VehicleAssemblyPart> &parts) const
{
	std::vector<GeneratedMeshPlacement> placements;
	placements.reserve(parts.size());
	for (const VehicleAssemblyPart &part : parts) {
		if (!finite(part.localTransform())) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::NonFiniteGeometry,
				"Vehicle assembly part '" + part.partIdentifier() +
					"' has a non-finite local transform.");
		}
		if (!part.generatedMesh().mesh()) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::UnsupportedTopology,
				"Vehicle assembly part '" + part.partIdentifier() +
					"' has no generated mesh.");
		}
		for (const glm::vec3 &vertex : part.generatedMesh().mesh()->vertices) {
			if (!finite(vertex)) {
				return GeometryBuildResult::createFailure(
					GeometryBuildStatus::NonFiniteGeometry,
					"Vehicle assembly part '" + part.partIdentifier() +
						"' contains a non-finite vertex.");
			}
		}
		placements.emplace_back(
			part.partIdentifier(), part.generatedMesh(), part.localTransform());
	}
	return GeneratedMeshComposer(complexity_limits_).compose(placements);
}
