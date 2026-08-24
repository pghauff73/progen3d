#include "geometry/service/LoftSpecificationValidator.h"

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

bool finite(const glm::vec2 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y);
}

std::string shape_name(ShapeFamily family)
{
	switch (family) {
	case ShapeFamily::Loft:
		return "Loft";
	case ShapeFamily::SurfaceLoft:
		return "SurfaceLoft";
	case ShapeFamily::ShellLoft:
		return "ShellLoft";
	default:
		return "Loft";
	}
}

bool profiles_correspond(
	const Profile2D &first,
	const Profile2D &second)
{
	if (first.outerLoop().points().size() !=
	    second.outerLoop().points().size()) {
		return false;
	}
	if (first.innerLoops().size() != second.innerLoops().size()) return false;
	for (std::size_t loop_index = 0;
	     loop_index < first.innerLoops().size();
	     ++loop_index) {
		if (first.innerLoops()[loop_index].points().size() !=
		    second.innerLoops()[loop_index].points().size()) {
			return false;
		}
	}
	return true;
}

void append_profile_key(std::ostringstream *key, const Profile2D &profile)
{
	*key << "outer=";
	for (const glm::vec2 &point : profile.outerLoop().points()) {
		*key << float_bits(point.x) << "," << float_bits(point.y) << ";";
	}
	for (const ProfileLoop2D &inner_loop : profile.innerLoops()) {
		*key << "hole=";
		for (const glm::vec2 &point : inner_loop.points()) {
			*key << float_bits(point.x) << "," << float_bits(point.y) << ";";
		}
	}
}

std::shared_ptr<const LoftShapeSpecification> validate_sections(
	std::vector<LoftSectionCandidate> candidates,
	ExtrudeProfileCapPolicy cap_policy,
	const GeometryComplexityLimits &complexity_limits,
	ShapeFamily family,
	GeometryDetailLevel detail_level,
	std::string *diagnostic)
{
	if (candidates.size() < 2u) {
		if (diagnostic != nullptr) *diagnostic = "Loft requires at least two sections.";
		return {};
	}
	if (candidates.size() > complexity_limits.maximumLoftSections()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Loft exceeds the configured section limit of " +
			              std::to_string(complexity_limits.maximumLoftSections()) + ".";
		}
		return {};
	}

	std::vector<LoftSectionSpecification> sections;
	sections.reserve(candidates.size());
	for (std::size_t section_index = 0;
	     section_index < candidates.size();
	     ++section_index) {
		LoftSectionCandidate &candidate = candidates[section_index];
		if (!std::isfinite(candidate.axial_position) ||
		    !finite(candidate.center) || !finite(candidate.scale) ||
		    !std::isfinite(candidate.rotation_degrees) ||
		    candidate.scale.x == 0.0f || candidate.scale.y == 0.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "Loft section transforms must be finite with non-zero scale.";
			}
			return {};
		}
		if (section_index > 0u &&
		    candidate.axial_position <= sections.back().axialPosition()) {
			if (diagnostic != nullptr) {
				*diagnostic = "Loft section positions must be strictly increasing.";
			}
			return {};
		}
		std::shared_ptr<const Profile2D> profile =
			Profile2DValidator(complexity_limits).validate(
				std::move(candidate.profile), diagnostic);
		if (!profile) return {};
		if (!sections.empty() &&
		    !profiles_correspond(sections.front().profile(), *profile)) {
			if (diagnostic != nullptr) {
				*diagnostic = "Loft v1 requires corresponding loop and vertex counts across sections.";
			}
			return {};
		}
		sections.emplace_back(
			candidate.axial_position,
			*profile,
			candidate.center,
			candidate.scale,
			candidate.rotation_degrees);
	}

	std::ostringstream key;
	key << shape_name(family)
	    << ":v1:cap=" << cap_policy.canonicalText();
	for (const LoftSectionSpecification &section : sections) {
		key << ":section=" << float_bits(section.axialPosition()) << ","
		    << float_bits(section.center().x) << ","
		    << float_bits(section.center().y) << ","
		    << float_bits(section.scale().x) << ","
		    << float_bits(section.scale().y) << ","
		    << float_bits(section.rotationDegrees()) << ":";
		append_profile_key(&key, section.profile());
	}
	const std::string canonical_text =
		shape_name(family) +
		std::string("(sections(") + std::to_string(sections.size()) +
		") cap(" + cap_policy.canonicalText() + "))";

	if (family == ShapeFamily::ShellLoft) {
		return std::make_shared<const ShellLoftShapeSpecification>(
			std::move(sections), cap_policy,
			ShapeSpecificationKey(key.str()), canonical_text, detail_level);
	}
	if (family == ShapeFamily::SurfaceLoft) {
		return std::make_shared<const SurfaceLoftShapeSpecification>(
			std::move(sections), ShapeSpecificationKey(key.str()),
			canonical_text, detail_level);
	}
	return std::make_shared<const LoftShapeSpecification>(
		std::move(sections), cap_policy,
		ShapeSpecificationKey(key.str()), canonical_text,
		ShapeFamily::Loft, detail_level);
}

}

std::shared_ptr<const LoftShapeSpecification>
LoftSpecificationValidator::validateLoft(
	LoftShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	return validate_sections(
		std::move(candidate.sections), candidate.cap_policy,
		complexity_limits_, ShapeFamily::Loft, candidate.detail_level, diagnostic);
}

std::shared_ptr<const SurfaceLoftShapeSpecification>
LoftSpecificationValidator::validateSurfaceLoft(
	LoftShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	std::shared_ptr<const LoftShapeSpecification> validated = validate_sections(
		std::move(candidate.sections), ExtrudeProfileCapPolicy::createNone(),
		complexity_limits_, ShapeFamily::SurfaceLoft,
		candidate.detail_level, diagnostic);
	return std::dynamic_pointer_cast<const SurfaceLoftShapeSpecification>(validated);
}

std::shared_ptr<const ShellLoftShapeSpecification>
LoftSpecificationValidator::validateShellLoft(
	ShellLoftShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	std::vector<LoftSectionCandidate> loft_sections;
	loft_sections.reserve(candidate.sections.size());
	for (ShellLoftSectionCandidate &shell_section : candidate.sections) {
		LoftSectionCandidate loft_section;
		loft_section.axial_position = shell_section.axial_position;
		loft_section.profile.outer_loop = std::move(shell_section.outer_loop);
		loft_section.profile.inner_loops.push_back(
			std::move(shell_section.inner_loop));
		loft_section.center = shell_section.center;
		loft_section.scale = shell_section.scale;
		loft_section.rotation_degrees = shell_section.rotation_degrees;
		loft_sections.push_back(std::move(loft_section));
	}
	std::shared_ptr<const LoftShapeSpecification> validated = validate_sections(
		std::move(loft_sections), candidate.cap_policy,
		complexity_limits_, ShapeFamily::ShellLoft,
		candidate.detail_level, diagnostic);
	return std::dynamic_pointer_cast<const ShellLoftShapeSpecification>(validated);
}
