#include "architecture/service/PanelArrayAssemblyBuilder.h"

#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/InstanceArraySpecification.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/Profile2DFactory.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

ArchitecturalAssemblyBuildResult PanelArrayAssemblyBuilder::build(
	const PanelArraySpecification &specification) const
{
	const glm::vec3 &spacing = specification.panelSpacing();
	if (specification.objectIdentifier().empty() ||
	    specification.materialIdentifier().empty() ||
	    !std::isfinite(specification.panelWidth()) ||
	    !std::isfinite(specification.panelHeight()) ||
	    !std::isfinite(specification.panelDepth()) ||
	    !std::isfinite(spacing.x) || !std::isfinite(spacing.y) ||
	    !std::isfinite(spacing.z) || specification.panelWidth() <= 0.0f ||
	    specification.panelHeight() <= 0.0f ||
	    specification.panelDepth() <= 0.0f || specification.panelCount() == 0u ||
	    specification.panelCount() > complexity_limits_.maximumInstanceArrayCount()) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"PanelArray requires finite positive panel dimensions and a bounded count.");
	}

	std::string diagnostic;
	auto profile = Profile2DFactory(complexity_limits_).createRectangle(
		specification.panelWidth(), specification.panelHeight(), &diagnostic);
	ExtrudeProfileShapeSpecificationCandidate candidate;
	if (profile) candidate.profile.outer_loop = profile->outerLoop().points();
	candidate.depth = specification.panelDepth();
	auto shape = profile
		? ExtrudeProfileSpecificationValidator(complexity_limits_).validate(
			std::move(candidate), &diagnostic)
		: std::shared_ptr<const ExtrudeProfileShapeSpecification>();
	if (!shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	GeometryBuildResult source =
		ExtrudeProfileMeshGenerator(complexity_limits_).build(*shape);
	if (!source.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			source.status(), source.firstDiagnostic());
	}

	const InstanceArraySpecification array = InstanceArraySpecification::createLinear(
		specification.panelCount(), spacing);
	GeometryBuildResult combined = InstanceArrayGeometryBuilder(complexity_limits_).build(
		source.generatedMesh(), array);
	if (!combined.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			combined.status(), combined.firstDiagnostic());
	}

	std::vector<ArchitecturalAssemblyPart> parts;
	parts.reserve(specification.panelCount());
	for (std::size_t index = 0; index < specification.panelCount(); ++index) {
		const glm::mat4 transform = array.transforms()[index];
		parts.emplace_back(
			"panel_" + std::to_string(index), "RepeatedPanel",
			specification.materialIdentifier(), shape, source.generatedMesh(), transform);
	}
	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), {}));
}
