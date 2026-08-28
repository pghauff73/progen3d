#pragma once

#include <glm/glm.hpp>

enum class ClipRetainedSide
{
	Positive,
	Negative
};

enum class PlaneClipSemantic
{
	Explicit,
	CylinderChord,
	SphereSlabMinimum,
	SphereSlabMaximum,
	Alias
};

class PlaneClipSpecification
{
public:
	PlaneClipSpecification(glm::vec3 normal,
	                       float normalized_offset,
	                       ClipRetainedSide retained_side,
	                       PlaneClipSemantic semantic = PlaneClipSemantic::Explicit)
		: normal_(normal),
		  normalized_offset_(normalized_offset),
		  retained_side_(retained_side),
		  semantic_(semantic)
	{
	}

	const glm::vec3 &normal() const
	{
		return normal_;
	}

	float normalizedOffset() const
	{
		return normalized_offset_;
	}

	ClipRetainedSide retainedSide() const
	{
		return retained_side_;
	}

	PlaneClipSemantic semantic() const
	{
		return semantic_;
	}

private:
	glm::vec3 normal_{0.0f, 1.0f, 0.0f};
	float normalized_offset_ = 0.0f;
	ClipRetainedSide retained_side_ = ClipRetainedSide::Positive;
	PlaneClipSemantic semantic_ = PlaneClipSemantic::Explicit;
};
