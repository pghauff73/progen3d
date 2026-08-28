#include "architecture/service/ArchitecturalPartGeometryBuilder.h"

#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/LoftMeshGenerator.h"
#include "geometry/service/LoftSpecificationValidator.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/RoundedBoxShapeSpecificationFactory.h"

#include <cmath>
#include <memory>
#include <utility>

namespace {

ArchitecturalPartGeometry invalid_geometry()
{
	return ArchitecturalPartGeometry(
		{}, GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}));
}

}

ArchitecturalPartGeometry ArchitecturalPartGeometryBuilder::buildExtrudedProfile(
	const Profile2D &profile,
	float depth,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	ExtrudeProfileShapeSpecificationCandidate candidate;
	candidate.profile.outer_loop = profile.outerLoop().points();
	for (const ProfileLoop2D &inner_loop : profile.innerLoops()) {
		candidate.profile.inner_loops.push_back(inner_loop.points());
	}
	candidate.depth = depth;
	candidate.detail_level = detail_level;
	auto shape = ExtrudeProfileSpecificationValidator(complexity_limits_).validate(
		std::move(candidate), diagnostic);
	if (!shape) return invalid_geometry();
	GeometryBuildResult built =
		ExtrudeProfileMeshGenerator(complexity_limits_).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return invalid_geometry();
	}
	return ArchitecturalPartGeometry(shape, built.generatedMesh());
}

ArchitecturalPartGeometry ArchitecturalPartGeometryBuilder::buildRectangularPrism(
	float width,
	float height,
	float depth,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	auto profile = Profile2DFactory(complexity_limits_).createRectangle(
		width, height, diagnostic);
	return profile
		? buildExtrudedProfile(*profile, depth, detail_level, diagnostic)
		: invalid_geometry();
}

ArchitecturalPartGeometry ArchitecturalPartGeometryBuilder::buildRoundedBox(
	const RoundedBoxSpecification &specification,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	auto shape = RoundedBoxShapeSpecificationFactory(complexity_limits_).create(
		specification, detail_level, diagnostic);
	if (!shape) return invalid_geometry();
	GeometryBuildResult built =
		ExtrudeProfileMeshGenerator(complexity_limits_).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return invalid_geometry();
	}
	return ArchitecturalPartGeometry(shape, built.generatedMesh());
}

ArchitecturalPartGeometry ArchitecturalPartGeometryBuilder::buildCircularPrism(
	float diameter,
	float depth,
	int segments,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	if (!std::isfinite(diameter) || diameter <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Circular architectural parts require a finite positive diameter.";
		}
		return invalid_geometry();
	}
	auto profile = Profile2DFactory(complexity_limits_).createCircle(
		diameter * 0.5f, segments, diagnostic);
	return profile
		? buildExtrudedProfile(*profile, depth, detail_level, diagnostic)
		: invalid_geometry();
}

ArchitecturalPartGeometry
ArchitecturalPartGeometryBuilder::buildShellLoftedCushion(
	float width,
	float height,
	float depth,
	float corner_radius,
	float shell_thickness,
	float middle_bulge,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	if (!std::isfinite(shell_thickness) || !std::isfinite(middle_bulge) ||
	    shell_thickness <= 0.0f || middle_bulge < 0.0f ||
	    width <= shell_thickness * 2.0f ||
	    height <= shell_thickness * 2.0f || depth <= 0.0f ||
	    corner_radius <= shell_thickness) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"ShellLoft cushion dimensions must contain a finite positive upholstery shell.";
		}
		return invalid_geometry();
	}
	auto outer = Profile2DFactory(complexity_limits_).createRoundedRectangle(
		width, height, corner_radius, 4, diagnostic);
	auto inner = Profile2DFactory(complexity_limits_).createRoundedRectangle(
		width - shell_thickness * 2.0f,
		height - shell_thickness * 2.0f,
		corner_radius - shell_thickness,
		4,
		diagnostic);
	if (!outer || !inner) return invalid_geometry();

	ShellLoftShapeSpecificationCandidate candidate;
	for (const auto &section_data :
	     std::vector<std::pair<float, float>>{
		     {0.0f, 1.0f},
		     {depth * 0.5f, 1.0f + middle_bulge},
		     {depth, 1.0f}}) {
		ShellLoftSectionCandidate section;
		section.axial_position = section_data.first;
		section.outer_loop = outer->outerLoop().points();
		section.inner_loop = inner->outerLoop().points();
		section.scale = glm::vec2(section_data.second);
		candidate.sections.push_back(std::move(section));
	}
	candidate.detail_level = detail_level;
	auto shape = LoftSpecificationValidator(complexity_limits_).validateShellLoft(
		std::move(candidate), diagnostic);
	if (!shape) return invalid_geometry();
	GeometryBuildResult built = LoftMeshGenerator(complexity_limits_).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return invalid_geometry();
	}
	return ArchitecturalPartGeometry(shape, built.generatedMesh());
}
