#include "mechanics/service/MemberServiceAnalysis.h"
#include "spatial/model/SpatialBuildingObject.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr double gravityMps2 = 9.80665;
constexpr double frameTolerance = 1e-5;

bool isSupport(const SpatialInterface &face)
{
	return face.type() == SpatialInterfaceType::Seat ||
	       face.type() == SpatialInterfaceType::Support ||
	       face.type() == SpatialInterfaceType::Bearing;
}

bool isPositive(double value) { return std::isfinite(value) && value > 0; }
bool isNonnegative(double value) { return std::isfinite(value) && value >= 0; }

// The first method admits rigid placements and uniform scale. Shear and unequal
// scales would require transforming each section and support patch individually.
double uniformScale(const glm::dmat3 &basis)
{
	const double scale = glm::length(basis[0]);
	if (!isPositive(scale)) return 0;
	const glm::dmat3 unit = basis / scale;
	const glm::dmat3 gram = glm::transpose(unit) * unit;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			if (!(std::abs(gram[i][j] - (i == j ? 1.0 : 0.0)) <= frameTolerance)) return 0;
	return scale;
}

double seatAreaM2(const SpatialInterface &seat, double metres)
{
	return seat.region().primaryExtent() * metres * seat.region().secondaryExtent() * metres;
}
}

double MemberServiceResponse::deflectionM(double station) const
{
	const double x = std::min(station, 1.0 - station) * spanM;
	const double span2 = spanM * spanM;
	const double uniform = totalLineLoadNPerM * x *
		(spanM * span2 - 2 * spanM * x * x + x * x * x) / 24;
	const double point = centreLoadN * x * (3 * span2 - 4 * x * x) / 48;
	return (uniform + point) / flexuralRigidityNm2;
}

MemberServiceAnalysis::MemberServiceAnalysis(const SpatialBuildingObject &object)
{
	if (!object.pinRollerMember()) return;
	const PinRollerMember &member = *object.pinRollerMember();
	const auto *pin = object.findInterface(SpatialInterfaceId(member.pinInterface));
	const auto *roller = object.findInterface(SpatialInterfaceId(member.rollerInterface));
	if (!pin || !roller || pin == roller || !isSupport(*pin) || !isSupport(*roller)) {
		diagnostic_ = "Choose two distinct, existing seat, bearing or support interfaces.";
		return;
	}
	if (std::count_if(object.interfaces().begin(), object.interfaces().end(), isSupport) != 2) {
		diagnostic_ = "Additional support interfaces require a different load-sharing model.";
		return;
	}
	if (pin->region().kind() != SpatialInterfaceRegionKind::PlaneRectangle ||
	    roller->region().kind() != SpatialInterfaceRegionKind::PlaneRectangle) {
		diagnostic_ = "Each support needs a rectangular contact area.";
		return;
	}
	const glm::dmat3 basis(object.frameState().resolvedWorldTransform());
	const double metres = member.metresPerUnit * uniformScale(basis);
	if (!isPositive(metres)) {
		diagnostic_ = "Use positive metres per unit and a rigid or uniformly scaled object frame.";
		return;
	}
	const glm::dvec3 run = glm::dvec3(roller->localFrame().localOrigin()) - glm::dvec3(pin->localFrame().localOrigin());
	const double span = glm::length(run) * metres;
	const glm::dvec3 normal = glm::normalize(glm::dvec3(pin->localFrame().localNormal()));
	const glm::dvec3 otherNormal = glm::normalize(glm::dvec3(roller->localFrame().localNormal()));
	if (!isPositive(span) || !(glm::dot(normal, otherNormal) >= 1.0 - frameTolerance) ||
	    !(std::abs(glm::dot(normal, glm::normalize(run))) <= frameTolerance)) {
		diagnostic_ = "Seat centres must define a nonzero span transverse to parallel, equally directed seat normals.";
		return;
	}
	const glm::dvec3 worldNormal = glm::normalize(basis * normal);
	if (!(std::abs(worldNormal.y) >= 1.0 - frameTolerance)) {
		diagnostic_ = "This gravity load case requires level seats in a Y-up world.";
		return;
	}
	const double breadth = member.breadth * metres, depth = member.depth * metres, wall = member.wall * metres;
	if (!isPositive(breadth) || !isPositive(depth) || !isNonnegative(wall) ||
	    2 * wall >= std::min(breadth, depth)) {
		diagnostic_ = "Use a positive rectangular section with zero wall for solid or a valid hollow wall.";
		return;
	}
	if (!isPositive(member.modulusPa) || !isPositive(member.densityKgM3) ||
	    !isNonnegative(member.lineLoadNPerM) || !isNonnegative(member.distributedPayloadKg) ||
	    !isNonnegative(member.centreLoadN)) {
		diagnostic_ = "Modulus and density must be positive and downward service loads nonnegative.";
		return;
	}
	MemberServiceResponse result;
	result.spanM = span;
	result.areaM2 = breadth * depth;
	result.secondMomentM4 = breadth * std::pow(depth, 3) / 12;
	if (wall > 0) {
		// Two webs and two flanges avoid subtracting nearly equal section properties.
		const double clearDepth = depth - 2 * wall;
		result.areaM2 = 2 * wall * (breadth + clearDepth);
		result.secondMomentM4 = 2 * wall * std::pow(clearDepth, 3) / 12 +
			breadth * wall * (depth * depth + depth * clearDepth + clearDepth * clearDepth) / 6;
	}
	result.massKg = result.areaM2 * span * member.densityKgM3;
	result.flexuralRigidityNm2 = member.modulusPa * result.secondMomentM4;
	result.totalLineLoadNPerM = member.lineLoadNPerM +
		gravityMps2 * (result.massKg + member.distributedPayloadKg) / span;
	result.centreLoadN = member.centreLoadN;
	result.reactionPerSeatN = (result.totalLineLoadNPerM * span + member.centreLoadN) / 2;
	result.peakMomentNm = result.totalLineLoadNPerM * span * span / 8 + member.centreLoadN * span / 4;
	result.bendingStressPa = result.peakMomentNm * depth / (2 * result.secondMomentM4);
	result.pinPressurePa = result.reactionPerSeatN / seatAreaM2(*pin, metres);
	result.rollerPressurePa = result.reactionPerSeatN / seatAreaM2(*roller, metres);
	result.midspanDeflectionM = result.deflectionM(0.5);
	for (double value : {result.areaM2, result.secondMomentM4, result.massKg, result.flexuralRigidityNm2,
	                     result.totalLineLoadNPerM, result.reactionPerSeatN, result.peakMomentNm,
	                     result.bendingStressPa, result.pinPressurePa, result.rollerPressurePa,
	                     result.midspanDeflectionM}) {
		if (!isPositive(value)) {
			diagnostic_ = "Section, contact area or response is outside the numerical range.";
			return;
		}
	}
	response_ = result;
}
