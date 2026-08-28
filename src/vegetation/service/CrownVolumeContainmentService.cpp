#include "vegetation/service/CrownVolumeContainmentService.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool valid_kind(CrownVolumeKind kind)
{
	const int value = static_cast<int>(kind);
	return value >= static_cast<int>(CrownVolumeKind::Sphere) &&
	       value <= static_cast<int>(CrownVolumeKind::CustomSampled);
}

AxisAlignedBounds bounds_from_minimum_and_maximum(
	const glm::vec3 &minimum,
	const glm::vec3 &maximum)
{
	AxisAlignedBounds bounds;
	bounds.min = minimum;
	bounds.max = maximum;
	bounds.center = (minimum + maximum) * 0.5f;
	bounds.half_extents = (maximum - minimum) * 0.5f;
	bounds.valid = true;
	return bounds;
}

float elliptical_radius_squared(
	const glm::vec3 &point,
	const glm::vec3 &origin,
	float radius_x,
	float radius_z)
{
	const float x = (point.x - origin.x) / radius_x;
	const float z = (point.z - origin.z) / radius_z;
	return x * x + z * z;
}

} // namespace

bool CrownVolumeContainmentService::validate(
	const CrownVolumeSpecification &volume,
	std::string *diagnostic) const
{
	if (!valid_kind(volume.kind())) {
		if (diagnostic != nullptr) *diagnostic = "Crown volume kind is invalid.";
		return false;
	}
	if (volume.kind() == CrownVolumeKind::CustomSampled) {
		if (!std::isfinite(volume.customSampleRadius()) ||
		    volume.customSampleRadius() <= 0.0f ||
		    volume.customSamples().empty()) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"A custom sampled crown requires samples and a positive finite radius.";
			}
			return false;
		}
		for (const glm::vec3 &sample : volume.customSamples()) {
			if (!finite(sample)) {
				if (diagnostic != nullptr) {
					*diagnostic = "Custom crown samples must be finite.";
				}
				return false;
			}
		}
		return true;
	}

	if (!finite(volume.origin()) || !finite(volume.dimensions()) ||
	    volume.dimensions().x <= 0.0f ||
	    volume.dimensions().y <= 0.0f ||
	    volume.dimensions().z <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Crown volume dimensions must be positive and finite.";
		}
		return false;
	}
	if (volume.kind() == CrownVolumeKind::Lobed &&
	    (volume.lobeCount() < 2 || !std::isfinite(volume.lobeAmplitude()) ||
	     volume.lobeAmplitude() < 0.0f || volume.lobeAmplitude() >= 0.5f)) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"A lobed crown requires at least two lobes and amplitude in [0, 0.5).";
		}
		return false;
	}
	return true;
}

bool CrownVolumeContainmentService::contains(
	const CrownVolumeSpecification &volume,
	const glm::vec3 &point) const
{
	if (!finite(point) || !validate(volume)) return false;
	const glm::vec3 delta = point - volume.origin();
	const glm::vec3 dimensions = volume.dimensions();
	switch (volume.kind()) {
	case CrownVolumeKind::Sphere:
	case CrownVolumeKind::Ellipsoid: {
		const glm::vec3 normalized = delta / dimensions;
		return glm::dot(normalized, normalized) <= 1.0f + 1.0e-6f;
	}
	case CrownVolumeKind::Cone: {
		if (delta.y < 0.0f || delta.y > dimensions.y) return false;
		const float scale = 1.0f - delta.y / dimensions.y;
		return elliptical_radius_squared(
			point, volume.origin(), dimensions.x, dimensions.z) <=
			scale * scale + 1.0e-6f;
	}
	case CrownVolumeKind::InverseCone: {
		if (delta.y < 0.0f || delta.y > dimensions.y) return false;
		const float scale = delta.y / dimensions.y;
		return elliptical_radius_squared(
			point, volume.origin(), dimensions.x, dimensions.z) <=
			scale * scale + 1.0e-6f;
	}
	case CrownVolumeKind::Cylinder:
		return delta.y >= 0.0f && delta.y <= dimensions.y &&
		       elliptical_radius_squared(
			       point, volume.origin(), dimensions.x, dimensions.z) <=
			       1.0f + 1.0e-6f;
	case CrownVolumeKind::Dome: {
		if (delta.y < 0.0f || delta.y > dimensions.y) return false;
		const glm::vec3 normalized = delta / dimensions;
		return glm::dot(normalized, normalized) <= 1.0f + 1.0e-6f;
	}
	case CrownVolumeKind::Lobed: {
		const glm::vec3 normalized = delta / dimensions;
		const float radial_distance = glm::length(normalized);
		if (radial_distance <= 1.0e-7f) return true;
		const float azimuth = std::atan2(normalized.z, normalized.x);
		const float modulation = 1.0f + volume.lobeAmplitude() *
			std::cos(static_cast<float>(volume.lobeCount()) * azimuth);
		return radial_distance <= modulation + 1.0e-6f;
	}
	case CrownVolumeKind::CustomSampled:
		for (const glm::vec3 &sample : volume.customSamples()) {
			if (glm::distance(point, sample) <=
			    volume.customSampleRadius() + 1.0e-6f) {
				return true;
			}
		}
		return false;
	}
	return false;
}

AxisAlignedBounds CrownVolumeContainmentService::bounds(
	const CrownVolumeSpecification &volume) const
{
	if (!validate(volume)) return {};
	if (volume.kind() == CrownVolumeKind::CustomSampled) {
		glm::vec3 minimum(std::numeric_limits<float>::max());
		glm::vec3 maximum(std::numeric_limits<float>::lowest());
		for (const glm::vec3 &sample : volume.customSamples()) {
			minimum = glm::min(minimum, sample);
			maximum = glm::max(maximum, sample);
		}
		const glm::vec3 expansion(volume.customSampleRadius());
		return bounds_from_minimum_and_maximum(
			minimum - expansion, maximum + expansion);
	}

	glm::vec3 minimum;
	glm::vec3 maximum;
	if (volume.kind() == CrownVolumeKind::Cone ||
	    volume.kind() == CrownVolumeKind::InverseCone ||
	    volume.kind() == CrownVolumeKind::Cylinder ||
	    volume.kind() == CrownVolumeKind::Dome) {
		minimum = volume.origin() + glm::vec3(
			-volume.dimensions().x, 0.0f, -volume.dimensions().z);
		maximum = volume.origin() + glm::vec3(
			volume.dimensions().x, volume.dimensions().y,
			volume.dimensions().z);
	}
	else {
		const float expansion = volume.kind() == CrownVolumeKind::Lobed
			? 1.0f + volume.lobeAmplitude()
			: 1.0f;
		const glm::vec3 radii = volume.dimensions() * expansion;
		minimum = volume.origin() - radii;
		maximum = volume.origin() + radii;
	}
	return bounds_from_minimum_and_maximum(minimum, maximum);
}
