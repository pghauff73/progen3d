#pragma once

#include <string>
#include <utility>

class LayerDefinition
{
public:
	LayerDefinition(std::string name,
	                float thickness,
	                std::string material_identifier)
		: name_(std::move(name)),
		  thickness_(thickness),
		  material_identifier_(std::move(material_identifier))
	{
	}

	const std::string &name() const { return name_; }
	float thickness() const { return thickness_; }
	const std::string &materialIdentifier() const { return material_identifier_; }

private:
	std::string name_;
	float thickness_ = 0.0f;
	std::string material_identifier_;
};
