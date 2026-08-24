#include "vegetation/service/FruitShellGeometryFactory.h"

#include "geometry/model/MeshSurfaceTag.h"
#include "geometry/service/RevolveMeshGenerator.h"
#include "geometry/service/RevolveSpecificationValidator.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace {

bool validate_fruit_shell(
	const PlantFruitSpecification &specification,
	std::string *diagnostic)
{
	if (!specification.isEnabled() ||
	    !std::isfinite(specification.height()) ||
	    !std::isfinite(specification.maximumRadius()) ||
	    !std::isfinite(specification.shoulderFraction()) ||
	    !std::isfinite(specification.fullness()) ||
	    specification.height() <= 0.0f ||
	    specification.maximumRadius() <= 0.0f ||
	    specification.shoulderFraction() < 0.20f ||
	    specification.shoulderFraction() > 0.80f ||
	    specification.fullness() < 2.0f ||
	    specification.fullness() > 12.0f ||
	    specification.radialSegments() < 6 ||
	    specification.profileSegments() < 4) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"FruitShell requires positive height/radius, shoulder in [0.20,0.80], fullness in [2,12], at least six radial segments, and at least four profile segments.";
		}
		return false;
	}
	return true;
}

float normalized_radius(
	float position,
	float shoulder_fraction,
	float fullness)
{
	if (position <= 0.0f || position >= 1.0f) return 0.0f;
	const float first_exponent = fullness * shoulder_fraction;
	const float second_exponent = fullness * (1.0f - shoulder_fraction);
	const float numerator =
		std::pow(position, first_exponent) *
		std::pow(1.0f - position, second_exponent);
	const float denominator =
		std::pow(shoulder_fraction, first_exponent) *
		std::pow(1.0f - shoulder_fraction, second_exponent);
	return denominator > 0.0f ? numerator / denominator : 0.0f;
}

} // namespace

std::optional<PlantOrganGeometrySource> FruitShellGeometryFactory::create(
	const PlantFruitSpecification &specification,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	if (!validate_fruit_shell(specification, diagnostic)) return std::nullopt;

	RevolveShapeSpecificationCandidate candidate;
	candidate.radial_profile.outer_loop.reserve(
		static_cast<std::size_t>(specification.profileSegments() + 1));
	for (int profile_index = 0;
	     profile_index <= specification.profileSegments();
	     ++profile_index) {
		const float position =
			static_cast<float>(profile_index) /
			static_cast<float>(specification.profileSegments());
		const float radius = specification.maximumRadius() * normalized_radius(
			position, specification.shoulderFraction(), specification.fullness());
		candidate.radial_profile.outer_loop.emplace_back(
			std::max(0.0f, radius), specification.height() * position);
	}
	candidate.angular_segments = specification.radialSegments();
	candidate.detail_level = detail_level;

	auto shape = RevolveSpecificationValidator(complexity_limits_).validate(
		std::move(candidate), diagnostic);
	if (!shape) return std::nullopt;
	GeometryBuildResult result =
		RevolveMeshGenerator(complexity_limits_).build(*shape);
	if (!result.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = result.firstDiagnostic();
		return std::nullopt;
	}

	std::vector<MeshSurfaceTag> surface_tags(
		result.generatedMesh().faceSurfaceTags().size(),
		MeshSurfaceTag(MeshSurfaceRole::BotanicalFruitOuter));
	GeneratedPrimitiveMesh fruit_mesh(
		result.generatedMesh().mesh(), std::move(surface_tags));
	if (diagnostic != nullptr) diagnostic->clear();
	return PlantOrganGeometrySource(
		std::move(shape), std::move(fruit_mesh),
		"InstancedOrganArrayFruitShells", "fruitwarmmattepaint");
}
