#pragma once

#include "vegetation/model/SurfaceAttachmentMode.h"
#include "vegetation/model/VegetationSurfaceTarget.h"

#include <utility>

class SurfaceAttachmentSpecification
{
public:
	SurfaceAttachmentSpecification(
		VegetationSurfaceTarget target,
		float attachment_distance,
		float tolerance,
		SurfaceAttachmentMode mode)
		: target_(std::move(target)),
		  attachment_distance_(attachment_distance),
		  tolerance_(tolerance),
		  mode_(mode)
	{
	}

	const VegetationSurfaceTarget &target() const { return target_; }
	float attachmentDistance() const { return attachment_distance_; }
	float tolerance() const { return tolerance_; }
	SurfaceAttachmentMode mode() const { return mode_; }

private:
	VegetationSurfaceTarget target_;
	float attachment_distance_ = 0.0f;
	float tolerance_ = 0.001f;
	SurfaceAttachmentMode mode_ = SurfaceAttachmentMode::Contact;
};
