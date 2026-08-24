#pragma once

#include "geometry/model/ShapeSpecification.h"
#include "geometry/model/ShapeTopology.h"

#include <string>
#include <utility>

class GeneratedMeshReferenceShapeSpecification final : public ShapeSpecification
{
public:
	GeneratedMeshReferenceShapeSpecification(
		std::string mesh_key,
		ShapeSpecificationKey specification_key,
		std::string canonical_text,
		GeometryDetailLevel detail_level,
		ShapeTopology topology)
		: ShapeSpecification(
			ShapeFamily::GeneratedMeshReference,
			std::move(specification_key),
			detail_level),
		  mesh_key_(std::move(mesh_key)),
		  canonical_text_(std::move(canonical_text)),
		  topology_(topology)
	{
	}

	const std::string &meshKey() const { return mesh_key_; }
	ShapeTopology topology() const { return topology_; }
	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override
	{
		return topology_ != ShapeTopology::Surface;
	}

private:
	std::string mesh_key_;
	std::string canonical_text_;
	ShapeTopology topology_ = ShapeTopology::Solid;
};
