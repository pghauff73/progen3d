#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class VegetationGrowthTip
{
public:
	VegetationGrowthTip(std::string identifier,
	                    std::string host_node_identifier,
	                    glm::vec3 direction,
	                    bool active)
		: identifier_(std::move(identifier)),
		  host_node_identifier_(std::move(host_node_identifier)),
		  direction_(direction),
		  active_(active)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hostNodeIdentifier() const
	{
		return host_node_identifier_;
	}
	const glm::vec3 &direction() const { return direction_; }
	bool isActive() const { return active_; }

private:
	std::string identifier_;
	std::string host_node_identifier_;
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
	bool active_ = true;
};

