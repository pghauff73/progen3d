#include "architecture/service/SanitaryBasinAssemblyBuilder.h"

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/ShellLoftShapeSpecification.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/LoftMeshGenerator.h"
#include "geometry/service/LoftSpecificationValidator.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/SweepDiskMeshGenerator.h"
#include "geometry/service/SweepDiskSpecificationValidator.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

}

ArchitecturalAssemblyBuildResult SanitaryBasinAssemblyBuilder::build(
	const SanitaryBasinSpecification &specification) const
{
	if (specification.objectIdentifier().empty() ||
	    specification.basinMaterialIdentifier().empty() ||
	    specification.drainMaterialIdentifier().empty() ||
	    !finite_positive(specification.width()) ||
	    !finite_positive(specification.depth()) ||
	    !finite_positive(specification.height()) ||
	    !finite_positive(specification.wallThickness()) ||
	    !finite_positive(specification.drainDiameter()) ||
	    !finite_positive(specification.drainLength()) ||
	    specification.width() <= specification.wallThickness() * 2.0f ||
	    specification.depth() <= specification.wallThickness() * 2.0f ||
	    specification.drainDiameter() >=
		    std::min(specification.width(), specification.depth()) * 0.5f) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"SanitaryBasin requires finite positive shell and drain dimensions.");
	}

	std::string diagnostic;
	Profile2DFactory profile_factory(complexity_limits_);
	auto outer_bottom = profile_factory.createRoundedRectangle(
		specification.width() * 0.62f,
		specification.depth() * 0.58f,
		std::min(specification.width(), specification.depth()) * 0.12f,
		4,
		&diagnostic);
	auto outer_top = profile_factory.createRoundedRectangle(
		specification.width(),
		specification.depth(),
		std::min(specification.width(), specification.depth()) * 0.18f,
		4,
		&diagnostic);
	auto inner_bottom = profile_factory.createCircle(
		specification.drainDiameter() * 0.5f, 20, &diagnostic);
	auto inner_top = profile_factory.createRoundedRectangle(
		specification.width() - specification.wallThickness() * 2.0f,
		specification.depth() - specification.wallThickness() * 2.0f,
		std::max(
			0.001f,
			std::min(specification.width(), specification.depth()) * 0.18f -
				specification.wallThickness()),
		4,
		&diagnostic);
	if (!outer_bottom || !outer_top || !inner_bottom || !inner_top) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	ShellLoftShapeSpecificationCandidate shell_candidate;
	ShellLoftSectionCandidate bottom;
	bottom.axial_position = 0.0f;
	bottom.outer_loop = outer_bottom->outerLoop().points();
	bottom.inner_loop = inner_bottom->outerLoop().points();
	ShellLoftSectionCandidate top;
	top.axial_position = specification.height();
	top.outer_loop = outer_top->outerLoop().points();
	top.inner_loop = inner_top->outerLoop().points();
	shell_candidate.sections = {bottom, top};
	auto shell_shape = LoftSpecificationValidator(complexity_limits_).validateShellLoft(
		std::move(shell_candidate), &diagnostic);
	if (!shell_shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	GeometryBuildResult shell_mesh =
		LoftMeshGenerator(complexity_limits_).build(*shell_shape);
	if (!shell_mesh.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			shell_mesh.status(), shell_mesh.firstDiagnostic());
	}

	SweepDiskShapeSpecificationCandidate drain_candidate;
	drain_candidate.path_points = {
		glm::vec3(0.0f, 0.0f, -specification.drainLength()),
		glm::vec3(0.0f, 0.0f, 0.0f)};
	drain_candidate.up_hint = glm::vec3(1.0f, 0.0f, 0.0f);
	drain_candidate.radius = specification.drainDiameter() * 0.5f;
	drain_candidate.circumferential_segments = 16;
	auto drain_shape = SweepDiskSpecificationValidator(complexity_limits_).validate(
		std::move(drain_candidate), &diagnostic);
	if (!drain_shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	GeometryBuildResult drain_mesh =
		SweepDiskMeshGenerator(complexity_limits_).build(*drain_shape);
	if (!drain_mesh.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			drain_mesh.status(), drain_mesh.firstDiagnostic());
	}

	const glm::mat4 upright_transform = glm::rotate(
		glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	GeometryBuildResult combined = GeneratedMeshComposer(complexity_limits_).compose({
		GeneratedMeshPlacement("basin_shell", shell_mesh.generatedMesh(), upright_transform),
		GeneratedMeshPlacement("waste_outlet", drain_mesh.generatedMesh(), upright_transform)});
	if (!combined.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			combined.status(), combined.firstDiagnostic());
	}

	std::vector<ArchitecturalAssemblyPart> parts;
	parts.emplace_back(
		"basin_shell", "SanitaryShell", specification.basinMaterialIdentifier(),
		shell_shape, shell_mesh.generatedMesh(), upright_transform);
	parts.emplace_back(
		"waste_outlet", "DrainPipe", specification.drainMaterialIdentifier(),
		drain_shape, drain_mesh.generatedMesh(), upright_transform);

	const SpatialObjectId owner(specification.objectIdentifier());
	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("countertop_seat"), owner, SpatialInterfaceType::Seat,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, specification.height(), 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.depth()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.004f));
	interfaces.emplace_back(
		SpatialInterfaceId("waste_outlet"), owner, SpatialInterfaceType::DrainPort,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, -specification.drainLength(), 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::axisSegment(specification.drainLength()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Circular, InterfaceGender::Male,
			specification.drainDiameter(), std::nullopt, std::nullopt, "waste"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.002f));
	interfaces.emplace_back(
		SpatialInterfaceId("cold_water_inlet"), owner, SpatialInterfaceType::PipePort,
		SpatialInterfaceFrame(
			glm::vec3(-0.08f, specification.height(), specification.depth() * 0.35f),
			glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::point(),
		InterfaceCompatibilityProfile(
			InterfaceShape::Circular, InterfaceGender::Female,
			0.015f, std::nullopt, std::nullopt, "cold-water"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.002f));
	interfaces.emplace_back(
		SpatialInterfaceId("hot_water_inlet"), owner, SpatialInterfaceType::PipePort,
		SpatialInterfaceFrame(
			glm::vec3(0.08f, specification.height(), specification.depth() * 0.35f),
			glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::point(),
		InterfaceCompatibilityProfile(
			InterfaceShape::Circular, InterfaceGender::Female,
			0.015f, std::nullopt, std::nullopt, "hot-water"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.002f));

	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces)));
}
