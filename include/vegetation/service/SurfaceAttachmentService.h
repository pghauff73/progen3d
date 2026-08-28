#pragma once

#include "vegetation/model/SurfaceAttachmentResolution.h"
#include "vegetation/model/SurfaceAttachmentSpecification.h"

#include <glm/glm.hpp>

#include <string>

class SurfaceAttachmentService
{
public:
	bool validate(
		const SurfaceAttachmentSpecification &specification,
		std::string *diagnostic = nullptr) const;
	SurfaceAttachmentResolution attachNearest(
		glm::vec3 source_point,
		const SurfaceAttachmentSpecification &specification) const;
	SurfaceAttachmentResolution firstContact(
		glm::vec3 segment_start,
		glm::vec3 segment_end,
		const SurfaceAttachmentSpecification &specification) const;
};
