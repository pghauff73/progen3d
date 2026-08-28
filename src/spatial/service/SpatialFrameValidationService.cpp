#include "spatial/service/SpatialFrameValidationService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>

namespace {

constexpr float kFrameEpsilon = 0.000001f;

bool is_finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool is_finite(const glm::mat4 &value)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(value[column][row])) return false;
		}
	}
	return true;
}

bool is_affine(const glm::mat4 &value)
{
	return std::fabs(value[0][3]) <= kFrameEpsilon &&
	       std::fabs(value[1][3]) <= kFrameEpsilon &&
	       std::fabs(value[2][3]) <= kFrameEpsilon &&
	       std::fabs(value[3][3] - 1.0f) <= kFrameEpsilon;
}

bool is_invertible(const glm::mat4 &value)
{
	return std::fabs(glm::determinant(glm::mat3(value))) > kFrameEpsilon;
}

void add_frame_issue(SpatialModelValidationReport *report,
	                 const SpatialBuildingObject &object,
	                 const std::string &detail)
{
	report->addIssue(SpatialModelValidationIssue(
		SpatialModelValidationCode::InvalidFrame,
		"Spatial object '" + object.identity().objectId().value() +
			"' has an invalid frame: " + detail,
		{object.identity().objectId()}));
}

} // namespace

SpatialModelValidationReport SpatialFrameValidationService::validateObjectFrame(
	const SpatialBuildingObject &object) const
{
	SpatialModelValidationReport report;
	const SpatialFrameState &frame = object.frameState();
	const glm::mat4 matrices[] = {
		frame.authoredLocalTransform(),
		frame.resolutionLocalTransform(),
		frame.resolvedWorldTransform()};
	const char *matrix_names[] = {
		"authored local transform",
		"resolution local transform",
		"resolved world transform"};

	for (std::size_t index = 0; index < 3; ++index) {
		if (!is_finite(matrices[index])) {
			add_frame_issue(&report, object, std::string(matrix_names[index]) +
				" contains a non-finite value.");
			continue;
		}
		if (!is_affine(matrices[index])) {
			add_frame_issue(&report, object, std::string(matrix_names[index]) +
				" is not affine.");
		}
		if (!is_invertible(matrices[index])) {
			add_frame_issue(&report, object, std::string(matrix_names[index]) +
				" is non-invertible.");
		}
	}

	const SpatialPoseUncertainty &uncertainty = frame.uncertainty();
	const glm::vec3 translation = uncertainty.translationStandardDeviation();
	const glm::vec3 rotation = uncertainty.rotationStandardDeviationDegrees();
	if (!is_finite(translation) || !is_finite(rotation) ||
	    translation.x < 0.0f || translation.y < 0.0f || translation.z < 0.0f ||
	    rotation.x < 0.0f || rotation.y < 0.0f || rotation.z < 0.0f) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::InvalidUncertainty,
			"Spatial object '" + object.identity().objectId().value() +
				"' has non-finite or negative pose uncertainty.",
			{object.identity().objectId()}));
	} else if (!uncertainty.isZero()) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::InvalidUncertainty,
			"Spatial object '" + object.identity().objectId().value() +
				"' uses nonzero pose uncertainty, which is not supported by SMB-OMv2 P0.",
			{object.identity().objectId()}));
	}

	if (!frame.parentFrame().isRoot() && frame.parentFrame().objectId().empty()) {
		add_frame_issue(&report, object, "the parent frame reference is empty.");
	}
	return report;
}

bool SpatialFrameValidationService::normalizeInterfaceFrame(
	const SpatialInterfaceFrame &input_frame,
	SpatialInterfaceFrame *normalized_frame,
	std::string *diagnostic) const
{
	if (normalized_frame == nullptr) {
		if (diagnostic != nullptr) *diagnostic = "Interface frame output is required.";
		return false;
	}
	if (!is_finite(input_frame.localOrigin()) ||
	    !is_finite(input_frame.localNormal()) ||
	    !is_finite(input_frame.localTangent())) {
		if (diagnostic != nullptr) *diagnostic = "Interface frame values must be finite.";
		return false;
	}

	const float normal_length = glm::length(input_frame.localNormal());
	const float tangent_length = glm::length(input_frame.localTangent());
	if (normal_length <= kFrameEpsilon || tangent_length <= kFrameEpsilon) {
		if (diagnostic != nullptr) {
			*diagnostic = "Interface normal and tangent must both be nonzero.";
		}
		return false;
	}

	const glm::vec3 normal = input_frame.localNormal() / normal_length;
	const glm::vec3 tangent_without_normal =
		input_frame.localTangent() -
		normal * glm::dot(input_frame.localTangent(), normal);
	const float orthogonal_tangent_length = glm::length(tangent_without_normal);
	if (orthogonal_tangent_length <= kFrameEpsilon) {
		if (diagnostic != nullptr) {
			*diagnostic = "Interface normal and tangent must not be parallel.";
		}
		return false;
	}

	*normalized_frame = SpatialInterfaceFrame(
		input_frame.localOrigin(),
		normal,
		tangent_without_normal / orthogonal_tangent_length);
	return true;
}
