#include "geometry/service/FormedPanelSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

std::string float_bits(float value)
{
	if (value == 0.0f) value = 0.0f;
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

std::string offset_side_name(ShellOffsetSide side)
{
	switch (side) {
	case ShellOffsetSide::Outward:
		return "outward";
	case ShellOffsetSide::Inward:
		return "inward";
	case ShellOffsetSide::Both:
		return "both";
	}
	return "both";
}

}

std::shared_ptr<const FormedPanelShapeSpecification>
FormedPanelSpecificationValidator::validate(
	FormedPanelShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.thickness) || candidate.thickness <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "FormedPanel thickness must be finite and greater than zero.";
		}
		return {};
	}

	LoftShapeSpecificationCandidate loft_candidate;
	loft_candidate.sections = std::move(candidate.sections);
	loft_candidate.cap_policy = ExtrudeProfileCapPolicy::createNone();
	loft_candidate.detail_level = candidate.detail_level;
	std::shared_ptr<const SurfaceLoftShapeSpecification> surface =
		LoftSpecificationValidator(complexity_limits_).validateSurfaceLoft(
			std::move(loft_candidate), diagnostic);
	if (!surface) return {};

	std::ostringstream key;
	key << "FormedPanel:v1:surface=" << surface->key().canonicalValue()
	    << ":thickness=" << float_bits(candidate.thickness)
	    << ":side=" << offset_side_name(candidate.offset_side);
	const std::string canonical_text =
		"FormedPanel(sections(" + std::to_string(surface->sections().size()) +
		") thickness(" + std::to_string(candidate.thickness) +
		") side(" + offset_side_name(candidate.offset_side) + "))";

	return std::make_shared<const FormedPanelShapeSpecification>(
		surface->sections(),
		candidate.thickness,
		candidate.offset_side,
		ShapeSpecificationKey(key.str()),
		canonical_text,
		candidate.detail_level);
}
