#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class SurfaceAttachmentResolution
{
public:
	static SurfaceAttachmentResolution succeeded(
		glm::vec3 surface_point,
		glm::vec3 attached_point,
		glm::vec3 surface_normal,
		float source_distance)
	{
		return SurfaceAttachmentResolution(
			true, surface_point, attached_point, surface_normal,
			source_distance, {});
	}

	static SurfaceAttachmentResolution failed(std::string diagnostic)
	{
		return SurfaceAttachmentResolution(
			false, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f),
			0.0f, std::move(diagnostic));
	}

	bool succeeded() const { return succeeded_; }
	const glm::vec3 &surfacePoint() const { return surface_point_; }
	const glm::vec3 &attachedPoint() const { return attached_point_; }
	const glm::vec3 &surfaceNormal() const { return surface_normal_; }
	float sourceDistance() const { return source_distance_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	SurfaceAttachmentResolution(
		bool succeeded,
		glm::vec3 surface_point,
		glm::vec3 attached_point,
		glm::vec3 surface_normal,
		float source_distance,
		std::string diagnostic)
		: succeeded_(succeeded),
		  surface_point_(surface_point),
		  attached_point_(attached_point),
		  surface_normal_(surface_normal),
		  source_distance_(source_distance),
		  diagnostic_(std::move(diagnostic))
	{
	}

	bool succeeded_ = false;
	glm::vec3 surface_point_{0.0f};
	glm::vec3 attached_point_{0.0f};
	glm::vec3 surface_normal_{0.0f};
	float source_distance_ = 0.0f;
	std::string diagnostic_;
};
