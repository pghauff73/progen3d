#include "geometry/service/InstanceArrayGeometryBuilder.h"

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/service/GeneratedMeshComposer.h"

#include <string>
#include <vector>

GeometryBuildResult InstanceArrayGeometryBuilder::build(
	const GeneratedPrimitiveMesh &source,
	const InstanceArraySpecification &array) const
{
	if (!source.mesh() || source.mesh()->faces.empty()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"InstanceArray requires a non-empty source mesh.");
	}
	if (array.transforms().empty()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"InstanceArray requires at least one transform.");
	}
	if (array.transforms().size() > complexity_limits_.maximumInstanceArrayCount()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::TriangleLimitExceeded,
			"InstanceArray exceeds the configured instance count limit.");
	}
	std::vector<GeneratedMeshPlacement> placements;
	placements.reserve(array.transforms().size());
	for (std::size_t index = 0; index < array.transforms().size(); ++index) {
		placements.emplace_back(
			"instance_" + std::to_string(index), source, array.transforms()[index]);
	}
	return GeneratedMeshComposer(complexity_limits_).compose(placements);
}
