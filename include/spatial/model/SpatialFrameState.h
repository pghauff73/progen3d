#pragma once

#include "spatial/model/SpatialFrameReference.h"
#include "spatial/model/SpatialPoseUncertainty.h"

#include <glm/glm.hpp>

#include <utility>

class SpatialFrameState {
public:
	SpatialFrameState(glm::mat4 authored_local_transform,
	                  glm::mat4 resolution_local_transform,
	                  glm::mat4 resolved_world_transform,
	                  SpatialFrameReference parent_frame,
	                  SpatialPoseUncertainty uncertainty)
		: authored_local_transform_(authored_local_transform),
		  resolution_local_transform_(resolution_local_transform),
		  resolved_world_transform_(resolved_world_transform),
		  parent_frame_(std::move(parent_frame)),
		  uncertainty_(uncertainty) {}

	const glm::mat4 &authoredLocalTransform() const { return authored_local_transform_; }
	const glm::mat4 &resolutionLocalTransform() const { return resolution_local_transform_; }
	const glm::mat4 &resolvedWorldTransform() const { return resolved_world_transform_; }
	const SpatialFrameReference &parentFrame() const { return parent_frame_; }
	const SpatialPoseUncertainty &uncertainty() const { return uncertainty_; }

	SpatialFrameState withResolvedPlacement(const glm::mat4 &resolution_local_transform,
	                                        const glm::mat4 &resolved_world_transform) const
	{
		return SpatialFrameState(
			authored_local_transform_,
			resolution_local_transform,
			resolved_world_transform,
			parent_frame_,
			uncertainty_);
	}

private:
	glm::mat4 authored_local_transform_{1.0f};
	glm::mat4 resolution_local_transform_{1.0f};
	glm::mat4 resolved_world_transform_{1.0f};
	SpatialFrameReference parent_frame_ = SpatialFrameReference::root();
	SpatialPoseUncertainty uncertainty_;
};
