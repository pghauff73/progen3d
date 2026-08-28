#pragma once

#include "geometry/model/ShapeSpecification.h"

#include <memory>
#include <string>
#include <utility>

enum class MirrorShapeMode
{
	MirroredOnly,
	SourceAndMirrored
};

class MirrorShapeSpecification : public ShapeSpecification
{
public:
	MirrorShapeSpecification(
		std::shared_ptr<const ShapeSpecification> source_shape,
		int plane_axis,
		float plane_offset,
		MirrorShapeMode mode,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(ShapeFamily::MirrorShape, std::move(key), detail_level),
		  source_shape_(std::move(source_shape)),
		  plane_axis_(plane_axis),
		  plane_offset_(plane_offset),
		  mode_(mode),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::shared_ptr<const ShapeSpecification> &sourceShape() const
	{
		return source_shape_;
	}
	int planeAxis() const { return plane_axis_; }
	float planeOffset() const { return plane_offset_; }
	MirrorShapeMode mode() const { return mode_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override
	{
		return source_shape_ && source_shape_->requestsClosedGeometry();
	}

private:
	std::shared_ptr<const ShapeSpecification> source_shape_;
	int plane_axis_ = 0;
	float plane_offset_ = 0.0f;
	MirrorShapeMode mode_ = MirrorShapeMode::MirroredOnly;
	std::string canonical_text_;
};
