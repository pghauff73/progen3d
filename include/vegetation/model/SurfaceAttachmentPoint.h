#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class SurfaceAttachmentPoint
{
public:
	SurfaceAttachmentPoint(
		std::string identifier,
		std::string host_node_identifier,
		std::string target_identifier,
		glm::vec3 position,
		glm::vec3 normal,
		float distance)
		: identifier_(std::move(identifier)),
		  host_node_identifier_(std::move(host_node_identifier)),
		  target_identifier_(std::move(target_identifier)),
		  position_(position),
		  normal_(normal),
		  distance_(distance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hostNodeIdentifier() const
	{
		return host_node_identifier_;
	}
	const std::string &targetIdentifier() const { return target_identifier_; }
	const glm::vec3 &position() const { return position_; }
	const glm::vec3 &normal() const { return normal_; }
	float distance() const { return distance_; }

private:
	std::string identifier_;
	std::string host_node_identifier_;
	std::string target_identifier_;
	glm::vec3 position_{0.0f};
	glm::vec3 normal_{0.0f};
	float distance_ = 0.0f;
};
