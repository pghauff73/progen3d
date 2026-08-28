#pragma once

#include "vegetation/model/VegetationOrganType.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

class OrganAttachment
{
public:
	OrganAttachment(std::string identifier,
	                std::string host_node_identifier,
	                VegetationOrganType organ_type,
	                glm::mat4 local_transform,
	                float developmental_age,
	                std::string shape_identifier,
	                std::string host_interface_identifier = "node_surface",
	                std::string organ_interface_identifier = "organ_base")
		: identifier_(std::move(identifier)),
		  host_node_identifier_(std::move(host_node_identifier)),
		  organ_type_(organ_type),
		  local_transform_(local_transform),
		  developmental_age_(developmental_age),
		  shape_identifier_(std::move(shape_identifier)),
		  host_interface_identifier_(std::move(host_interface_identifier)),
		  organ_interface_identifier_(std::move(organ_interface_identifier))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hostNodeIdentifier() const
	{
		return host_node_identifier_;
	}
	VegetationOrganType organType() const { return organ_type_; }
	const glm::mat4 &localTransform() const { return local_transform_; }
	float developmentalAge() const { return developmental_age_; }
	const std::string &shapeIdentifier() const { return shape_identifier_; }
	const std::string &hostInterfaceIdentifier() const
	{
		return host_interface_identifier_;
	}
	const std::string &organInterfaceIdentifier() const
	{
		return organ_interface_identifier_;
	}

private:
	std::string identifier_;
	std::string host_node_identifier_;
	VegetationOrganType organ_type_ = VegetationOrganType::Leaf;
	glm::mat4 local_transform_{1.0f};
	float developmental_age_ = 0.0f;
	std::string shape_identifier_;
	std::string host_interface_identifier_;
	std::string organ_interface_identifier_;
};
