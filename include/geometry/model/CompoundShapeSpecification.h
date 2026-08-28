#pragma once

#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CompoundShapePartSpecification
{
public:
	CompoundShapePartSpecification(
		std::string purpose,
		std::shared_ptr<const ShapeSpecification> shape,
		glm::mat4 local_transform)
		: purpose_(std::move(purpose)),
		  shape_(std::move(shape)),
		  local_transform_(local_transform)
	{
	}

	const std::string &purpose() const { return purpose_; }
	const std::shared_ptr<const ShapeSpecification> &shape() const { return shape_; }
	const glm::mat4 &localTransform() const { return local_transform_; }

private:
	std::string purpose_;
	std::shared_ptr<const ShapeSpecification> shape_;
	glm::mat4 local_transform_{1.0f};
};

class CompoundShapeSpecification : public ShapeSpecification
{
public:
	CompoundShapeSpecification(
		std::vector<CompoundShapePartSpecification> parts,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(
			  ShapeFamily::CompoundShape, std::move(key), detail_level),
		  parts_(std::move(parts)),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::vector<CompoundShapePartSpecification> &parts() const { return parts_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override
	{
		for (const CompoundShapePartSpecification &part : parts_) {
			if (!part.shape() || !part.shape()->requestsClosedGeometry()) return false;
		}
		return !parts_.empty();
	}

private:
	std::vector<CompoundShapePartSpecification> parts_;
	std::string canonical_text_;
};
