#include "spatial/service/SpatialInterfaceCompatibilityService.h"

#include <cmath>

namespace {

bool types_are_compatible(SpatialInterfaceType source, SpatialInterfaceType target)
{
	if (source == target) {
		return source != SpatialInterfaceType::InspectionInterface;
	}
	return (source == SpatialInterfaceType::Support &&
	        target == SpatialInterfaceType::Bearing) ||
	       (source == SpatialInterfaceType::Bearing &&
	        target == SpatialInterfaceType::Support) ||
	       (source == SpatialInterfaceType::Insert &&
	        target == SpatialInterfaceType::Socket) ||
	       (source == SpatialInterfaceType::Socket &&
	        target == SpatialInterfaceType::Insert) ||
	       (source == SpatialInterfaceType::Shaft &&
	        target == SpatialInterfaceType::Socket) ||
	       (source == SpatialInterfaceType::Socket &&
	        target == SpatialInterfaceType::Shaft) ||
	       (source == SpatialInterfaceType::Fastener &&
	        target == SpatialInterfaceType::Anchor) ||
	       (source == SpatialInterfaceType::Anchor &&
	        target == SpatialInterfaceType::Fastener);
}

bool shapes_are_compatible(InterfaceShape source, InterfaceShape target)
{
	return source == InterfaceShape::Unspecified ||
	       target == InterfaceShape::Unspecified || source == target;
}

bool genders_are_compatible(InterfaceGender source, InterfaceGender target)
{
	if (source == InterfaceGender::Unspecified || target == InterfaceGender::Unspecified) {
		return true;
	}
	if (source == InterfaceGender::Neutral || target == InterfaceGender::Neutral) {
		return source == target;
	}
	return (source == InterfaceGender::Male && target == InterfaceGender::Female) ||
	       (source == InterfaceGender::Female && target == InterfaceGender::Male);
}

bool optional_dimensions_match(const std::optional<float> &source,
	                           const std::optional<float> &target,
	                           float tolerance)
{
	return !source.has_value() || !target.has_value() ||
	       std::fabs(*source - *target) <= tolerance;
}

} // namespace

SpatialInterfaceCompatibilityResult SpatialInterfaceCompatibilityService::evaluate(
	const SpatialInterface &source,
	const SpatialInterface &target,
	float dimensional_tolerance) const
{
	if (!std::isfinite(dimensional_tolerance) || dimensional_tolerance < 0.0f) {
		return SpatialInterfaceCompatibilityResult::incompatible(
			"Interface compatibility tolerance must be finite and nonnegative.");
	}
	if (!types_are_compatible(source.type(), target.type())) {
		return SpatialInterfaceCompatibilityResult::incompatible(
			"Interface types are not compatible.");
	}

	const InterfaceCompatibilityProfile &source_profile = source.compatibility();
	const InterfaceCompatibilityProfile &target_profile = target.compatibility();
	if (!shapes_are_compatible(source_profile.shape(), target_profile.shape())) {
		return SpatialInterfaceCompatibilityResult::incompatible(
			"Interface shapes are not compatible.");
	}
	if (!genders_are_compatible(source_profile.gender(), target_profile.gender())) {
		return SpatialInterfaceCompatibilityResult::incompatible(
			"Interface genders are not compatible.");
	}
	if (!source_profile.connectionFamily().empty() &&
	    !target_profile.connectionFamily().empty() &&
	    source_profile.connectionFamily() != target_profile.connectionFamily()) {
		return SpatialInterfaceCompatibilityResult::incompatible(
			"Interface connection families do not match.");
	}
	if (!optional_dimensions_match(source_profile.nominalDiameter(),
	                              target_profile.nominalDiameter(),
	                              dimensional_tolerance)) {
		return SpatialInterfaceCompatibilityResult::incompatible(
			"Interface nominal diameters exceed tolerance.");
	}
	if (!optional_dimensions_match(source_profile.nominalWidth(),
	                              target_profile.nominalWidth(),
	                              dimensional_tolerance) ||
	    !optional_dimensions_match(source_profile.nominalHeight(),
	                              target_profile.nominalHeight(),
	                              dimensional_tolerance)) {
		return SpatialInterfaceCompatibilityResult::incompatible(
			"Interface rectangular dimensions exceed tolerance.");
	}
	return SpatialInterfaceCompatibilityResult::compatible();
}
