#include "geometry/service/RoundedBoxShapeSpecificationFactory.h"

#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/Profile2DFactory.h"

#include <cmath>
#include <utility>

std::shared_ptr<const ExtrudeProfileShapeSpecification>
RoundedBoxShapeSpecificationFactory::create(
	const RoundedBoxSpecification &specification,
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	if (!std::isfinite(specification.width()) ||
	    !std::isfinite(specification.height()) ||
	    !std::isfinite(specification.depth()) ||
	    !std::isfinite(specification.cornerRadius()) ||
	    specification.width() <= 0.0f || specification.height() <= 0.0f ||
	    specification.depth() <= 0.0f || specification.cornerRadius() <= 0.0f ||
	    specification.cornerRadius() * 2.0f >= specification.width() ||
	    specification.cornerRadius() * 2.0f >= specification.height() ||
	    specification.segmentsPerCorner() < 1 ||
	    specification.segmentsPerCorner() > 64) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"RoundedBox requires finite positive dimensions, a contained corner radius, and 1-64 corner segments.";
		}
		return {};
	}

	auto profile = Profile2DFactory(complexity_limits_).createRoundedRectangle(
		specification.width(),
		specification.height(),
		specification.cornerRadius(),
		specification.segmentsPerCorner(),
		diagnostic);
	if (!profile) return {};
	ExtrudeProfileShapeSpecificationCandidate candidate;
	candidate.profile.outer_loop = profile->outerLoop().points();
	candidate.depth = specification.depth();
	candidate.detail_level = detail_level;
	return ExtrudeProfileSpecificationValidator(complexity_limits_).validate(
		std::move(candidate), diagnostic);
}
