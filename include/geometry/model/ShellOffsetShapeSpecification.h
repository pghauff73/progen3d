#pragma once

#include "geometry/model/ShapeSpecification.h"

#include <memory>
#include <string>
#include <utility>

enum class ShellOffsetSide
{
	Outward,
	Inward,
	Both
};

class ShellOffsetShapeSpecification : public ShapeSpecification
{
public:
	ShellOffsetShapeSpecification(
		std::shared_ptr<const ShapeSpecification> source_shape,
		float thickness,
		ShellOffsetSide side,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(ShapeFamily::ShellOffset, std::move(key), detail_level),
		  source_shape_(std::move(source_shape)),
		  thickness_(thickness),
		  side_(side),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::shared_ptr<const ShapeSpecification> &sourceShape() const
	{
		return source_shape_;
	}
	float thickness() const { return thickness_; }
	ShellOffsetSide side() const { return side_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return true; }

private:
	std::shared_ptr<const ShapeSpecification> source_shape_;
	float thickness_ = 0.0f;
	ShellOffsetSide side_ = ShellOffsetSide::Both;
	std::string canonical_text_;
};
