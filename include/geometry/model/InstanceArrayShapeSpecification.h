#pragma once

#include "geometry/model/InstanceArraySpecification.h"
#include "geometry/model/ShapeSpecification.h"

#include <memory>
#include <string>
#include <utility>

class InstanceArrayShapeSpecification : public ShapeSpecification
{
public:
	InstanceArrayShapeSpecification(
		std::shared_ptr<const ShapeSpecification> source_shape,
		InstanceArraySpecification instance_array,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(ShapeFamily::InstanceArray, std::move(key), detail_level),
		  source_shape_(std::move(source_shape)),
		  instance_array_(std::move(instance_array)),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::shared_ptr<const ShapeSpecification> &sourceShape() const
	{
		return source_shape_;
	}

	const InstanceArraySpecification &instanceArray() const
	{
		return instance_array_;
	}

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override
	{
		return source_shape_ && source_shape_->requestsClosedGeometry();
	}

private:
	std::shared_ptr<const ShapeSpecification> source_shape_;
	InstanceArraySpecification instance_array_;
	std::string canonical_text_;
};
