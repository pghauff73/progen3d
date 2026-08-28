#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryDetailRange.h"
#include "geometry/model/ShapeSpecification.h"
#include "spatial/model/SpatialInterface.h"

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class VehicleAssemblyPart
{
public:
	VehicleAssemblyPart(
		std::string part_identifier,
		std::string semantic_role,
		std::string material_identifier,
		std::shared_ptr<const ShapeSpecification> shape,
		GeneratedPrimitiveMesh generated_mesh,
		glm::mat4 local_transform,
		GeometryDetailRange detail_range = GeometryDetailRange())
		: part_identifier_(std::move(part_identifier)),
		  semantic_role_(std::move(semantic_role)),
		  material_identifier_(std::move(material_identifier)),
		  shape_(std::move(shape)),
		  generated_mesh_(std::move(generated_mesh)),
		  local_transform_(local_transform),
		  detail_range_(detail_range)
	{
	}

	const std::string &partIdentifier() const { return part_identifier_; }
	const std::string &semanticRole() const { return semantic_role_; }
	const std::string &materialIdentifier() const { return material_identifier_; }
	const std::shared_ptr<const ShapeSpecification> &shape() const { return shape_; }
	const GeneratedPrimitiveMesh &generatedMesh() const { return generated_mesh_; }
	const glm::mat4 &localTransform() const { return local_transform_; }
	const GeometryDetailRange &detailRange() const { return detail_range_; }

private:
	std::string part_identifier_;
	std::string semantic_role_;
	std::string material_identifier_;
	std::shared_ptr<const ShapeSpecification> shape_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
	glm::mat4 local_transform_{1.0f};
	GeometryDetailRange detail_range_;
};

class VehicleAssemblyGeometry
{
public:
	VehicleAssemblyGeometry(
		std::vector<VehicleAssemblyPart> parts,
		GeneratedPrimitiveMesh combined_preview_mesh,
		std::vector<SpatialInterface> interfaces,
		GeometryDetailLevel detail_level)
		: parts_(std::move(parts)),
		  combined_preview_mesh_(std::move(combined_preview_mesh)),
		  interfaces_(std::move(interfaces)),
		  detail_level_(detail_level)
	{
	}

	const std::vector<VehicleAssemblyPart> &parts() const { return parts_; }
	const GeneratedPrimitiveMesh &combinedPreviewMesh() const
	{
		return combined_preview_mesh_;
	}
	const std::vector<SpatialInterface> &interfaces() const { return interfaces_; }
	GeometryDetailLevel detailLevel() const { return detail_level_; }

	VehicleAssemblyGeometry withInterfaceOwner(
		const SpatialObjectId &owner_object_id) const
	{
		std::vector<SpatialInterface> rebound_interfaces;
		rebound_interfaces.reserve(interfaces_.size());
		for (const SpatialInterface &interface : interfaces_) {
			rebound_interfaces.push_back(
				interface.withOwnerObjectId(owner_object_id));
		}
		return VehicleAssemblyGeometry(
			parts_, combined_preview_mesh_, std::move(rebound_interfaces),
			detail_level_);
	}

private:
	std::vector<VehicleAssemblyPart> parts_;
	GeneratedPrimitiveMesh combined_preview_mesh_{std::make_shared<Mesh>(), {}};
	std::vector<SpatialInterface> interfaces_;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::Component;
};

class VehicleAssemblyBuildResult
{
public:
	static VehicleAssemblyBuildResult succeeded(VehicleAssemblyGeometry geometry)
	{
		return VehicleAssemblyBuildResult(std::move(geometry), {});
	}

	static VehicleAssemblyBuildResult failed(std::string diagnostic)
	{
		return VehicleAssemblyBuildResult(std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return geometry_.has_value(); }
	const std::optional<VehicleAssemblyGeometry> &geometry() const { return geometry_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	VehicleAssemblyBuildResult(
		std::optional<VehicleAssemblyGeometry> geometry,
		std::string diagnostic)
		: geometry_(std::move(geometry)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<VehicleAssemblyGeometry> geometry_;
	std::string diagnostic_;
};
