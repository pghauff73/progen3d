#pragma once

#include "vegetation/model/CrownVolumeKind.h"

#include <glm/glm.hpp>

#include <utility>
#include <vector>

class CrownVolumeSpecification
{
public:
	static CrownVolumeSpecification sphere(glm::vec3 center, float radius)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::Sphere, center, glm::vec3(radius), 0, 0.0f,
			{}, 0.0f);
	}

	static CrownVolumeSpecification ellipsoid(
		glm::vec3 center,
		glm::vec3 radii)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::Ellipsoid, center, radii, 0, 0.0f, {}, 0.0f);
	}

	static CrownVolumeSpecification cone(
		glm::vec3 base_center,
		glm::vec2 base_radii,
		float height)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::Cone, base_center,
			glm::vec3(base_radii.x, height, base_radii.y), 0, 0.0f, {},
			0.0f);
	}

	static CrownVolumeSpecification inverseCone(
		glm::vec3 base_center,
		glm::vec2 top_radii,
		float height)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::InverseCone, base_center,
			glm::vec3(top_radii.x, height, top_radii.y), 0, 0.0f, {},
			0.0f);
	}

	static CrownVolumeSpecification cylinder(
		glm::vec3 base_center,
		glm::vec2 radii,
		float height)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::Cylinder, base_center,
			glm::vec3(radii.x, height, radii.y), 0, 0.0f, {}, 0.0f);
	}

	static CrownVolumeSpecification dome(
		glm::vec3 base_center,
		glm::vec3 radii)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::Dome, base_center, radii, 0, 0.0f, {}, 0.0f);
	}

	static CrownVolumeSpecification lobed(
		glm::vec3 center,
		glm::vec3 radii,
		int lobe_count,
		float lobe_amplitude)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::Lobed, center, radii, lobe_count,
			lobe_amplitude, {}, 0.0f);
	}

	static CrownVolumeSpecification customSampled(
		std::vector<glm::vec3> samples,
		float sample_radius)
	{
		return CrownVolumeSpecification(
			CrownVolumeKind::CustomSampled, glm::vec3(0.0f), glm::vec3(0.0f),
			0, 0.0f, std::move(samples), sample_radius);
	}

	CrownVolumeKind kind() const { return kind_; }
	const glm::vec3 &origin() const { return origin_; }
	const glm::vec3 &dimensions() const { return dimensions_; }
	int lobeCount() const { return lobe_count_; }
	float lobeAmplitude() const { return lobe_amplitude_; }
	const std::vector<glm::vec3> &customSamples() const
	{
		return custom_samples_;
	}
	float customSampleRadius() const { return custom_sample_radius_; }

private:
	CrownVolumeSpecification(
		CrownVolumeKind kind,
		glm::vec3 origin,
		glm::vec3 dimensions,
		int lobe_count,
		float lobe_amplitude,
		std::vector<glm::vec3> custom_samples,
		float custom_sample_radius)
		: kind_(kind),
		  origin_(origin),
		  dimensions_(dimensions),
		  lobe_count_(lobe_count),
		  lobe_amplitude_(lobe_amplitude),
		  custom_samples_(std::move(custom_samples)),
		  custom_sample_radius_(custom_sample_radius)
	{
	}

	CrownVolumeKind kind_ = CrownVolumeKind::Sphere;
	glm::vec3 origin_{0.0f};
	glm::vec3 dimensions_{1.0f};
	int lobe_count_ = 0;
	float lobe_amplitude_ = 0.0f;
	std::vector<glm::vec3> custom_samples_;
	float custom_sample_radius_ = 0.0f;
};
